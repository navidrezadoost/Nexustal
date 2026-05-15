#pragma once

#include <string>

namespace nexustal::domain::services
{
class IPasswordHasher
{
public:
    virtual ~IPasswordHasher() = default;

    virtual std::string hash(const std::string& plaintext) = 0;
    virtual bool verify(const std::string& plaintext, const std::string& hash) = 0;
};
} // namespace nexustal::domain::services