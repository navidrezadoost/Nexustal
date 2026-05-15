#include "controllers/sse_controller.hpp"

#include <drogon/drogon.h>

namespace nexustal::controllers
{
void SseController::configure(drogon::nosql::RedisClientPtr redis)
{
    redisClient_ = std::move(redis);
}

void SseController::stream(const drogon::HttpRequestPtr& request,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto userId = request->getHeader("X-User-ID");
    auto response = drogon::HttpResponse::newAsyncStreamResponse(
        [userId](drogon::ResponseStreamPtr stream) {
            stream->send("event: ready\ndata: {\"status\":\"connected\"}\n\n");

            if (!redisClient_)
            {
                stream->send("event: warning\ndata: {\"message\":\"redis client not configured\"}\n\n");
                return;
            }

            const auto channel = "user:" + userId + ":notifications";
            redisClient_->execCommandAsync(
                [stream](const drogon::nosql::RedisResult&) {
                    stream->send(": subscription registered\n\n");
                },
                [stream](const std::exception& exception) {
                    stream->send("event: error\ndata: {\"message\":\"" + std::string{exception.what()} + "\"}\n\n");
                },
                "SUBSCRIBE %s",
                channel.c_str());
        });

    response->setContentTypeCodeAndCustomString(drogon::CT_CUSTOM, "text/event-stream");
    response->setHeader("Cache-Control", "no-cache");
    response->setHeader("Connection", "keep-alive");
    callback(response);
}
} // namespace nexustal::controllers
