#pragma once

#include <optional>
#include <vector>

#include "domain/identity.hpp"

namespace nexustal::domain::identity
{
class IUserRepository
{
public:
    virtual ~IUserRepository() = default;

    virtual std::optional<User> findByUsername(const std::string& username) = 0;
    virtual std::optional<User> findById(const UUID& id) = 0;
    virtual std::vector<User> search(const UserSearchCriteria& criteria) = 0;
    virtual UUID create(const User& user) = 0;
    virtual bool update(const User& user) = 0;
};

class ITeamRepository
{
public:
    virtual ~ITeamRepository() = default;

    virtual UUID create(const Team& team) = 0;
    virtual std::optional<Team> findById(const UUID& id) = 0;
    virtual std::vector<Team> findByOwner(const UUID& owner_id) = 0;
    virtual bool addMember(const UUID& team_id, const UUID& user_id, UserRole role) = 0;
};

class IProjectRepository
{
public:
    virtual ~IProjectRepository() = default;

    virtual std::optional<UserRole> findRoleForUser(const UUID& project_id, const UUID& user_id) = 0;
    virtual bool addMember(const UUID& project_id, const UUID& user_id, UserRole role) = 0;
};

class IInvitationRepository
{
public:
    virtual ~IInvitationRepository() = default;

    virtual UUID create(const Invitation& invitation) = 0;
    virtual std::vector<Invitation> findPendingForUser(const UUID& user_id) = 0;
    virtual bool updateStatus(const UUID& invitation_id, InvitationStatus status) = 0;
};
} // namespace nexustal::domain::identity
