#include <gtest/gtest.h>

#include "domain/auth/capabilities.hpp"

TEST(CapabilitiesTest, AdminCanManageTeam)
{
    EXPECT_TRUE(nexustal::domain::auth::hasCapability(
        nexustal::domain::identity::UserRole::Admin,
        nexustal::domain::auth::Capability::ManageTeam));
}

TEST(CapabilitiesTest, ViewerCannotManageTeam)
{
    EXPECT_FALSE(nexustal::domain::auth::hasCapability(
        nexustal::domain::identity::UserRole::Viewer,
        nexustal::domain::auth::Capability::ManageTeam));
}
