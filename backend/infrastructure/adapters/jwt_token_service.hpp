#pragma once

#include <memory>
#include <string>

#include <drogon/nosql/RedisClient.h>

#include "domain/auth/token_service.hpp"
#include "domain/identity_repository.hpp"

namespace nexustal::infrastructure::adapters
{
class JwtTokenService : public domain::auth::ITokenService
{
public:
    static constexpr int AccessTokenTtlSeconds = 900;
    static constexpr int CacheTtlSeconds = 3600;

    JwtTokenService(std::string secret,
                    drogon::nosql::RedisClientPtr cache,
                    std::shared_ptr<domain::identity::IUserRepository> userRepository);

    domain::auth::TokenPair createTokens(const domain::identity::User& user) override;
    std::optional<domain::auth::TokenClaims> validateAccessToken(const std::string& token) override;
    void rotateVersion(const domain::UUID& userId) override;

private:
    [[nodiscard]] auto fetchCurrentVersion(const domain::UUID& userId) -> std::optional<domain::UUID>;
    static auto cacheKeyFor(const domain::UUID& userId) -> std::string;

    std::string secret_;
    drogon::nosql::RedisClientPtr cache_;
    std::shared_ptr<domain::identity::IUserRepository> userRepository_;
};
} // namespace nexustal::infrastructure::adapters
