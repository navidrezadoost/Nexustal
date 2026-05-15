#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "application/wizard_service.hpp"
#include "domain/identity_repository.hpp"
#include "domain/password_hasher.hpp"

namespace
{
using nexustal::domain::UUID;
using nexustal::domain::identity::IInvitationRepository;
using nexustal::domain::identity::ITeamRepository;
using nexustal::domain::identity::IUserRepository;
using nexustal::domain::identity::Invitation;
using nexustal::domain::identity::InvitationStatus;
using nexustal::domain::identity::Team;
using nexustal::domain::identity::User;
using nexustal::domain::identity::UserSearchCriteria;
using nexustal::domain::services::IPasswordHasher;

class FakeUserRepository final : public IUserRepository
{
public:
    std::optional<User> findByUsername(const std::string& username) override
    {
        const auto iterator = std::find_if(users_.begin(), users_.end(), [&](const User& user) {
            return user.username == username;
        });

        if (iterator == users_.end())
        {
            return std::nullopt;
        }

        return *iterator;
    }

    std::optional<User> findById(const UUID& id) override
    {
        const auto iterator = std::find_if(users_.begin(), users_.end(), [&](const User& user) {
            return user.id == id;
        });

        if (iterator == users_.end())
        {
            return std::nullopt;
        }

        return *iterator;
    }

    std::vector<User> search(const UserSearchCriteria& criteria) override
    {
        std::vector<User> results;
        for (const auto& user : users_)
        {
            if (!criteria.query.empty() && user.username.find(criteria.query) == std::string::npos)
            {
                continue;
            }

            if (criteria.exclude_user_id.has_value() && user.id == criteria.exclude_user_id.value())
            {
                continue;
            }

            results.push_back(user);
        }

        return results;
    }

    UUID create(const User& user) override
    {
        auto created = user;
        if (created.id.empty())
        {
            created.id = "generated-" + std::to_string(users_.size() + 1U);
        }
        users_.push_back(created);
        return created.id;
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

private:
    std::vector<User> users_;
};

class FakePasswordHasher final : public IPasswordHasher
{
public:
    std::string hash(const std::string& plaintext) override
    {
        return "hashed::" + plaintext;
    }

    bool verify(const std::string& plaintext, const std::string& hash) override
    {
        return hash == "hashed::" + plaintext;
    }
};
} // namespace

TEST(WizardServiceTest, StatusAdvancesAcrossWizardSteps)
{
    auto userRepository = std::make_shared<FakeUserRepository>();
    auto passwordHasher = std::make_shared<FakePasswordHasher>();
    nexustal::application::WizardService service{userRepository, passwordHasher};

    auto status = service.getStatus();
    EXPECT_EQ(status.current_step, nexustal::domain::wizard::WizardStatus::Step::Database);
    EXPECT_FALSE(status.is_complete);

    ASSERT_TRUE(service.configureDatabase({.host = "db", .port = 5432, .dbname = "nexustal", .user = "nexustal", .password = "secret"}));
    status = service.getStatus();
    EXPECT_EQ(status.current_step, nexustal::domain::wizard::WizardStatus::Step::Admin);

    ASSERT_TRUE(service.createAdmin({.username = "root", .password = "secret"}));
    status = service.getStatus();
    EXPECT_EQ(status.current_step, nexustal::domain::wizard::WizardStatus::Step::Company);

    ASSERT_TRUE(service.registerCompany({.name = "Nexustal Labs", .activity_scope = "platform", .installation_id = "instance-1"}));
    status = service.getStatus();
    EXPECT_EQ(status.current_step, nexustal::domain::wizard::WizardStatus::Step::Complete);
    EXPECT_TRUE(status.is_complete);
}

TEST(WizardServiceTest, CreateAdminHashesPasswordBeforePersisting)
{
    auto userRepository = std::make_shared<FakeUserRepository>();
    auto passwordHasher = std::make_shared<FakePasswordHasher>();
    nexustal::application::WizardService service{userRepository, passwordHasher};

    ASSERT_TRUE(service.configureDatabase({.host = "db", .port = 5432, .dbname = "nexustal", .user = "nexustal", .password = "secret"}));
    ASSERT_TRUE(service.createAdmin({.username = "root", .password = "plaintext"}));

    const auto admin = userRepository->findByUsername("root");
    ASSERT_TRUE(admin.has_value());
    EXPECT_EQ(admin->password_hash, "hashed::plaintext");
    EXPECT_FALSE(admin->token_version.empty());
}
