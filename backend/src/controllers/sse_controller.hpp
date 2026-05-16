#pragma once

#include <drogon/HttpController.h>

#include <memory>

#include "infrastructure/adapters/sse_connection_manager.hpp"

namespace nexustal::controllers
{
class SseController : public drogon::HttpController<SseController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(SseController::stream, "/api/notifications/stream", drogon::Get, "JwtAuthFilter");
    METHOD_LIST_END

    static void configure(std::shared_ptr<infrastructure::adapters::SseConnectionManager> connection_manager);

    void stream(const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    inline static std::shared_ptr<infrastructure::adapters::SseConnectionManager> connection_manager_{};
};
} // namespace nexustal::controllers
