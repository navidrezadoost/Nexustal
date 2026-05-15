#pragma once

#include <memory>

#include <drogon/HttpController.h>

#include "application/auth_service.hpp"

namespace nexustal::controllers
{
class AuthController : public drogon::HttpController<AuthController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::login, "/api/login", drogon::Post);
    ADD_METHOD_TO(AuthController::logout, "/api/logout", drogon::Post, "JwtAuthFilter");
    METHOD_LIST_END

    static void configure(std::shared_ptr<application::AuthService> service);

    void login(const drogon::HttpRequestPtr& request,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void logout(const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    inline static std::shared_ptr<application::AuthService> service_{};
};
} // namespace nexustal::controllers
