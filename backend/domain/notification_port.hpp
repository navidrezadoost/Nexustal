#pragma once

#include <string>

#include "domain/common.hpp"

namespace nexustal::domain::notification
{
using ::nexustal::domain::Timestamp;
using ::nexustal::domain::UUID;

struct Notification
{
    UUID id;
    UUID user_id;
    std::string title;
    std::string message;
    std::string type;
    bool is_read = false;
    Timestamp created_at;
};

class INotificationService
{
public:
    virtual ~INotificationService() = default;

    virtual void sendNotification(const Notification& notification) = 0;
};
} // namespace nexustal::domain::notification
