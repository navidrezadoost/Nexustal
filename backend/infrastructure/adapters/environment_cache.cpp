#include "infrastructure/adapters/environment_cache.hpp"

#include <nlohmann/json.hpp>

namespace nexustal::infrastructure::adapters
{
namespace
{
auto parseValue(const std::string& raw) -> nlohmann::json
{
    const auto parsed = nlohmann::json::parse(raw, nullptr, false);
    return parsed.is_discarded() ? nlohmann::json(raw) : parsed;
}

auto environmentToJson(const domain::environment::Environment& environment) -> nlohmann::json
{
    return nlohmann::json{{"id", environment.id},
                          {"project_id", environment.project_id},
                          {"created_by", environment.created_by},
                          {"name", environment.name},
                          {"variables", domain::environment::variablesToJson(environment.variables)}};
}

auto environmentFromJson(const nlohmann::json& json) -> domain::environment::Environment
{
    return domain::environment::Environment{
        .id = json.value("id", std::string{}),
        .project_id = json.value("project_id", std::string{}),
        .created_by = json.value("created_by", std::string{}),
        .name = json.value("name", std::string{}),
        .variables = domain::environment::variablesFromJson(json.value("variables", nlohmann::json::array())),
    };
}
} // namespace

EnvironmentCache::EnvironmentCache(drogon::nosql::RedisClientPtr redis,
                                   std::shared_ptr<domain::environment::IEnvironmentRepository> repository)
    : redis_(std::move(redis))
    , repository_(std::move(repository))
{
}

auto EnvironmentCache::cacheKeyFor(const domain::UUID& env_id) -> std::string
{
    return "environment:" + env_id;
}

auto EnvironmentCache::get(const domain::UUID& env_id) -> std::optional<domain::environment::Environment>
{
    if (redis_)
    {
        try
        {
            const auto result = redis_->execCommandSync("GET %s", cacheKeyFor(env_id).c_str());
            const auto encoded = result.asString();
            if (!encoded.empty())
            {
                return environmentFromJson(nlohmann::json::parse(encoded));
            }
        }
        catch (const std::exception&)
        {
        }
    }

    if (!repository_)
    {
        return std::nullopt;
    }

    const auto environment = repository_->findById(env_id);
    if (environment.has_value())
    {
        set(environment.value());
    }
    return environment;
}

void EnvironmentCache::set(const domain::environment::Environment& env)
{
    if (!redis_)
    {
        return;
    }

    const auto payload = environmentToJson(env).dump();
    redis_->execCommandAsync(
        [](const drogon::nosql::RedisResult&) {},
        [](const std::exception&) {},
        "SETEX %s %d %s",
        cacheKeyFor(env.id).c_str(),
        TtlSeconds,
        payload.c_str());
}

void EnvironmentCache::invalidate(const domain::UUID& env_id)
{
    if (!redis_)
    {
        return;
    }

    redis_->execCommandAsync(
        [](const drogon::nosql::RedisResult&) {},
        [](const std::exception&) {},
        "DEL %s",
        cacheKeyFor(env_id).c_str());
}

auto EnvironmentCache::resolveVariables(const domain::UUID& project_id,
                                        const std::optional<domain::UUID>& env_id) -> nlohmann::json
{
    nlohmann::json resolved = nlohmann::json::object();
    if (!repository_)
    {
        return resolved;
    }

    const auto globals = repository_->getGlobalVariables(project_id);
    for (const auto& variable : globals.variables)
    {
        resolved[variable.key] = parseValue(variable.value);
    }

    if (!env_id.has_value())
    {
        return resolved;
    }

    const auto environment = get(env_id.value());
    if (!environment.has_value())
    {
        return resolved;
    }

    for (const auto& variable : environment->variables)
    {
        resolved[variable.key] = parseValue(variable.value);
    }

    return resolved;
}
} // namespace nexustal::infrastructure::adapters