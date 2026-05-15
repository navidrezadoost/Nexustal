#pragma once

#include <memory>

#include <drogon/HttpFilter.h>
#include <drogon/nosql/RedisClient.h>

#include "application/auth_service.hpp"
#include "domain/identity_repository.hpp"

namespace nexustal::filters
{
class JwtAuthFilter : public drogon::HttpFilter<JwtAuthFilter>
{
public:
    static void configure(std::shared_ptr<application::AuthService> service,
                          std::shared_ptr<domain::identity::IUserRepository> userRepository,
                          drogon::nosql::RedisClientPtr cache);

    void doFilter(const drogon::HttpRequestPtr& request,
                  drogon::FilterCallback&& failedCallback,
                  drogon::FilterChainCallback&& passCallback) override;

private:
    [[nodiscard]] static auto resolveRole(const domain::UUID& userId) -> std::string;
    static auto roleCacheKey(const domain::UUID& userId) -> std::string;

    inline static std::shared_ptr<application::AuthService> service_{};
    inline static std::shared_ptr<domain::identity::IUserRepository> userRepository_{};
    inline static drogon::nosql::RedisClientPtr cache_{};
};
} // namespace nexustal::filters
