#include "controllers/invitation_controller.hpp"

#include <nlohmann/json.hpp>

#include "domain/auth/capabilities.hpp"

namespace nexustal::controllers
{
namespace
{
auto serviceUnavailable() -> drogon::HttpResponsePtr
{
    auto response = drogon::HttpResponse::newHttpJsonResponse(
        nlohmann::json{{"error", "invitation service is not configured"}});
    response->setStatusCode(drogon::k503ServiceUnavailable);
    return response;
}

auto forbiddenResponse() -> drogon::HttpResponsePtr
{
    auto response = drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"error", "forbidden"}});
    response->setStatusCode(drogon::k403Forbidden);
    return response;
}

auto badRequest(const std::string& message) -> drogon::HttpResponsePtr
{
    auto response = drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"error", message}});
    response->setStatusCode(drogon::k400BadRequest);
    return response;
}

auto invitationToJson(const nexustal::domain::identity::Invitation& invitation) -> nlohmann::json
{
    return nlohmann::json{{"id", invitation.id},
                          {"from_user_id", invitation.from_user_id},
                          {"to_user_id", invitation.to_user_id},
                          {"project_id", invitation.project_id.value_or("")},
                          {"team_id", invitation.team_id.value_or("")},
                          {"role", std::string{nexustal::domain::auth::toString(invitation.role)}}};
}
} // namespace

void InvitationController::configure(std::shared_ptr<application::InvitationService> service)
{
    service_ = std::move(service);
}

void InvitationController::send(const drogon::HttpRequestPtr& request,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    if (!service_)
    {
        callback(serviceUnavailable());
        return;
    }

    const auto callerRole = domain::auth::roleFromString(request->getHeader("X-User-Role"));
    if (!domain::auth::hasCapability(callerRole, domain::auth::Capability::ManageTeam))
    {
        callback(forbiddenResponse());
        return;
    }

    const auto json = request->getJsonObject();
    if (!json)
    {
        callback(badRequest("invalid json"));
        return;
    }

    const auto createdId = service_->sendInvitation(
        request->getHeader("X-User-ID"),
        (*json)["to_user_id"].asString(),
        (*json)["project_id"].asString(),
        domain::auth::roleFromString((*json)["role"].asString()));

    if (createdId.empty())
    {
        callback(badRequest("unable to create invitation"));
        return;
    }

    callback(drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"id", createdId}}));
}

void InvitationController::getPending(const drogon::HttpRequestPtr& request,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    if (!service_)
    {
        callback(serviceUnavailable());
        return;
    }

    nlohmann::json payload = nlohmann::json::array();
    for (const auto& invitation : service_->getPendingForUser(request->getHeader("X-User-ID")))
    {
        payload.push_back(invitationToJson(invitation));
    }

    callback(drogon::HttpResponse::newHttpJsonResponse(payload));
}

void InvitationController::accept(const drogon::HttpRequestPtr& request,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                  std::string invitationId)
{
    if (!service_)
    {
        callback(serviceUnavailable());
        return;
    }

    const auto success = service_->accept(invitationId, request->getHeader("X-User-ID"));
    auto response = drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"success", success}});
    response->setStatusCode(success ? drogon::k200OK : drogon::k404NotFound);
    callback(response);
}

void InvitationController::reject(const drogon::HttpRequestPtr& request,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                  std::string invitationId)
{
    if (!service_)
    {
        callback(serviceUnavailable());
        return;
    }

    const auto success = service_->reject(invitationId, request->getHeader("X-User-ID"));
    auto response = drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"success", success}});
    response->setStatusCode(success ? drogon::k200OK : drogon::k404NotFound);
    callback(response);
}
} // namespace nexustal::controllers
