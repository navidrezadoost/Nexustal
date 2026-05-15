#pragma once

#include <stdexcept>
#include <string>

#if __has_include(<bcrypt/BCrypt.hpp>)
#include <bcrypt/BCrypt.hpp>
#endif

#include "domain/password_hasher.hpp"

namespace nexustal::infrastructure::adapters
{
class BCryptPasswordHasher : public domain::services::IPasswordHasher
{
public:
    std::string hash(const std::string& plaintext) override
    {
#if __has_include(<bcrypt/BCrypt.hpp>)
        return BCrypt::generateHash(plaintext);
#else
        throw std::runtime_error("BCrypt dependency is not available");
#endif
    }

    bool verify(const std::string& plaintext, const std::string& hashValue) override
    {
#if __has_include(<bcrypt/BCrypt.hpp>)
        return BCrypt::validatePassword(plaintext, hashValue);
#else
        (void)plaintext;
        (void)hashValue;
        return false;
#endif
    }
};
} // namespace nexustal::infrastructure::adapters
