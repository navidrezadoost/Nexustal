#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "infrastructure/sandbox/nt_api_bindings.hpp"

namespace
{
using nexustal::infrastructure::sandbox::NtExecutionBindings;
using nexustal::infrastructure::sandbox::QuickJsSandbox;
using nexustal::infrastructure::sandbox::installNtApi;
} // namespace

TEST(QuickJsSandboxTest, ReportsAvailability)
{
    QuickJsSandbox sandbox;

#if defined(NEXUSTAL_HAS_QUICKJS)
    EXPECT_TRUE(sandbox.isAvailable());
#else
    EXPECT_FALSE(sandbox.isAvailable());
#endif
}

TEST(QuickJsSandboxTest, ExecuteThrowsWhenQuickJsIsUnavailable)
{
    QuickJsSandbox sandbox;
    if (sandbox.isAvailable())
    {
        GTEST_SKIP() << "QuickJS is enabled for this build";
    }

    EXPECT_THROW((void)sandbox.execute("1 + 1"), std::runtime_error);
}

TEST(QuickJsSandboxTest, RegisteredFunctionReturnsJsonPayload)
{
    QuickJsSandbox sandbox;
    if (!sandbox.isAvailable())
    {
        GTEST_SKIP() << "QuickJS package is not enabled";
    }

    sandbox.registerFunction("echoNative", [](const std::string& payload) {
        return payload;
    });

    EXPECT_EQ(sandbox.execute("echoNative({ value: 42, label: 'ok' })"), R"({"value":42,"label":"ok"})");
}

TEST(QuickJsSandboxTest, TimeoutInterruptsInfiniteLoop)
{
    QuickJsSandbox sandbox;
    if (!sandbox.isAvailable())
    {
        GTEST_SKIP() << "QuickJS package is not enabled";
    }

    sandbox.setExecutionTimeout(50);
    EXPECT_THROW((void)sandbox.execute("while (true) {}"), std::runtime_error);
}

TEST(QuickJsSandboxTest, NtBindingsExposeEnvironmentAndAssertions)
{
    QuickJsSandbox sandbox;
    if (!sandbox.isAvailable())
    {
        GTEST_SKIP() << "QuickJS package is not enabled";
    }

    std::unordered_map<std::string, std::string> environment;
    std::vector<std::string> logs;

    installNtApi(sandbox, NtExecutionBindings{
                              .log = [&](const std::string& message) {
                                  logs.push_back(message);
                              },
                              .environmentGet = [&](const std::string& key) {
                                  const auto iterator = environment.find(key);
                                  return iterator == environment.end() ? std::string{"null"} : iterator->second;
                              },
                              .environmentSet = [&](const std::string& key, const std::string& value) {
                                  environment[key] = value;
                              },
                          });

    const auto result = sandbox.execute(
        R"JS(
nt.environment.set("baseUrl", "https://api.example.test");
nt.console.log("sandbox booted");
nt.test("response assertions", () => {
  nt.response.to.have.status(200);
  nt.response.to.have.header("content-type", "application/json");
  nt.response.to.have.jsonBody("data.ok", true);
  nt.response.to.have.responseTimeLessThan(50);
  nt.expect(nt.environment.get("baseUrl")).to.equal("https://api.example.test");
});
__nt_tests;
)JS",
        R"({"response":{"status":200,"headers":{"content-type":"application/json"},"jsonBody":{"data":{"ok":true}},"responseTimeMs":12}})");

    EXPECT_NE(result.find("\"status\":\"passed\""), std::string::npos);
    EXPECT_EQ(environment.at("baseUrl"), R"("https://api.example.test")");
    ASSERT_EQ(logs.size(), 1U);
    EXPECT_NE(logs.front().find("sandbox booted"), std::string::npos);
}