#include "infrastructure/adapters/sse_connection_manager.hpp"

#include <algorithm>

namespace nexustal::infrastructure::adapters
{
SseConnectionManager::SseConnectionManager(drogon::nosql::RedisClientPtr redis)
    : redis_(std::move(redis))
{
    if (redis_)
    {
        global_subscriber_ = redis_->newSubscriber();
        global_subscriber_->psubscribe(
            "user:*:notifications",
            [this](const std::string&, const std::string& channel, const std::string& message) {
                constexpr std::string_view Prefix{"user:"};
                constexpr std::string_view Suffix{":notifications"};

                if (!channel.starts_with(Prefix.data()) || !channel.ends_with(Suffix.data()))
                {
                    return;
                }

                const auto user_id = channel.substr(Prefix.size(), channel.size() - Prefix.size() - Suffix.size());
                sendToUser(user_id, "data: " + message + "\n\n");
            },
            [](const std::exception& error) {
                LOG_ERROR << "SSE global subscriber error: " << error.what();
            });
    }

    startHeartbeat();
}

SseConnectionManager::~SseConnectionManager()
{
    if (global_subscriber_)
    {
        global_subscriber_->punsubscribe("user:*:notifications");
    }
}

void SseConnectionManager::startHeartbeat()
{
    drogon::app().getLoop()->runEvery(15.0, [this]() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [user_id, streams] : connections_)
        {
            (void)user_id;
            for (auto& stream : streams)
            {
                try
                {
                    stream->send(": heartbeat\n\n");
                }
                catch (const std::exception& error)
                {
                    LOG_WARN << "Failed to send SSE heartbeat: " << error.what();
                }
            }
        }
    });
}

void SseConnectionManager::addConnection(const std::string& user_id, drogon::ResponseStreamPtr stream)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_[user_id].push_back(stream);
    }

    stream->setCloseHandler([this, user_id, weakStream = std::weak_ptr<drogon::ResponseStream>(stream)]() {
        if (const auto shared = weakStream.lock())
        {
            removeConnection(user_id, shared);
        }
    });

    stream->send("event: ready\ndata: {\"status\":\"connected\"}\n\n");
}

void SseConnectionManager::removeConnection(const std::string& user_id, const drogon::ResponseStreamPtr& stream)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = connections_.find(user_id);
    if (iterator == connections_.end())
    {
        return;
    }

    auto& streams = iterator->second;
    streams.erase(std::remove(streams.begin(), streams.end(), stream), streams.end());
    if (streams.empty())
    {
        connections_.erase(iterator);
    }
}

void SseConnectionManager::sendToUser(const std::string& user_id, const std::string& sse_data)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = connections_.find(user_id);
    if (iterator == connections_.end())
    {
        return;
    }

    for (auto& stream : iterator->second)
    {
        try
        {
            stream->send(sse_data);
        }
        catch (const std::exception& error)
        {
            LOG_WARN << "Failed to send SSE to user " << user_id << ": " << error.what();
        }
    }
}
} // namespace nexustal::infrastructure::adapters
