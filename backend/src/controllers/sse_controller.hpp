#pragma once

#include <drogon/HttpController.h>
#include <drogon/nosql/RedisClient.h>

namespace nexustal::controllers
{
class SseController : public drogon::HttpController<SseController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(SseController::stream, "/api/notifications/stream", drogon::Get, "JwtAuthFilter");
    METHOD_LIST_END

    static void configure(drogon::nosql::RedisClientPtr redis);

    void stream(const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    inline static drogon::nosql::RedisClientPtr redisClient_{};
};
} // namespace nexustal::controllers
