#include <gtest/gtest.h>

#include <memory>
#include <optional>

#include "application/role_service.hpp"
#include "domain/identity_repository.hpp"

namespace
{
class FakeProjectRepository final : public nexustal::domain::identity::IProjectRepository
{
public:
    std::optional<nexustal::domain::identity::UserRole> findRoleForUser(const nexustal::domain::UUID&,
                                                                        const nexustal::domain::UUID&) override
    {
        return role;
    }

    bool addMember(const nexustal::domain::UUID&, const nexustal::domain::UUID&, nexustal::domain::identity::UserRole) override
    {
        return true;
    }

    std::optional<nexustal::domain::identity::UserRole> role = nexustal::domain::identity::UserRole::Viewer;
};
} // namespace

TEST(RoleServiceTest, AdminRoleHasManageTeamCapability)
{
    auto repository = std::make_shared<FakeProjectRepository>();
    repository->role = nexustal::domain::identity::UserRole::Admin;

    nexustal::application::RoleService service{repository, nullptr};
    EXPECT_TRUE(service.hasCapability("user-1", "project-1", nexustal::domain::auth::Capability::ManageTeam));
}

TEST(RoleServiceTest, ViewerRoleCannotEditEndpoints)
{
    auto repository = std::make_shared<FakeProjectRepository>();
    repository->role = nexustal::domain::identity::UserRole::Viewer;

    nexustal::application::RoleService service{repository, nullptr};
    EXPECT_FALSE(service.hasCapability("user-1", "project-1", nexustal::domain::auth::Capability::EditEndpoints));
}
