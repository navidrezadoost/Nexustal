#pragma once

#include <memory>
#include <optional>
#include <unordered_set>

#include <drogon/nosql/RedisClient.h>

#include "domain/auth/capabilities.hpp"
#include "domain/identity_repository.hpp"

namespace nexustal::application
{
class RoleService
{
public:
    static constexpr int RoleCacheTtlSeconds = 300;

    RoleService(std::shared_ptr<domain::identity::IProjectRepository> projectRepository,
                drogon::nosql::RedisClientPtr cache);

    [[nodiscard]] auto getProjectRole(const domain::UUID& userId, const domain::UUID& projectId)
        -> std::optional<domain::identity::UserRole>;
    [[nodiscard]] auto getCapabilities(const domain::UUID& userId, const domain::UUID& projectId)
        -> std::unordered_set<domain::auth::Capability, domain::auth::CapabilityHash>;
    [[nodiscard]] auto hasCapability(const domain::UUID& userId,
                                     const domain::UUID& projectId,
                                     domain::auth::Capability capability) -> bool;

private:
    [[nodiscard]] static auto cacheKeyFor(const domain::UUID& userId, const domain::UUID& projectId) -> std::string;

    std::shared_ptr<domain::identity::IProjectRepository> projectRepository_;
    drogon::nosql::RedisClientPtr cache_;
};
} // namespace nexustal::application
