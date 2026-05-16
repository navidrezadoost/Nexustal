#pragma once

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "domain/common.hpp"
#include "domain/endpoint.hpp"

namespace nexustal::domain::execution
{
using ::nexustal::domain::Timestamp;
using ::nexustal::domain::UUID;

struct RequestBundle
{
    endpoint::Endpoint endpoint;
    std::optional<UUID> environment_id;
    std::string pre_request_script;
    std::string test_script;
    nlohmann::json overrides = nlohmann::json::object();
};

struct TestAssertion
{
    std::string name;
    bool passed = false;
    std::string error_message;
};

struct ExecutionResult
{
    UUID id;
    UUID endpoint_id;
    int status_code = 0;
    double response_time_ms = 0.0;
    std::size_t payload_size = 0;
    nlohmann::json response_headers = nlohmann::json::object();
    std::string response_body;
    std::vector<TestAssertion> test_results;
    Timestamp executed_at;
};
} // namespace nexustal::domain::execution