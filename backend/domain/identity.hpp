#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "domain/common.hpp"

namespace nexustal::domain::identity
{
using ::nexustal::domain::Timestamp;
using ::nexustal::domain::UUID;

enum class UserRole : std::uint8_t
{
    Admin = 0,
    BackendDev = 1,
    FrontendDev = 2,
    Viewer = 3,
};

enum class InvitationStatus : std::uint8_t
{
    Pending = 0,
    Accepted = 1,
    Rejected = 2,
};

struct User
{
    UUID id;
    std::string username;
    std::string password_hash;
    bool is_admin = false;
    bool is_active = true;
    UUID token_version;
    Timestamp created_at;
};

struct Team
{
    UUID id;
    std::string name;
    std::string description;
    UUID owner_id;
    Timestamp created_at;
};

struct TeamMember
{
    UUID team_id;
    UUID user_id;
    UserRole role;
};

struct Project
{
    UUID id;
    std::string name;
    std::string description;
    std::string photo_url;
    UUID creator_id;
    Timestamp created_at;
};

struct Invitation
{
    UUID id;
    UUID from_user_id;
    UUID to_user_id;
    std::optional<UUID> project_id;
    std::optional<UUID> team_id;
    UserRole role;
    InvitationStatus status = InvitationStatus::Pending;
    Timestamp created_at;
};

struct UserSearchCriteria
{
    std::string query;
    std::optional<UUID> exclude_user_id;
};
} // namespace nexustal::domain::identity
