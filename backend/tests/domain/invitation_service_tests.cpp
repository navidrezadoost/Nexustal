#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

#include "application/invitation_service.hpp"
#include "domain/common.hpp"
#include "domain/identity_repository.hpp"
#include "domain/notification_port.hpp"

namespace
{
using nexustal::domain::UUID;
using nexustal::domain::identity::IInvitationRepository;
using nexustal::domain::identity::IProjectRepository;
using nexustal::domain::identity::ITeamRepository;
using nexustal::domain::identity::IUserRepository;
using nexustal::domain::identity::Invitation;
using nexustal::domain::identity::InvitationStatus;
using nexustal::domain::identity::UserRole;
using nexustal::domain::identity::User;
using nexustal::domain::identity::UserSearchCriteria;

class FakeUserRepository final : public IUserRepository
{
public:
    std::optional<User> findByUsername(const std::string& username) override
    {
        const auto iterator = std::find_if(users_.begin(), users_.end(), [&](const User& user) {
            return user.username == username;
        });
        return iterator == users_.end() ? std::nullopt : std::optional<User>{*iterator};
    }

    std::optional<User> findById(const UUID& id) override
    {
        const auto iterator = std::find_if(users_.begin(), users_.end(), [&](const User& user) {
            return user.id == id;
        });
        return iterator == users_.end() ? std::nullopt : std::optional<User>{*iterator};
    }

    std::vector<User> search(const UserSearchCriteria&) override
    {
        return users_;
    }

    UUID create(const User& user) override
    {
        users_.push_back(user);
        return user.id;
    }

    bool update(const User& user) override
    {
        for (auto& current : users_)
        {
            if (current.id == user.id)
            {
                current = user;
                return true;
            }
        }
        return false;
    }

    void seed(User user)
    {
        users_.push_back(std::move(user));
    }

private:
    std::vector<User> users_;
};

class FakeInvitationRepository final : public IInvitationRepository
{
public:
    UUID create(const Invitation& invitation) override
    {
        invitations_.push_back(invitation);
        return invitation.id;
    }

    std::vector<Invitation> findPendingForUser(const UUID& userId) override
    {
        std::vector<Invitation> result;
        for (const auto& invitation : invitations_)
        {
            if (invitation.to_user_id == userId && invitation.status == InvitationStatus::Pending)
            {
                result.push_back(invitation);
            }
        }
        return result;
    }

    bool updateStatus(const UUID& invitationId, InvitationStatus status) override
    {
        for (auto& invitation : invitations_)
        {
            if (invitation.id == invitationId)
            {
                invitation.status = status;
                return true;
            }
        }
        return false;
    }

private:
    std::vector<Invitation> invitations_;
};

class FakeNotificationService final : public nexustal::domain::notification::INotificationService
{
public:
    void sendNotification(const nexustal::domain::notification::Notification& notification) override
    {
        notifications.push_back(notification);
    }

    std::vector<nexustal::domain::notification::Notification> notifications;
};

class FakeTeamRepository final : public ITeamRepository
{
public:
    UUID create(const nexustal::domain::identity::Team&) override
    {
        return {};
    }

    std::optional<nexustal::domain::identity::Team> findById(const UUID&) override
    {
        return std::nullopt;
    }

    std::vector<nexustal::domain::identity::Team> findByOwner(const UUID&) override
    {
        return {};
    }

    bool addMember(const UUID& teamId, const UUID& userId, UserRole role) override
    {
        added_team_id = teamId;
        added_user_id = userId;
        added_role = role;
        return true;
    }

    UUID added_team_id;
    UUID added_user_id;
    std::optional<UserRole> added_role;
};

class FakeProjectRepository final : public IProjectRepository
{
public:
    std::optional<UserRole> findRoleForUser(const UUID&, const UUID&) override
    {
        return std::nullopt;
    }

    bool addMember(const UUID& projectId, const UUID& userId, UserRole role) override
    {
        added_project_id = projectId;
        added_user_id = userId;
        added_role = role;
        return true;
    }

    UUID added_project_id;
    UUID added_user_id;
    std::optional<UserRole> added_role;
};

auto makeUser(std::string id, std::string username) -> User
{
    return User{
        .id = std::move(id),
        .username = std::move(username),
        .password_hash = "hash",
        .is_admin = false,
        .is_active = true,
        .token_version = nexustal::domain::generate_uuid(),
        .created_at = nexustal::domain::Timestamp{},
    };
}
} // namespace

TEST(InvitationServiceTest, SendInvitationCreatesPendingInvitationAndNotification)
{
    auto users = std::make_shared<FakeUserRepository>();
    auto teams = std::make_shared<FakeTeamRepository>();
    auto projects = std::make_shared<FakeProjectRepository>();
    auto invitations = std::make_shared<FakeInvitationRepository>();
    auto notifications = std::make_shared<FakeNotificationService>();

    users->seed(makeUser("user-1", "alice"));
    users->seed(makeUser("user-2", "bob"));

    nexustal::application::InvitationService service{users, teams, projects, invitations, notifications};
    const auto invitationId = service.sendInvitation(
        "user-1",
        "user-2",
        "project-1",
        nexustal::domain::identity::UserRole::FrontendDev);

    ASSERT_FALSE(invitationId.empty());
    EXPECT_EQ(service.getPendingForUser("user-2").size(), 1U);
    EXPECT_EQ(notifications->notifications.size(), 1U);
}

TEST(InvitationServiceTest, AcceptChangesPendingInvitationStatus)
{
    auto users = std::make_shared<FakeUserRepository>();
    auto teams = std::make_shared<FakeTeamRepository>();
    auto projects = std::make_shared<FakeProjectRepository>();
    auto invitations = std::make_shared<FakeInvitationRepository>();
    auto notifications = std::make_shared<FakeNotificationService>();

    users->seed(makeUser("user-1", "alice"));
    users->seed(makeUser("user-2", "bob"));

    nexustal::application::InvitationService service{users, teams, projects, invitations, notifications};
    const auto invitationId = service.sendInvitation(
        "user-1",
        "user-2",
        "project-1",
        nexustal::domain::identity::UserRole::Viewer);

    ASSERT_TRUE(service.accept(invitationId, "user-2"));
    EXPECT_TRUE(service.getPendingForUser("user-2").empty());
    EXPECT_EQ(projects->added_project_id, "project-1");
    EXPECT_EQ(projects->added_user_id, "user-2");
    ASSERT_TRUE(projects->added_role.has_value());
    EXPECT_EQ(projects->added_role.value(), nexustal::domain::identity::UserRole::Viewer);
    EXPECT_EQ(notifications->notifications.size(), 2U);
}
