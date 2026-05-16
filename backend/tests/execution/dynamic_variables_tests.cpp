#include <gtest/gtest.h>

#include "application/execution/dynamic_variables.hpp"

TEST(DynamicVariableEngineTest, ResolvesKnownTokens)
{
    const auto resolved = nexustal::application::execution::DynamicVariableEngine::resolve(
        "{{$guid}}/{{$randomInt}}/{{$timestamp}}/{{$isoTimestamp}}/{{$randomEmail}}");

    EXPECT_EQ(resolved.find("{{$guid}}"), std::string::npos);
    EXPECT_EQ(resolved.find("{{$randomInt}}"), std::string::npos);
    EXPECT_EQ(resolved.find("{{$timestamp}}"), std::string::npos);
    EXPECT_EQ(resolved.find("{{$isoTimestamp}}"), std::string::npos);
    EXPECT_EQ(resolved.find("{{$randomEmail}}"), std::string::npos);
}

TEST(DynamicVariableEngineTest, ResolvesNestedJsonStrings)
{
    const nlohmann::json payload{{"url", "https://example.test/{{$guid}}"},
                                 {"headers", nlohmann::json::array({"{{$randomEmail}}"})}};

    const auto resolved = nexustal::application::execution::DynamicVariableEngine::resolveJson(payload);

    EXPECT_NE(resolved["url"].get<std::string>().find("https://example.test/"), std::string::npos);
    EXPECT_EQ(resolved["headers"][0].get<std::string>().find("{{$randomEmail}}"), std::string::npos);
}