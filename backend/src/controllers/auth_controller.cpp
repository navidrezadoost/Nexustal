#include "controllers/auth_controller.hpp"

#include <nlohmann/json.hpp>

namespace nexustal::controllers
{
namespace
{
auto unavailableResponse() -> drogon::HttpResponsePtr
{
    auto response = drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"error", "auth service is not configured"}});
    response->setStatusCode(drogon::k503ServiceUnavailable);
    return response;
}

auto unauthorizedResponse() -> drogon::HttpResponsePtr
{
    auto response = drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"error", "unauthorized"}});
    response->setStatusCode(drogon::k401Unauthorized);
    return response;
}
} // namespace

void AuthController::configure(std::shared_ptr<application::AuthService> service)
{
    service_ = std::move(service);
}

void AuthController::login(const drogon::HttpRequestPtr& request,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    if (!service_)
    {
        callback(unavailableResponse());
        return;
    }

    const auto json = request->getJsonObject();
    if (!json)
    {
        auto response = drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"error", "invalid json"}});
        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    const auto result = service_->login((*json)["username"].asString(), (*json)["password"].asString());
    if (!result.success)
    {
        callback(unauthorizedResponse());
        return;
    }

    callback(drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"access_token", result.tokens.access_token}}));
}

void AuthController::logout(const drogon::HttpRequestPtr& request,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    if (!service_)
    {
        callback(unavailableResponse());
        return;
    }

    const auto userId = request->getHeader("X-User-ID");
    if (userId.empty() || !service_->logout(userId))
    {
        callback(unauthorizedResponse());
        return;
    }

    callback(drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"success", true}}));
}
} // namespace nexustal::controllers
