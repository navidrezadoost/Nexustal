#pragma once

#include <drogon/nosql/RedisClient.h>

#include "domain/notification_port.hpp"

namespace nexustal::infrastructure::adapters
{
class RedisNotificationService : public domain::notification::INotificationService
{
public:
    explicit RedisNotificationService(drogon::nosql::RedisClientPtr redis);

    void sendNotification(const domain::notification::Notification& notification) override;

private:
    drogon::nosql::RedisClientPtr redis_;
};
} // namespace nexustal::infrastructure::adapters
