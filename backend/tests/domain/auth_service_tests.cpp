#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "application/auth_service.hpp"
#include "domain/auth/token_service.hpp"
#include "domain/common.hpp"
#include "domain/identity_repository.hpp"
#include "domain/password_hasher.hpp"

namespace
{
using nexustal::domain::UUID;
using nexustal::domain::identity::IUserRepository;
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
        return iterator == users_.end() ? std::nullopt : std::optional<User>{*iterator};
    }

    std::optional<User> findById(const UUID& id) override
    {
        const auto iterator = std::find_if(users_.begin(), users_.end(), [&](const User& user) {
            return user.id == id;
        });
        return iterator == users_.end() ? std::nullopt : std::optional<User>{*iterator};
    }

    std::vector<User> search(const UserSearchCriteria& criteria) override
    {
        std::vector<User> result;
        for (const auto& user : users_)
        {
            if (!criteria.query.empty() && user.username.find(criteria.query) == std::string::npos)
            {
                continue;
            }
            result.push_back(user);
        }
        return result;
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

class InMemoryTokenService final : public nexustal::domain::auth::ITokenService
{
public:
    nexustal::domain::auth::TokenPair createTokens(const User& user) override
    {
        versions_[user.id] = user.token_version;
        return nexustal::domain::auth::TokenPair{
            .access_token = user.id + ":" + user.token_version,
        };
    }

    std::optional<nexustal::domain::auth::TokenClaims> validateAccessToken(const std::string& token) override
    {
        const auto delimiter = token.find(':');
        if (delimiter == std::string::npos)
        {
            return std::nullopt;
        }

        const auto userId = token.substr(0, delimiter);
        const auto version = token.substr(delimiter + 1);
        const auto iterator = versions_.find(userId);
        if (iterator == versions_.end() || iterator->second != version)
        {
            return std::nullopt;
        }

        return nexustal::domain::auth::TokenClaims{
            .user_id = userId,
            .token_version = version,
        };
    }

    void rotateVersion(const UUID& userId) override
    {
        versions_[userId] = nexustal::domain::generate_uuid();
    }

private:
    std::unordered_map<UUID, UUID> versions_;
};

auto makeUser(std::string id, std::string username, std::string password) -> User
{
    return User{
        .id = std::move(id),
        .username = std::move(username),
        .password_hash = "hashed::" + password,
        .is_active = true,
        .token_version = nexustal::domain::generate_uuid(),
        .created_at = nexustal::domain::Timestamp{},
    };
}
} // namespace

TEST(AuthServiceTest, LoginReturnsTokenForValidCredentials)
{
    auto userRepository = std::make_shared<FakeUserRepository>();
    auto passwordHasher = std::make_shared<FakePasswordHasher>();
    auto tokenService = std::make_shared<InMemoryTokenService>();
    userRepository->seed(makeUser("user-1", "alice", "secret"));

    nexustal::application::AuthService service{userRepository, passwordHasher, tokenService};
    const auto result = service.login("alice", "secret");

    ASSERT_TRUE(result.success);
    EXPECT_FALSE(result.tokens.access_token.empty());
    EXPECT_EQ(result.user_id, "user-1");
}

TEST(AuthServiceTest, LogoutInvalidatesPreviousToken)
{
    auto userRepository = std::make_shared<FakeUserRepository>();
    auto passwordHasher = std::make_shared<FakePasswordHasher>();
    auto tokenService = std::make_shared<InMemoryTokenService>();
    userRepository->seed(makeUser("user-1", "alice", "secret"));

    nexustal::application::AuthService service{userRepository, passwordHasher, tokenService};
    const auto login = service.login("alice", "secret");
    ASSERT_TRUE(login.success);
    ASSERT_TRUE(service.validateAccessToken(login.tokens.access_token).has_value());

    ASSERT_TRUE(service.logout(login.user_id));
    EXPECT_FALSE(service.validateAccessToken(login.tokens.access_token).has_value());
}

TEST(AuthServiceTest, TokenServiceRejectsRotatedVersion)
{
    InMemoryTokenService tokenService;
    const auto user = makeUser("user-9", "navid", "secret");
    const auto token = tokenService.createTokens(user).access_token;

    ASSERT_TRUE(tokenService.validateAccessToken(token).has_value());
    tokenService.rotateVersion(user.id);
    EXPECT_FALSE(tokenService.validateAccessToken(token).has_value());
}
