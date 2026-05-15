#include "application/auth_service.hpp"

namespace nexustal::application
{
AuthService::AuthService(std::shared_ptr<domain::identity::IUserRepository> userRepository,
                         std::shared_ptr<domain::services::IPasswordHasher> passwordHasher,
                         std::shared_ptr<domain::auth::ITokenService> tokenService)
    : userRepository_(std::move(userRepository))
    , passwordHasher_(std::move(passwordHasher))
    , tokenService_(std::move(tokenService))
{
}

auto AuthService::login(const std::string& username, const std::string& password) -> LoginResult
{
    const auto user = userRepository_->findByUsername(username);
    if (!user.has_value())
    {
        return LoginResult{.success = false, .message = "invalid credentials"};
    }

    if (!user->is_active)
    {
        return LoginResult{.success = false, .message = "user is inactive"};
    }

    if (!passwordHasher_->verify(password, user->password_hash))
    {
        return LoginResult{.success = false, .message = "invalid credentials"};
    }

    return LoginResult{
        .success = true,
        .message = "authenticated",
        .tokens = tokenService_->createTokens(*user),
        .user_id = user->id,
    };
}

auto AuthService::logout(const domain::UUID& userId) -> bool
{
    if (userId.empty())
    {
        return false;
    }

    tokenService_->rotateVersion(userId);
    return true;
}

auto AuthService::validateAccessToken(const std::string& token) const
    -> std::optional<domain::auth::TokenClaims>
{
    return tokenService_->validateAccessToken(token);
}
} // namespace nexustal::application
