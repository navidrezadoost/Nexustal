#pragma once

#include <memory>
#include <optional>
#include <string>

#include "domain/auth/token_service.hpp"
#include "domain/identity_repository.hpp"
#include "domain/password_hasher.hpp"

namespace nexustal::application
{
struct LoginResult
{
    bool success = false;
    std::string message;
    domain::auth::TokenPair tokens;
    domain::UUID user_id;
};

class AuthService
{
public:
    AuthService(std::shared_ptr<domain::identity::IUserRepository> userRepository,
                std::shared_ptr<domain::services::IPasswordHasher> passwordHasher,
                std::shared_ptr<domain::auth::ITokenService> tokenService);

    [[nodiscard]] auto login(const std::string& username, const std::string& password) -> LoginResult;
    auto logout(const domain::UUID& userId) -> bool;
    [[nodiscard]] auto validateAccessToken(const std::string& token) const
        -> std::optional<domain::auth::TokenClaims>;

private:
    std::shared_ptr<domain::identity::IUserRepository> userRepository_;
    std::shared_ptr<domain::services::IPasswordHasher> passwordHasher_;
    std::shared_ptr<domain::auth::ITokenService> tokenService_;
};
} // namespace nexustal::application
