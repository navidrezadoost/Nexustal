#pragma once

#include <optional>

#include <drogon/nosql/RedisClient.h>
#include <nlohmann/json.hpp>

#include "domain/environment/environment_repository.hpp"

namespace nexustal::infrastructure::adapters
{
class EnvironmentCache
{
public:
    static constexpr int TtlSeconds = 600;

    EnvironmentCache(drogon::nosql::RedisClientPtr redis,
                     std::shared_ptr<domain::environment::IEnvironmentRepository> repository);

    [[nodiscard]] auto get(const domain::UUID& env_id) -> std::optional<domain::environment::Environment>;
    void set(const domain::environment::Environment& env);
    void invalidate(const domain::UUID& env_id);
    [[nodiscard]] auto resolveVariables(const domain::UUID& project_id,
                                        const std::optional<domain::UUID>& env_id) -> nlohmann::json;

private:
    [[nodiscard]] static auto cacheKeyFor(const domain::UUID& env_id) -> std::string;

    drogon::nosql::RedisClientPtr redis_;
    std::shared_ptr<domain::environment::IEnvironmentRepository> repository_;
};
} // namespace nexustal::infrastructure::adapters