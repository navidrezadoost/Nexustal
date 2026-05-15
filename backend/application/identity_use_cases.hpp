#pragma once

#include <string>
#include <vector>

#include "domain/identity_repository.hpp"

namespace nexustal::application::identity
{
using ::nexustal::domain::UUID;
using ::nexustal::domain::identity::IUserRepository;
using ::nexustal::domain::identity::User;
using ::nexustal::domain::identity::UserSearchCriteria;

struct CreateUserResult
{
    bool success = false;
    std::string message;
    UUID created_user_id;
};

class CreateUserInteractor
{
public:
    explicit CreateUserInteractor(IUserRepository& userRepository)
        : userRepository_(userRepository)
    {
    }

    auto execute(const User& user) -> CreateUserResult
    {
        if (user.username.empty())
        {
            return CreateUserResult{.success = false, .message = "username is required"};
        }

        if (user.password_hash.empty())
        {
            return CreateUserResult{.success = false, .message = "password hash is required"};
        }

        if (userRepository_.findByUsername(user.username).has_value())
        {
            return CreateUserResult{.success = false, .message = "username already exists"};
        }

        return CreateUserResult{
            .success = true,
            .message = "user created",
            .created_user_id = userRepository_.create(user),
        };
    }

private:
    IUserRepository& userRepository_;
};

class SearchUsersInteractor
{
public:
    explicit SearchUsersInteractor(IUserRepository& userRepository)
        : userRepository_(userRepository)
    {
    }

    auto execute(const UserSearchCriteria& criteria) -> std::vector<User>
    {
        return userRepository_.search(criteria);
    }

private:
    IUserRepository& userRepository_;
};
} // namespace nexustal::application::identity
