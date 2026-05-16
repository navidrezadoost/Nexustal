#include "application/role_service.hpp"

namespace nexustal::application
{
RoleService::RoleService(std::shared_ptr<domain::identity::IProjectRepository> projectRepository,
                         drogon::nosql::RedisClientPtr cache)
    : projectRepository_(std::move(projectRepository))
    , cache_(std::move(cache))
{
}

auto RoleService::cacheKeyFor(const domain::UUID& userId, const domain::UUID& projectId) -> std::string
{
    return "project_role:" + projectId + ':' + userId;
}

auto RoleService::getProjectRole(const domain::UUID& userId, const domain::UUID& projectId)
    -> std::optional<domain::identity::UserRole>
{
    const auto cacheKey = cacheKeyFor(userId, projectId);
    if (cache_)
    {
        try
        {
            const auto cached = cache_->execCommandSync("GET %s", cacheKey.c_str());
            const auto cachedValue = cached.asString();
            if (!cachedValue.empty())
            {
                return domain::auth::roleFromString(cachedValue);
            }
        }
        catch (const std::exception&)
        {
        }
    }

    const auto role = projectRepository_->findRoleForUser(projectId, userId);
    if (role.has_value() && cache_)
    {
        const auto serialized = std::string{domain::auth::toString(role.value())};
        cache_->execCommandAsync(
            [](const drogon::nosql::RedisResult&) {},
            [](const std::exception&) {},
            "SETEX %s %d %s",
            cacheKey.c_str(),
            RoleCacheTtlSeconds,
            serialized.c_str());
    }

    return role;
}

auto RoleService::getCapabilities(const domain::UUID& userId, const domain::UUID& projectId)
    -> std::unordered_set<domain::auth::Capability, domain::auth::CapabilityHash>
{
    const auto role = getProjectRole(userId, projectId);
    return role.has_value()
               ? domain::auth::capabilitiesForRole(role.value())
               : std::unordered_set<domain::auth::Capability, domain::auth::CapabilityHash>{};
}

auto RoleService::hasCapability(const domain::UUID& userId,
                                const domain::UUID& projectId,
                                domain::auth::Capability capability) -> bool
{
    const auto role = getProjectRole(userId, projectId);
    return role.has_value() && domain::auth::hasCapability(role.value(), capability);
}
} // namespace nexustal::application
