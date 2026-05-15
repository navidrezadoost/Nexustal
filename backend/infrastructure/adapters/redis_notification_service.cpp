#include "infrastructure/adapters/redis_notification_service.hpp"

#include <nlohmann/json.hpp>

#include "infrastructure/adapters/timestamp_codec.hpp"

namespace nexustal::infrastructure::adapters
{
RedisNotificationService::RedisNotificationService(drogon::nosql::RedisClientPtr redis)
    : redis_(std::move(redis))
{
}

void RedisNotificationService::sendNotification(const domain::notification::Notification& notification)
{
    if (!redis_)
    {
        return;
    }

    nlohmann::json payload = {
        {"id", notification.id},
        {"user_id", notification.user_id},
        {"title", notification.title},
        {"message", notification.message},
        {"type", notification.type},
        {"is_read", notification.is_read},
        {"created_at", formatTimestamp(notification.created_at)},
    };

    const auto channel = "user:" + notification.user_id + ":notifications";
    redis_->execCommandAsync(
        [](const drogon::nosql::RedisResult&) {},
        [](const std::exception& exception) {
            LOG_ERROR << "RedisNotificationService publish failed: " << exception.what();
        },
        "PUBLISH %s %s",
        channel.c_str(),
        payload.dump().c_str());
}
} // namespace nexustal::infrastructure::adapters
