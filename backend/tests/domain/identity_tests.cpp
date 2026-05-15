#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <utility>
#include <vector>

#include "application/identity_use_cases.hpp"
#include "domain/identity.hpp"
#include "domain/identity_repository.hpp"

namespace
{
using nexustal::domain::Timestamp;
using nexustal::domain::UUID;
using nexustal::domain::identity::IUserRepository;
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
        std::vector<User> matches;
        const auto loweredQuery = toLower(criteria.query);

        for (const auto& user : users_)
        {
            if (criteria.exclude_user_id.has_value() && user.id == criteria.exclude_user_id.value())
            {
                continue;
            }

            const auto loweredUsername = toLower(user.username);
            if (loweredQuery.empty() || loweredUsername.find(loweredQuery) != std::string::npos)
            {
                matches.push_back(user);
            }
        }

        return matches;
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

private:
    static auto toLower(const std::string& value) -> std::string
    {
        std::string lowered = value;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
        return lowered;
    }

    std::vector<User> users_;
};

auto makeUser(UUID id, std::string username) -> User
{
    return User{
        .id = std::move(id),
        .username = std::move(username),
        .password_hash = "$2b$12$examplehash",
        .is_active = true,
        .token_version = nexustal::domain::generate_uuid(),
        .created_at = Timestamp{},
    };
}
} // namespace

TEST(IdentityTest, CreateUserValidDataSuccess)
{
    FakeUserRepository repository;
    nexustal::application::identity::CreateUserInteractor interactor{repository};

    const auto result = interactor.execute(makeUser("user-1", "alice"));

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.created_user_id, "user-1");
    ASSERT_TRUE(repository.findByUsername("alice").has_value());
}

TEST(IdentityTest, SearchUsersFiltersByQueryAndExcludedUser)
{
    FakeUserRepository repository;
    repository.create(makeUser("user-1", "alice"));
    repository.create(makeUser("user-2", "alicia"));
    repository.create(makeUser("user-3", "bob"));

    nexustal::application::identity::SearchUsersInteractor interactor{repository};

    const auto results = interactor.execute(UserSearchCriteria{
        .query = "ali",
        .exclude_user_id = std::make_optional<UUID>("user-1"),
    });

    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results.front().username, "alicia");
}
