#pragma once

#include <memory>

#include <drogon/HttpController.h>

#include "application/invitation_service.hpp"

namespace nexustal::controllers
{
class InvitationController : public drogon::HttpController<InvitationController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(InvitationController::send, "/api/invitations", drogon::Post, "JwtAuthFilter");
    ADD_METHOD_TO(InvitationController::getPending, "/api/invitations/pending", drogon::Get, "JwtAuthFilter");
    ADD_METHOD_TO(InvitationController::accept, "/api/invitations/{1}/accept", drogon::Put, "JwtAuthFilter");
    ADD_METHOD_TO(InvitationController::reject, "/api/invitations/{1}/reject", drogon::Put, "JwtAuthFilter");
    METHOD_LIST_END

    static void configure(std::shared_ptr<application::InvitationService> service);

    void send(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getPending(const drogon::HttpRequestPtr& request,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void accept(const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string invitationId);
    void reject(const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string invitationId);

private:
    inline static std::shared_ptr<application::InvitationService> service_{};
};
} // namespace nexustal::controllers
