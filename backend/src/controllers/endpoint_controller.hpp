#pragma once

#include <memory>

#include <drogon/HttpController.h>

#include "application/endpoint_service.hpp"

namespace nexustal::controllers
{
class EndpointController : public drogon::HttpController<EndpointController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(EndpointController::list, "/api/projects/{1}/endpoints", drogon::Get, "JwtAuthFilter");
    ADD_METHOD_TO(EndpointController::create, "/api/projects/{1}/endpoints", drogon::Post, "JwtAuthFilter");
    ADD_METHOD_TO(EndpointController::get, "/api/endpoints/{1}", drogon::Get, "JwtAuthFilter");
    ADD_METHOD_TO(EndpointController::update, "/api/endpoints/{1}", drogon::Put, "JwtAuthFilter");
    ADD_METHOD_TO(EndpointController::remove, "/api/endpoints/{1}", drogon::Delete, "JwtAuthFilter");
    ADD_METHOD_TO(EndpointController::publish, "/api/endpoints/{1}/publish", drogon::Post, "JwtAuthFilter");
    METHOD_LIST_END

    static void configure(std::shared_ptr<application::EndpointService> service);

    void list(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              std::string projectId);
    void create(const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string projectId);
    void get(const drogon::HttpRequestPtr& request,
             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
             std::string endpointId);
    void update(const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string endpointId);
    void remove(const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string endpointId);
    void publish(const drogon::HttpRequestPtr& request,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                 std::string endpointId);

private:
    [[nodiscard]] static auto parseEndpointPayload(const Json::Value& json,
                                                   const domain::UUID& projectId,
                                                   const domain::UUID& userId)
        -> domain::endpoint::Endpoint;
    [[nodiscard]] static auto endpointToJson(const domain::endpoint::Endpoint& endpoint) -> Json::Value;

    inline static std::shared_ptr<application::EndpointService> service_{};
};
} // namespace nexustal::controllers
