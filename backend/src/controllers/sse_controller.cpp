#include "controllers/sse_controller.hpp"

#include <drogon/drogon.h>

namespace nexustal::controllers
{
void SseController::configure(std::shared_ptr<infrastructure::adapters::SseConnectionManager> connection_manager)
{
    connection_manager_ = std::move(connection_manager);
}

void SseController::stream(const drogon::HttpRequestPtr& request,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto userId = request->getHeader("X-User-ID");
    if (userId.empty())
    {
        auto response = drogon::HttpResponse::newHttpResponse();
        response->setStatusCode(drogon::k401Unauthorized);
        callback(response);
        return;
    }

    auto response = drogon::HttpResponse::newAsyncStreamResponse(
        [userId](drogon::ResponseStreamPtr stream) {
            if (!connection_manager_)
            {
                stream->send("event: warning\ndata: {\"message\":\"redis client not configured\"}\n\n");
                return;
            }

            connection_manager_->addConnection(userId, stream);
        });

    response->setContentTypeCodeAndCustomString(drogon::CT_CUSTOM, "text/event-stream");
    response->setHeader("Cache-Control", "no-cache");
    response->setHeader("Connection", "keep-alive");
    response->setHeader("X-Accel-Buffering", "no");
    callback(response);
}
} // namespace nexustal::controllers
