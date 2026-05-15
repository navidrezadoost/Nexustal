#include "infrastructure/adapters/jwt_token_service.hpp"

#include <chrono>
#include <drogon/drogon.h>
#include <stdexcept>

#if __has_include(<jwt-cpp/jwt.h>)
#include <jwt-cpp/jwt.h>
#define NEXUSTAL_HAS_JWT_CPP 1
#else
#define NEXUSTAL_HAS_JWT_CPP 0
#endif

#include "domain/common.hpp"

namespace nexustal::infrastructure::adapters
{
JwtTokenService::JwtTokenService(std::string secret,
                                 drogon::nosql::RedisClientPtr cache,
                                 std::shared_ptr<domain::identity::IUserRepository> userRepository)
    : secret_(std::move(secret))
    , cache_(std::move(cache))
    , userRepository_(std::move(userRepository))
{
}

auto JwtTokenService::createTokens(const domain::identity::User& user) -> domain::auth::TokenPair
{
#if NEXUSTAL_HAS_JWT_CPP
    if (cache_)
    {
        const auto cacheKey = cacheKeyFor(user.id);
        cache_->execCommandAsync(
            [](const drogon::nosql::RedisResult&) {},
            [](const std::exception& exception) {
                LOG_WARN << "Token cache warm-up failed: " << exception.what();
            },
            "SETEX %s %d %s",
            cacheKey.c_str(),
            CacheTtlSeconds,
            user.token_version.c_str());
    }

    const auto now = std::chrono::system_clock::now();
    const auto expiration = now + std::chrono::seconds(AccessTokenTtlSeconds);

    const auto token = jwt::create()
                           .set_issuer("nexustal")
                           .set_subject(user.id)
                           .set_payload_claim("ver", jwt::claim(user.token_version))
                           .set_payload_claim("jti", jwt::claim(domain::generate_uuid()))
                           .set_issued_at(now)
                           .set_expires_at(expiration)
                           .sign(jwt::algorithm::hs256{secret_});

    return domain::auth::TokenPair{.access_token = token};
#else
    (void)user;
    throw std::runtime_error("jwt-cpp dependency is not available");
#endif
}

auto JwtTokenService::validateAccessToken(const std::string& token)
    -> std::optional<domain::auth::TokenClaims>
{
#if NEXUSTAL_HAS_JWT_CPP
    try
    {
        const auto decoded = jwt::decode(token);
        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{secret_})
            .with_issuer("nexustal")
            .verify(decoded);

        const auto userId = decoded.get_subject();
        const auto version = decoded.get_payload_claim("ver").as_string();
        const auto currentVersion = fetchCurrentVersion(userId);
        if (!currentVersion.has_value() || currentVersion.value() != version)
        {
            return std::nullopt;
        }

        return domain::auth::TokenClaims{
            .user_id = userId,
            .token_version = version,
        };
    }
    catch (const std::exception& exception)
    {
        LOG_WARN << "Token validation failed: " << exception.what();
        return std::nullopt;
    }
#else
    (void)token;
    return std::nullopt;
#endif
}

void JwtTokenService::rotateVersion(const domain::UUID& userId)
{
    const auto user = userRepository_->findById(userId);
    if (!user.has_value())
    {
        return;
    }

    auto rotated = user.value();
    rotated.token_version = domain::generate_uuid();
    if (!userRepository_->update(rotated))
    {
        return;
    }

    if (cache_)
    {
        const auto cacheKey = cacheKeyFor(userId);
        cache_->execCommandAsync(
            [](const drogon::nosql::RedisResult&) {},
            [](const std::exception& exception) {
                LOG_WARN << "Token cache invalidation failed: " << exception.what();
            },
            "DEL %s",
            cacheKey.c_str());
    }
}

auto JwtTokenService::fetchCurrentVersion(const domain::UUID& userId) -> std::optional<domain::UUID>
{
    const auto cacheKey = cacheKeyFor(userId);

    if (cache_)
    {
        try
        {
            const auto cached = cache_->execCommandSync("GET %s", cacheKey.c_str());
            return cached.asString();
        }
        catch (const std::exception&)
        {
        }
    }

    const auto user = userRepository_->findById(userId);
    if (!user.has_value())
    {
        return std::nullopt;
    }

    if (cache_)
    {
        cache_->execCommandAsync(
            [](const drogon::nosql::RedisResult&) {},
            [](const std::exception& exception) {
                LOG_WARN << "Token cache populate failed: " << exception.what();
            },
            "SETEX %s %d %s",
            cacheKey.c_str(),
            CacheTtlSeconds,
            user->token_version.c_str());
    }

    return user->token_version;
}

auto JwtTokenService::cacheKeyFor(const domain::UUID& userId) -> std::string
{
    return "user_token_version:" + userId;
}
} // namespace nexustal::infrastructure::adapters
