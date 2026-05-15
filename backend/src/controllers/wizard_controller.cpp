#include "controllers/wizard_controller.hpp"

#include <drogon/drogon.h>
#include <nlohmann/json.hpp>

namespace nexustal::controllers
{
namespace
{
auto serviceUnavailableResponse() -> drogon::HttpResponsePtr
{
    auto response = drogon::HttpResponse::newHttpJsonResponse(
        nlohmann::json{{"error", "wizard service is not configured"}});
    response->setStatusCode(drogon::k503ServiceUnavailable);
    return response;
}
} // namespace

void WizardController::configure(std::shared_ptr<application::WizardService> service)
{
    service_ = std::move(service);
}

void WizardController::status(const drogon::HttpRequestPtr& request,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    (void)request;
    if (!service_)
    {
        callback(serviceUnavailableResponse());
        return;
    }

    const auto status = service_->getStatus();
    const auto payload = nlohmann::json{{"step", static_cast<int>(status.current_step)},
                                        {"is_complete", status.is_complete}};
    callback(drogon::HttpResponse::newHttpJsonResponse(payload));
}

void WizardController::configureDb(const drogon::HttpRequestPtr& request,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    if (!service_)
    {
        callback(serviceUnavailableResponse());
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

    const domain::wizard::DatabaseConfig config{
        .host = (*json)["host"].asString(),
        .port = (*json).isMember("port") ? (*json)["port"].asInt() : 5432,
        .dbname = (*json)["dbname"].asString(),
        .user = (*json)["user"].asString(),
        .password = (*json)["password"].asString(),
    };

    const auto success = service_->configureDatabase(config);
    auto response = drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"success", success}});
    response->setStatusCode(success ? drogon::k200OK : drogon::k400BadRequest);
    callback(response);
}

void WizardController::createAdmin(const drogon::HttpRequestPtr& request,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    if (!service_)
    {
        callback(serviceUnavailableResponse());
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

    const domain::wizard::AdminCredentials credentials{
        .username = (*json)["username"].asString(),
        .password = (*json)["password"].asString(),
    };

    const auto success = service_->createAdmin(credentials);
    auto response = drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"success", success}});
    response->setStatusCode(success ? drogon::k200OK : drogon::k400BadRequest);
    callback(response);
}

void WizardController::registerCompany(const drogon::HttpRequestPtr& request,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    if (!service_)
    {
        callback(serviceUnavailableResponse());
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

    const domain::wizard::CompanyProfile profile{
        .name = (*json)["name"].asString(),
        .activity_scope = (*json)["activity_scope"].asString(),
        .installation_id = (*json)["installation_id"].asString(),
    };

    const auto success = service_->registerCompany(profile);
    auto response = drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"success", success}});
    response->setStatusCode(success ? drogon::k200OK : drogon::k400BadRequest);
    callback(response);
}
} // namespace nexustal::controllers
