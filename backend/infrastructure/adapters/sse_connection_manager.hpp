#pragma once

#include <drogon/drogon.h>
#include <drogon/nosql/RedisClient.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace nexustal::infrastructure::adapters
{
class SseConnectionManager
{
public:
    explicit SseConnectionManager(drogon::nosql::RedisClientPtr redis);
    ~SseConnectionManager();

    void addConnection(const std::string& user_id, drogon::ResponseStreamPtr stream);
    void removeConnection(const std::string& user_id, const drogon::ResponseStreamPtr& stream);
    void sendToUser(const std::string& user_id, const std::string& sse_data);

private:
    void startHeartbeat();

    drogon::nosql::RedisClientPtr redis_;
    std::mutex mutex_;
    std::unordered_map<std::string, std::vector<drogon::ResponseStreamPtr>> connections_;
    std::shared_ptr<drogon::nosql::RedisSubscriber> global_subscriber_;
};
} // namespace nexustal::infrastructure::adapters
