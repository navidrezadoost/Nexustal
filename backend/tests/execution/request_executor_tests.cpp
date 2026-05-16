#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <unordered_map>

#include "application/execution/request_executor.hpp"
#include "domain/environment/environment_repository.hpp"

namespace
{
class FakeEnvironmentRepository final : public nexustal::domain::environment::IEnvironmentRepository
{
public:
    std::vector<nexustal::domain::environment::Environment> findByProject(const nexustal::domain::UUID&) override
    {
        return environments;
    }

    std::optional<nexustal::domain::environment::Environment> findById(const nexustal::domain::UUID& id) override
    {
        for (const auto& environment : environments)
        {
            if (environment.id == id)
            {
                return environment;
            }
        }
        return std::nullopt;
    }

    nexustal::domain::UUID create(const nexustal::domain::environment::Environment& env) override
    {
        environments.push_back(env);
        return env.id;
    }

    bool update(const nexustal::domain::environment::Environment&) override
    {
        return true;
    }

    bool remove(const nexustal::domain::UUID&) override
    {
        return true;
    }

    nexustal::domain::environment::GlobalVariables getGlobalVariables(const nexustal::domain::UUID& project_id) override
    {
        globals.project_id = project_id;
        return globals;
    }

    bool setGlobalVariables(const nexustal::domain::environment::GlobalVariables& value) override
    {
        globals = value;
        return true;
    }

    nexustal::domain::environment::GlobalVariables globals;
    std::vector<nexustal::domain::environment::Environment> environments;
};

class FakeTransport final : public nexustal::application::execution::IHttpTransport
{
public:
    auto execute(const nexustal::domain::endpoint::Endpoint& endpoint,
                 const nlohmann::json& resolvedVariables)
        -> nexustal::application::execution::HttpExecutionResponse override
    {
        captured_endpoint = endpoint;
        captured_variables = resolvedVariables;
        return response;
    }

    nexustal::domain::endpoint::Endpoint captured_endpoint;
    nlohmann::json captured_variables;
    nexustal::application::execution::HttpExecutionResponse response{
        .status_code = 200,
        .response_time_ms = 15.0,
        .headers = nlohmann::json{{"content-type", "application/json"}},
        .body = R"({"data":{"ok":true}})",
    };
};

auto makeRestEndpoint() -> nexustal::domain::endpoint::Endpoint
{
    nexustal::domain::endpoint::Endpoint endpoint;
    endpoint.id = "endpoint-1";
    endpoint.project_id = "project-1";
    endpoint.created_by = "user-1";
    endpoint.details = nexustal::domain::endpoint::RestDetails{
        .method = nexustal::domain::endpoint::HttpMethod::Get,
        .url_path = "{{base_url}}/pets/{{$guid}}",
    };
    return endpoint;
}
} // namespace

TEST(RequestExecutorTest, ResolvesEnvironmentVariablesBeforeTransport)
{
    auto repository = std::make_shared<FakeEnvironmentRepository>();
    repository->globals = nexustal::domain::environment::GlobalVariables{
        .project_id = "project-1",
        .variables = {{.key = "base_url", .value = "https://api.example.test"}},
    };

    auto cache = std::make_shared<nexustal::infrastructure::adapters::EnvironmentCache>(nullptr, repository);
    auto transport = std::make_shared<FakeTransport>();
    nexustal::application::execution::RequestExecutor executor{
        cache,
        [] { return std::make_unique<nexustal::infrastructure::sandbox::QuickJsSandbox>(); },
        transport,
    };

    nexustal::domain::execution::RequestBundle bundle;
    bundle.endpoint = makeRestEndpoint();

    const auto result = executor.execute(bundle, "user-1");

    EXPECT_EQ(result.status_code, 200);
    EXPECT_EQ(transport->captured_variables["base_url"], "https://api.example.test");
    const auto details = std::get<nexustal::domain::endpoint::RestDetails>(transport->captured_endpoint.details);
    EXPECT_NE(details.url_path.find("https://api.example.test/pets/"), std::string::npos);
    EXPECT_EQ(details.url_path.find("{{$guid}}"), std::string::npos);
}

TEST(RequestExecutorTest, ExecutesNtAssertionsWhenQuickJsIsAvailable)
{
    auto repository = std::make_shared<FakeEnvironmentRepository>();
    repository->globals = nexustal::domain::environment::GlobalVariables{
        .project_id = "project-1",
        .variables = {{.key = "base_url", .value = "https://api.example.test"}},
    };

    auto cache = std::make_shared<nexustal::infrastructure::adapters::EnvironmentCache>(nullptr, repository);
    auto transport = std::make_shared<FakeTransport>();
    nexustal::application::execution::RequestExecutor executor{
        cache,
        [] { return std::make_unique<nexustal::infrastructure::sandbox::QuickJsSandbox>(); },
        transport,
    };

    nexustal::domain::execution::RequestBundle bundle;
    bundle.endpoint = makeRestEndpoint();
    bundle.test_script = R"JS(
nt.test("status is 200", () => {
  nt.response.to.have.status(200);
});
)JS";

    auto probeSandbox = nexustal::infrastructure::sandbox::QuickJsSandbox{};
    if (!probeSandbox.isAvailable())
    {
        EXPECT_THROW((void)executor.execute(bundle, "user-1"), std::runtime_error);
        return;
    }

    const auto result = executor.execute(bundle, "user-1");
    ASSERT_EQ(result.test_results.size(), 1U);
    EXPECT_TRUE(result.test_results.front().passed);
    EXPECT_EQ(result.test_results.front().name, "status is 200");
}