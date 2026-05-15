#pragma once

#include <memory>

#include <drogon/HttpController.h>

#include "application/wizard_service.hpp"

namespace nexustal::controllers
{
class WizardController : public drogon::HttpController<WizardController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(WizardController::status, "/api/wizard/status", drogon::Get);
    ADD_METHOD_TO(WizardController::configureDb, "/api/wizard/database", drogon::Post);
    ADD_METHOD_TO(WizardController::createAdmin, "/api/wizard/admin", drogon::Post);
    ADD_METHOD_TO(WizardController::registerCompany, "/api/wizard/company", drogon::Post);
    METHOD_LIST_END

    static void configure(std::shared_ptr<application::WizardService> service);

    void status(const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void configureDb(const drogon::HttpRequestPtr& request,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void createAdmin(const drogon::HttpRequestPtr& request,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void registerCompany(const drogon::HttpRequestPtr& request,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    inline static std::shared_ptr<application::WizardService> service_{};
};
} // namespace nexustal::controllers
