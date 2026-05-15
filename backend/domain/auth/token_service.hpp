#pragma once

#include <optional>
#include <string>

#include "domain/identity.hpp"

namespace nexustal::domain::auth
{
struct TokenPair
{
    std::string access_token;
};

struct TokenClaims
{
    identity::UUID user_id;
    identity::UUID token_version;
};

class ITokenService
{
public:
    virtual ~ITokenService() = default;

    virtual TokenPair createTokens(const identity::User& user) = 0;
    virtual std::optional<TokenClaims> validateAccessToken(const std::string& token) = 0;
    virtual void rotateVersion(const identity::UUID& user_id) = 0;
};
} // namespace nexustal::domain::auth
