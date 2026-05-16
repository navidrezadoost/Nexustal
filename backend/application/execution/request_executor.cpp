#include "application/execution/request_executor.hpp"

#include <chrono>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "application/execution/dynamic_variables.hpp"
#include "infrastructure/sandbox/nt_api_bindings.hpp"

namespace nexustal::application::execution
{
namespace
{
auto parseBodyJson(const std::string& body) -> nlohmann::json
{
    const auto parsed = nlohmann::json::parse(body, nullptr, false);
    return parsed.is_discarded() ? nlohmann::json::object() : parsed;
}

void mergeObjectInto(nlohmann::json& target, const nlohmann::json& source)
{
    if (!source.is_object())
    {
        return;
    }

    for (auto iterator = source.begin(); iterator != source.end(); ++iterator)
    {
        target[iterator.key()] = iterator.value();
    }
}
} // namespace

RequestExecutor::RequestExecutor(std::shared_ptr<infrastructure::adapters::EnvironmentCache> environmentCache,
                                 SandboxFactory sandboxFactory,
                                 std::shared_ptr<IHttpTransport> transport)
    : environmentCache_(std::move(environmentCache))
    , sandboxFactory_(std::move(sandboxFactory))
    , transport_(std::move(transport))
{
}

auto RequestExecutor::execute(const domain::execution::RequestBundle& bundle,
                              const domain::UUID& user_id) -> domain::execution::ExecutionResult
{
    if (!environmentCache_)
    {
        throw std::runtime_error("environment cache is not configured");
    }
    if (!transport_)
    {
        throw std::runtime_error("HTTP transport is not configured");
    }

    auto resolvedVariables = environmentCache_->resolveVariables(bundle.endpoint.project_id, bundle.environment_id);
    resolvedVariables = applyOverrides(resolvedVariables, DynamicVariableEngine::resolveJson(bundle.overrides));

    if (!bundle.pre_request_script.empty())
    {
        nlohmann::json context{{"environment", resolvedVariables},
                               {"request", {{"endpoint_id", bundle.endpoint.id}, {"type", domain::endpoint::endpointTypeToString(bundle.endpoint.type)}}},
                               {"user_id", user_id}};
        runScript(bundle.pre_request_script, resolvedVariables, context);
    }

    const auto substitutedEndpoint = substituteEndpoint(bundle.endpoint, resolvedVariables);
    const auto response = transport_->execute(substitutedEndpoint, resolvedVariables);

    domain::execution::ExecutionResult result{
        .id = domain::generate_uuid(),
        .endpoint_id = substitutedEndpoint.id,
        .status_code = response.status_code,
        .response_time_ms = response.response_time_ms,
        .payload_size = response.body.size(),
        .response_headers = response.headers,
        .response_body = response.body,
        .executed_at = std::chrono::system_clock::now(),
    };

    if (!bundle.test_script.empty())
    {
        nlohmann::json context{{"environment", resolvedVariables},
                               {"response",
                                {{"status", response.status_code},
                                 {"headers", response.headers},
                                 {"body", response.body},
                                 {"jsonBody", parseBodyJson(response.body)},
                                 {"responseTimeMs", response.response_time_ms}}}};

        const auto rawTests = runScript(
            std::string{"(() => {\n"} + bundle.test_script + "\nreturn globalThis.__nt_tests || [];\n})()",
            resolvedVariables,
            context);

        if (rawTests.is_array())
        {
            result.test_results.reserve(rawTests.size());
            for (const auto& item : rawTests)
            {
                result.test_results.push_back(domain::execution::TestAssertion{
                    .name = item.value("name", std::string{}),
                    .passed = item.value("status", std::string{}) == "passed",
                    .error_message = item.value("message", std::string{}),
                });
            }
        }
    }

    return result;
}

auto RequestExecutor::applyOverrides(const nlohmann::json& base, const nlohmann::json& overrides) const
    -> nlohmann::json
{
    auto merged = base.is_object() ? base : nlohmann::json::object();
    mergeObjectInto(merged, overrides);
    return merged;
}

auto RequestExecutor::substituteEndpoint(const domain::endpoint::Endpoint& endpoint,
                                         const nlohmann::json& resolvedVariables) const
    -> domain::endpoint::Endpoint
{
    auto resolved = endpoint;
    resolved.name = substituteJson(resolved.name, resolvedVariables);
    resolved.description = substituteJson(resolved.description, resolvedVariables);

    std::visit(
        [&](auto& details) {
            using Details = std::decay_t<decltype(details)>;

            if constexpr (std::is_same_v<Details, domain::endpoint::RestDetails>)
            {
                details.url_path = substituteString(details.url_path, resolvedVariables);
                details.query_params = substituteJson(details.query_params, resolvedVariables);
                details.headers = substituteJson(details.headers, resolvedVariables);
                details.body = substituteJson(details.body, resolvedVariables);
                details.body_raw = substituteString(details.body_raw, resolvedVariables);
            }
            else if constexpr (std::is_same_v<Details, domain::endpoint::GraphqlDetails>)
            {
                details.query = substituteString(details.query, resolvedVariables);
                details.variables = substituteJson(details.variables, resolvedVariables);
                if (details.schema_url.has_value())
                {
                    details.schema_url = substituteString(details.schema_url.value(), resolvedVariables);
                }
            }
            else if constexpr (std::is_same_v<Details, domain::endpoint::SoapDetails>)
            {
                details.wsdl_url = substituteString(details.wsdl_url, resolvedVariables);
                details.operation = substituteString(details.operation, resolvedVariables);
                details.request_xml = substituteString(details.request_xml, resolvedVariables);
            }
            else if constexpr (std::is_same_v<Details, domain::endpoint::WebsocketDetails>)
            {
                details.connection_url = substituteString(details.connection_url, resolvedVariables);
                for (auto& protocol : details.protocols)
                {
                    protocol = substituteString(protocol, resolvedVariables);
                }
                details.sample_messages = substituteJson(details.sample_messages, resolvedVariables);
            }
        },
        resolved.details);

    return resolved;
}

auto RequestExecutor::substituteString(const std::string& input, const nlohmann::json& resolvedVariables) const
    -> std::string
{
    auto result = DynamicVariableEngine::resolve(input);
    if (!resolvedVariables.is_object())
    {
        return result;
    }

    for (auto iterator = resolvedVariables.begin(); iterator != resolvedVariables.end(); ++iterator)
    {
        const auto token = "{{" + iterator.key() + "}}";
        const auto replacement = iterator.value().is_string() ? iterator.value().get<std::string>() : iterator.value().dump();

        std::size_t position = 0;
        while ((position = result.find(token, position)) != std::string::npos)
        {
            result.replace(position, token.size(), replacement);
            position += replacement.size();
        }
    }

    return result;
}

auto RequestExecutor::substituteJson(const nlohmann::json& input, const nlohmann::json& resolvedVariables) const
    -> nlohmann::json
{
    if (input.is_string())
    {
        return substituteString(input.get<std::string>(), resolvedVariables);
    }

    if (input.is_array())
    {
        auto result = nlohmann::json::array();
        for (const auto& item : input)
        {
            result.push_back(substituteJson(item, resolvedVariables));
        }
        return result;
    }

    if (input.is_object())
    {
        auto result = nlohmann::json::object();
        for (auto iterator = input.begin(); iterator != input.end(); ++iterator)
        {
            result[iterator.key()] = substituteJson(iterator.value(), resolvedVariables);
        }
        return result;
    }

    return input;
}

auto RequestExecutor::runScript(const std::string& script,
                                nlohmann::json& resolvedVariables,
                                const nlohmann::json& context) const -> nlohmann::json
{
    if (!sandboxFactory_)
    {
        throw std::runtime_error("sandbox factory is not configured");
    }

    auto sandbox = sandboxFactory_();
    if (!sandbox || !sandbox->isAvailable())
    {
        throw std::runtime_error("QuickJS sandbox is not enabled in this build");
    }

    infrastructure::sandbox::installNtApi(
        *sandbox,
        infrastructure::sandbox::NtExecutionBindings{
            .log = [](const std::string&) {},
            .environmentGet = [&](const std::string& key) {
                if (!resolvedVariables.contains(key))
                {
                    return std::string{"null"};
                }
                return resolvedVariables[key].dump();
            },
            .environmentSet = [&](const std::string& key, const std::string& value) {
                const auto parsed = nlohmann::json::parse(value, nullptr, false);
                resolvedVariables[key] = parsed.is_discarded() ? nlohmann::json(value) : parsed;
            },
        });

    const auto result = sandbox->execute(script, context.dump());
    const auto parsed = nlohmann::json::parse(result, nullptr, false);
    return parsed.is_discarded() ? nlohmann::json::object() : parsed;
}
} // namespace nexustal::application::execution