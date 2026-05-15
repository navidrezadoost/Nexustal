#include "filters/jwt_auth_filter.hpp"

#include <nlohmann/json.hpp>

namespace nexustal::filters
{
void JwtAuthFilter::configure(std::shared_ptr<application::AuthService> service,
                              std::shared_ptr<domain::identity::IUserRepository> userRepository,
                              drogon::nosql::RedisClientPtr cache)
{
    service_ = std::move(service);
    userRepository_ = std::move(userRepository);
    cache_ = std::move(cache);
}

auto JwtAuthFilter::roleCacheKey(const domain::UUID& userId) -> std::string
{
    return "user_role:" + userId;
}

auto JwtAuthFilter::resolveRole(const domain::UUID& userId) -> std::string
{
    if (cache_)
    {
        try
        {
            const auto cacheKey = roleCacheKey(userId);
            const auto cached = cache_->execCommandSync("GET %s", cacheKey.c_str());
            const auto cachedRole = cached.asString();
            if (!cachedRole.empty())
            {
                return cachedRole;
            }
        }
        catch (const std::exception&)
        {
        }
    }

    if (!userRepository_)
    {
        return "viewer";
    }

    const auto user = userRepository_->findById(userId);
    const auto resolvedRole = user.has_value() && user->is_admin ? std::string{"admin"} : std::string{"viewer"};

    if (cache_)
    {
        const auto cacheKey = roleCacheKey(userId);
        cache_->execCommandAsync(
            [](const drogon::nosql::RedisResult&) {},
            [](const std::exception&) {},
            "SETEX %s %d %s",
            cacheKey.c_str(),
            3600,
            resolvedRole.c_str());
    }

    return resolvedRole;
}

void JwtAuthFilter::doFilter(const drogon::HttpRequestPtr& request,
                             drogon::FilterCallback&& failedCallback,
                             drogon::FilterChainCallback&& passCallback)
{
    if (!service_)
    {
        auto response = drogon::HttpResponse::newHttpJsonResponse(
            nlohmann::json{{"error", "authentication service is not configured"}});
        response->setStatusCode(drogon::k503ServiceUnavailable);
        failedCallback(response);
        return;
    }

    const auto authorization = request->getHeader("Authorization");
    if (authorization.rfind("Bearer ", 0) != 0)
    {
        auto response = drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"error", "missing bearer token"}});
        response->setStatusCode(drogon::k401Unauthorized);
        failedCallback(response);
        return;
    }

    const auto claims = service_->validateAccessToken(authorization.substr(7));
    if (!claims.has_value())
    {
        auto response = drogon::HttpResponse::newHttpJsonResponse(nlohmann::json{{"error", "invalid token"}});
        response->setStatusCode(drogon::k401Unauthorized);
        failedCallback(response);
        return;
    }

    request->addHeader("X-User-ID", claims->user_id);
    request->addHeader("X-Token-Version", claims->token_version);
    request->addHeader("X-User-Role", resolveRole(claims->user_id));
    passCallback();
}
} // namespace nexustal::filters
