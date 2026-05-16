#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "domain/common.hpp"

namespace nexustal::domain::endpoint
{
using ::nexustal::domain::Timestamp;
using ::nexustal::domain::UUID;

enum class EndpointType : std::uint8_t
{
    Rest = 0,
    Graphql = 1,
    Soap = 2,
    Websocket = 3,
};

enum class HttpMethod : std::uint8_t
{
    Get = 0,
    Post,
    Put,
    Delete,
    Patch,
    Head,
    Options,
};

struct RestDetails
{
    HttpMethod method = HttpMethod::Get;
    std::string url_path;
    nlohmann::json query_params = nlohmann::json::array();
    nlohmann::json headers = nlohmann::json::array();
    nlohmann::json body = nlohmann::json::object();
    std::string body_raw;
};

struct GraphqlDetails
{
    std::string query;
    nlohmann::json variables = nlohmann::json::object();
    std::optional<std::string> schema_url;
};

struct SoapDetails
{
    std::string wsdl_url;
    std::string operation;
    std::string request_xml;
};

struct WebsocketDetails
{
    std::string connection_url;
    std::vector<std::string> protocols;
    nlohmann::json sample_messages = nlohmann::json::array();
};

using EndpointDetails = std::variant<RestDetails, GraphqlDetails, SoapDetails, WebsocketDetails>;

struct Endpoint
{
    UUID id;
    UUID project_id;
    nlohmann::json name = nlohmann::json::object();
    nlohmann::json description = nlohmann::json::object();
    EndpointType type = EndpointType::Rest;
    std::string status = "draft";
    UUID created_by;
    Timestamp created_at;
    Timestamp updated_at;
    EndpointDetails details = RestDetails{};
    std::vector<UUID> tag_ids;
};

inline auto endpointTypeToString(EndpointType type) -> std::string
{
    switch (type)
    {
    case EndpointType::Graphql:
        return "GRAPHQL";
    case EndpointType::Soap:
        return "SOAP";
    case EndpointType::Websocket:
        return "WEBSOCKET";
    case EndpointType::Rest:
    default:
        return "REST";
    }
}

inline auto endpointTypeFromString(const std::string& value) -> EndpointType
{
    if (value == "GRAPHQL")
    {
        return EndpointType::Graphql;
    }
    if (value == "SOAP")
    {
        return EndpointType::Soap;
    }
    if (value == "WEBSOCKET")
    {
        return EndpointType::Websocket;
    }
    return EndpointType::Rest;
}

inline auto httpMethodToString(HttpMethod method) -> std::string
{
    switch (method)
    {
    case HttpMethod::Post:
        return "POST";
    case HttpMethod::Put:
        return "PUT";
    case HttpMethod::Delete:
        return "DELETE";
    case HttpMethod::Patch:
        return "PATCH";
    case HttpMethod::Head:
        return "HEAD";
    case HttpMethod::Options:
        return "OPTIONS";
    case HttpMethod::Get:
    default:
        return "GET";
    }
}

inline auto httpMethodFromString(const std::string& value) -> HttpMethod
{
    if (value == "POST")
    {
        return HttpMethod::Post;
    }
    if (value == "PUT")
    {
        return HttpMethod::Put;
    }
    if (value == "DELETE")
    {
        return HttpMethod::Delete;
    }
    if (value == "PATCH")
    {
        return HttpMethod::Patch;
    }
    if (value == "HEAD")
    {
        return HttpMethod::Head;
    }
    if (value == "OPTIONS")
    {
        return HttpMethod::Options;
    }
    return HttpMethod::Get;
}
} // namespace nexustal::domain::endpoint
