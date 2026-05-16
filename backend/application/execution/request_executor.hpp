#pragma once

#include <functional>
#include <memory>

#include <nlohmann/json.hpp>

#include "application/execution/request_bundle.hpp"
#include "domain/environment/environment_repository.hpp"
#include "infrastructure/adapters/environment_cache.hpp"
#include "infrastructure/sandbox/quickjs_sandbox.hpp"

namespace nexustal::application::execution
{
struct HttpExecutionResponse
{
    int status_code = 0;
    double response_time_ms = 0.0;
    nlohmann::json headers = nlohmann::json::object();
    std::string body;
};

class IHttpTransport
{
public:
    virtual ~IHttpTransport() = default;

    virtual auto execute(const domain::endpoint::Endpoint& endpoint,
                         const nlohmann::json& resolvedVariables) -> HttpExecutionResponse = 0;
};

class RequestExecutor
{
public:
    using SandboxFactory = std::function<std::unique_ptr<infrastructure::sandbox::QuickJsSandbox>()>;

    RequestExecutor(std::shared_ptr<infrastructure::adapters::EnvironmentCache> environmentCache,
                    SandboxFactory sandboxFactory,
                    std::shared_ptr<IHttpTransport> transport);

    [[nodiscard]] auto execute(const domain::execution::RequestBundle& bundle,
                               const domain::UUID& user_id) -> domain::execution::ExecutionResult;

private:
    [[nodiscard]] auto applyOverrides(const nlohmann::json& base, const nlohmann::json& overrides) const
        -> nlohmann::json;
    [[nodiscard]] auto substituteEndpoint(const domain::endpoint::Endpoint& endpoint,
                                          const nlohmann::json& resolvedVariables) const
        -> domain::endpoint::Endpoint;
    [[nodiscard]] auto substituteString(const std::string& input, const nlohmann::json& resolvedVariables) const
        -> std::string;
    [[nodiscard]] auto substituteJson(const nlohmann::json& input, const nlohmann::json& resolvedVariables) const
        -> nlohmann::json;
    [[nodiscard]] auto runScript(const std::string& script,
                                 nlohmann::json& resolvedVariables,
                                 const nlohmann::json& context) const -> nlohmann::json;

    std::shared_ptr<infrastructure::adapters::EnvironmentCache> environmentCache_;
    SandboxFactory sandboxFactory_;
    std::shared_ptr<IHttpTransport> transport_;
};
} // namespace nexustal::application::execution