#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "domain/common.hpp"

namespace nexustal::domain::environment
{
using ::nexustal::domain::Timestamp;
using ::nexustal::domain::UUID;

struct Variable
{
    std::string key;
    std::string value;
    bool secret = false;
    std::string description;
};

struct Environment
{
    UUID id;
    UUID project_id;
    UUID created_by;
    std::string name;
    std::vector<Variable> variables;
    Timestamp created_at;
    Timestamp updated_at;
};

struct GlobalVariables
{
    UUID project_id;
    std::vector<Variable> variables;
};

inline auto variableToJson(const Variable& variable) -> nlohmann::json
{
    return nlohmann::json{{"key", variable.key},
                          {"value", variable.value},
                          {"secret", variable.secret},
                          {"description", variable.description}};
}

inline auto variableFromJson(const nlohmann::json& json) -> Variable
{
    return Variable{
        .key = json.value("key", std::string{}),
        .value = json.value("value", std::string{}),
        .secret = json.value("secret", false),
        .description = json.value("description", std::string{}),
    };
}

inline auto variablesToJson(const std::vector<Variable>& variables) -> nlohmann::json
{
    auto payload = nlohmann::json::array();
    for (const auto& variable : variables)
    {
        payload.push_back(variableToJson(variable));
    }
    return payload;
}

inline auto variablesFromJson(const nlohmann::json& json) -> std::vector<Variable>
{
    std::vector<Variable> variables;
    if (!json.is_array())
    {
        return variables;
    }

    variables.reserve(json.size());
    for (const auto& item : json)
    {
        variables.push_back(variableFromJson(item));
    }
    return variables;
}
} // namespace nexustal::domain::environment