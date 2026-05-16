#include "controllers/endpoint_controller.hpp"

#include <json/json.h>

namespace nexustal::controllers
{
namespace
{
auto toJsonValue(const nlohmann::json& value) -> Json::Value
{
    Json::CharReaderBuilder builder;
    Json::Value result;
    std::string errors;
    auto serialized = value.dump();
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (reader->parse(serialized.data(), serialized.data() + serialized.size(), &result, &errors))
    {
        return result;
    }

    return Json::Value{};
}

auto jsonError(const std::string& message, drogon::HttpStatusCode status) -> drogon::HttpResponsePtr
{
    Json::Value payload;
    payload["error"] = message;
    auto response = drogon::HttpResponse::newHttpJsonResponse(payload);
    response->setStatusCode(status);
    return response;
}

auto endpointTypeForPayload(const Json::Value& json) -> domain::endpoint::EndpointType
{
    return domain::endpoint::endpointTypeFromString(json["type"].asString());
}
} // namespace

void EndpointController::configure(std::shared_ptr<application::EndpointService> service)
{
    service_ = std::move(service);
}

auto EndpointController::parseEndpointPayload(const Json::Value& json,
                                              const domain::UUID& projectId,
                                              const domain::UUID& userId) -> domain::endpoint::Endpoint
{
    domain::endpoint::Endpoint endpoint;
    endpoint.project_id = projectId;
    endpoint.created_by = userId;
    endpoint.type = endpointTypeForPayload(json);
    endpoint.status = json.isMember("status") ? json["status"].asString() : "draft";
    endpoint.name = nlohmann::json::parse(Json::writeString(Json::StreamWriterBuilder{}, json["name"]));
    endpoint.description = json.isMember("description")
                               ? nlohmann::json::parse(Json::writeString(Json::StreamWriterBuilder{}, json["description"]))
                               : nlohmann::json::object();

    const auto& details = json["details"];
    switch (endpoint.type)
    {
    case domain::endpoint::EndpointType::Graphql:
        endpoint.details = domain::endpoint::GraphqlDetails{
            .query = details["query"].asString(),
            .variables = details.isMember("variables")
                             ? nlohmann::json::parse(Json::writeString(Json::StreamWriterBuilder{}, details["variables"]))
                             : nlohmann::json::object(),
            .schema_url = details.isMember("schema_url") ? std::optional<std::string>{details["schema_url"].asString()}
                                                          : std::nullopt,
        };
        break;
    case domain::endpoint::EndpointType::Soap:
        endpoint.details = domain::endpoint::SoapDetails{
            .wsdl_url = details["wsdl_url"].asString(),
            .operation = details["operation"].asString(),
            .request_xml = details["request_xml"].asString(),
        };
        break;
    case domain::endpoint::EndpointType::Websocket:
        endpoint.details = domain::endpoint::WebsocketDetails{
            .connection_url = details["connection_url"].asString(),
            .protocols = details.isMember("protocols")
                             ? nlohmann::json::parse(Json::writeString(Json::StreamWriterBuilder{}, details["protocols"])).get<std::vector<std::string>>()
                             : std::vector<std::string>{},
            .sample_messages = details.isMember("sample_messages")
                                   ? nlohmann::json::parse(Json::writeString(Json::StreamWriterBuilder{}, details["sample_messages"]))
                                   : nlohmann::json::array(),
        };
        break;
    case domain::endpoint::EndpointType::Rest:
    default:
        endpoint.details = domain::endpoint::RestDetails{
            .method = domain::endpoint::httpMethodFromString(details["method"].asString()),
            .url_path = details["url_path"].asString(),
            .query_params = details.isMember("query_params")
                                ? nlohmann::json::parse(Json::writeString(Json::StreamWriterBuilder{}, details["query_params"]))
                                : nlohmann::json::array(),
            .headers = details.isMember("headers")
                           ? nlohmann::json::parse(Json::writeString(Json::StreamWriterBuilder{}, details["headers"]))
                           : nlohmann::json::array(),
            .body = details.isMember("body")
                        ? nlohmann::json::parse(Json::writeString(Json::StreamWriterBuilder{}, details["body"]))
                        : nlohmann::json::object(),
            .body_raw = details.isMember("body_raw") ? details["body_raw"].asString() : std::string{},
        };
        break;
    }

    return endpoint;
}

auto EndpointController::endpointToJson(const domain::endpoint::Endpoint& endpoint) -> Json::Value
{
    Json::Value payload;
    payload["id"] = endpoint.id;
    payload["project_id"] = endpoint.project_id;
    payload["type"] = domain::endpoint::endpointTypeToString(endpoint.type);
    payload["status"] = endpoint.status;
    payload["created_by"] = endpoint.created_by;
    payload["name"] = toJsonValue(endpoint.name);
    payload["description"] = toJsonValue(endpoint.description);

    Json::Value details;
    std::visit(
        [&](const auto& current) {
            using Details = std::decay_t<decltype(current)>;

            if constexpr (std::is_same_v<Details, domain::endpoint::RestDetails>)
            {
                details["method"] = domain::endpoint::httpMethodToString(current.method);
                details["url_path"] = current.url_path;
                details["query_params"] = toJsonValue(current.query_params);
                details["headers"] = toJsonValue(current.headers);
                details["body"] = toJsonValue(current.body);
                details["body_raw"] = current.body_raw;
            }
            else if constexpr (std::is_same_v<Details, domain::endpoint::GraphqlDetails>)
            {
                details["query"] = current.query;
                details["variables"] = toJsonValue(current.variables);
                if (current.schema_url.has_value())
                {
                    details["schema_url"] = current.schema_url.value();
                }
            }
            else if constexpr (std::is_same_v<Details, domain::endpoint::SoapDetails>)
            {
                details["wsdl_url"] = current.wsdl_url;
                details["operation"] = current.operation;
                details["request_xml"] = current.request_xml;
            }
            else if constexpr (std::is_same_v<Details, domain::endpoint::WebsocketDetails>)
            {
                details["connection_url"] = current.connection_url;
                details["protocols"] = toJsonValue(nlohmann::json(current.protocols));
                details["sample_messages"] = toJsonValue(current.sample_messages);
            }
        },
        endpoint.details);

    payload["details"] = details;
    return payload;
}

void EndpointController::list(const drogon::HttpRequestPtr& request,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              std::string projectId)
{
    if (!service_)
    {
        callback(jsonError("endpoint service is not configured", drogon::k503ServiceUnavailable));
        return;
    }

    Json::Value payload(Json::arrayValue);
    for (const auto& endpoint : service_->getProjectEndpoints(projectId, request->getHeader("X-User-ID")))
    {
        payload.append(endpointToJson(endpoint));
    }

    callback(drogon::HttpResponse::newHttpJsonResponse(payload));
}

void EndpointController::create(const drogon::HttpRequestPtr& request,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                std::string projectId)
{
    if (!service_)
    {
        callback(jsonError("endpoint service is not configured", drogon::k503ServiceUnavailable));
        return;
    }

    const auto json = request->getJsonObject();
    if (!json)
    {
        callback(jsonError("invalid json", drogon::k400BadRequest));
        return;
    }

    const auto endpointId = service_->createEndpoint(
        projectId,
        parseEndpointPayload(*json, projectId, request->getHeader("X-User-ID")),
        request->getHeader("X-User-ID"));

    if (endpointId.empty())
    {
        callback(jsonError("unable to create endpoint", drogon::k403Forbidden));
        return;
    }

    Json::Value payload;
    payload["id"] = endpointId;
    callback(drogon::HttpResponse::newHttpJsonResponse(payload));
}

void EndpointController::get(const drogon::HttpRequestPtr& request,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                             std::string endpointId)
{
    if (!service_)
    {
        callback(jsonError("endpoint service is not configured", drogon::k503ServiceUnavailable));
        return;
    }

    const auto endpoint = service_->getEndpoint(endpointId, request->getHeader("X-User-ID"));
    if (!endpoint.has_value())
    {
        callback(jsonError("endpoint not found", drogon::k404NotFound));
        return;
    }

    callback(drogon::HttpResponse::newHttpJsonResponse(endpointToJson(endpoint.value())));
}

void EndpointController::update(const drogon::HttpRequestPtr& request,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                std::string endpointId)
{
    if (!service_)
    {
        callback(jsonError("endpoint service is not configured", drogon::k503ServiceUnavailable));
        return;
    }

    const auto current = service_->getEndpoint(endpointId, request->getHeader("X-User-ID"));
    if (!current.has_value())
    {
        callback(jsonError("endpoint not found", drogon::k404NotFound));
        return;
    }

    const auto json = request->getJsonObject();
    if (!json)
    {
        callback(jsonError("invalid json", drogon::k400BadRequest));
        return;
    }

    const auto success = service_->updateEndpoint(
        endpointId,
        parseEndpointPayload(*json, current->project_id, request->getHeader("X-User-ID")),
        request->getHeader("X-User-ID"));
    Json::Value payload;
    payload["success"] = success;
    auto response = drogon::HttpResponse::newHttpJsonResponse(payload);
    response->setStatusCode(success ? drogon::k200OK : drogon::k403Forbidden);
    callback(response);
}

void EndpointController::remove(const drogon::HttpRequestPtr& request,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                std::string endpointId)
{
    if (!service_)
    {
        callback(jsonError("endpoint service is not configured", drogon::k503ServiceUnavailable));
        return;
    }

    const auto success = service_->deleteEndpoint(endpointId, request->getHeader("X-User-ID"));
    Json::Value payload;
    payload["success"] = success;
    auto response = drogon::HttpResponse::newHttpJsonResponse(payload);
    response->setStatusCode(success ? drogon::k200OK : drogon::k403Forbidden);
    callback(response);
}

void EndpointController::publish(const drogon::HttpRequestPtr& request,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 std::string endpointId)
{
    if (!service_)
    {
        callback(jsonError("endpoint service is not configured", drogon::k503ServiceUnavailable));
        return;
    }

    const auto success = service_->publishEndpoint(endpointId, request->getHeader("X-User-ID"));
    Json::Value payload;
    payload["success"] = success;
    auto response = drogon::HttpResponse::newHttpJsonResponse(payload);
    response->setStatusCode(success ? drogon::k200OK : drogon::k403Forbidden);
    callback(response);
}
} // namespace nexustal::controllers
