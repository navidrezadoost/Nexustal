#include "infrastructure/adapters/postgres_invitation_repository.hpp"

#include <drogon/orm/Exception.h>
#include <drogon/drogon.h>

#include "domain/auth/capabilities.hpp"
#include "infrastructure/adapters/timestamp_codec.hpp"

namespace nexustal::infrastructure::adapters
{
PostgresInvitationRepository::PostgresInvitationRepository(drogon::orm::DbClientPtr db)
    : db_(std::move(db))
{
}

auto PostgresInvitationRepository::create(const domain::identity::Invitation& invitation) -> domain::UUID
{
    try
    {
        const auto role = std::string{domain::auth::toString(invitation.role)};
        const auto status = statusToString(invitation.status);

        drogon::orm::Result result;
        if (invitation.team_id.has_value() && invitation.project_id.has_value())
        {
            result = db_->execSqlSync(
                "INSERT INTO invitations (id, from_user_id, to_user_id, team_id, project_id, role, status) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING id",
                invitation.id,
                invitation.from_user_id,
                invitation.to_user_id,
                invitation.team_id.value(),
                invitation.project_id.value(),
                role,
                status);
        }
        else if (invitation.project_id.has_value())
        {
            result = db_->execSqlSync(
                "INSERT INTO invitations (id, from_user_id, to_user_id, team_id, project_id, role, status) "
                "VALUES ($1, $2, $3, NULL, $4, $5, $6) RETURNING id",
                invitation.id,
                invitation.from_user_id,
                invitation.to_user_id,
                invitation.project_id.value(),
                role,
                status);
        }
        else if (invitation.team_id.has_value())
        {
            result = db_->execSqlSync(
                "INSERT INTO invitations (id, from_user_id, to_user_id, team_id, project_id, role, status) "
                "VALUES ($1, $2, $3, $4, NULL, $5, $6) RETURNING id",
                invitation.id,
                invitation.from_user_id,
                invitation.to_user_id,
                invitation.team_id.value(),
                role,
                status);
        }
        else
        {
            result = db_->execSqlSync(
                "INSERT INTO invitations (id, from_user_id, to_user_id, team_id, project_id, role, status) "
                "VALUES ($1, $2, $3, NULL, NULL, $4, $5) RETURNING id",
                invitation.id,
                invitation.from_user_id,
                invitation.to_user_id,
                role,
                status);
        }

        return result.empty() ? domain::UUID{} : result[0]["id"].as<std::string>();
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresInvitationRepository::create failed: " << exception.base().what();
        return {};
    }
}

auto PostgresInvitationRepository::findPendingForUser(const domain::UUID& userId)
    -> std::vector<domain::identity::Invitation>
{
    std::vector<domain::identity::Invitation> invitations;

    try
    {
        const auto result = db_->execSqlSync(
            "SELECT id, from_user_id, to_user_id, team_id, project_id, role, status, created_at "
            "FROM invitations WHERE to_user_id = $1 AND status = 'pending' ORDER BY created_at DESC",
            userId);

        invitations.reserve(result.size());
        for (const auto& row : result)
        {
            invitations.push_back(mapInvitation(row));
        }
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresInvitationRepository::findPendingForUser failed: " << exception.base().what();
    }

    return invitations;
}

auto PostgresInvitationRepository::updateStatus(const domain::UUID& invitationId,
                                                domain::identity::InvitationStatus status) -> bool
{
    try
    {
        const auto result = db_->execSqlSync(
            "UPDATE invitations SET status = $1, responded_at = NOW() WHERE id = $2",
            statusToString(status),
            invitationId);

        return result.affectedRows() > 0;
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresInvitationRepository::updateStatus failed: " << exception.base().what();
        return false;
    }
}

auto PostgresInvitationRepository::mapInvitation(const drogon::orm::Row& row) const
    -> domain::identity::Invitation
{
    std::optional<domain::UUID> teamId;
    if (!row["team_id"].isNull())
    {
        teamId = row["team_id"].as<std::string>();
    }

    std::optional<domain::UUID> projectId;
    if (!row["project_id"].isNull())
    {
        projectId = row["project_id"].as<std::string>();
    }

    return domain::identity::Invitation{
        .id = row["id"].as<std::string>(),
        .from_user_id = row["from_user_id"].as<std::string>(),
        .to_user_id = row["to_user_id"].as<std::string>(),
        .project_id = projectId,
        .team_id = teamId,
        .role = domain::auth::roleFromString(row["role"].as<std::string>()),
        .status = statusFromString(row["status"].as<std::string>()),
        .created_at = parseTimestamp(row["created_at"].as<std::string>()),
    };
}

auto PostgresInvitationRepository::statusToString(domain::identity::InvitationStatus status) -> std::string
{
    switch (status)
    {
    case domain::identity::InvitationStatus::Accepted:
        return "accepted";
    case domain::identity::InvitationStatus::Rejected:
        return "rejected";
    case domain::identity::InvitationStatus::Pending:
    default:
        return "pending";
    }
}

auto PostgresInvitationRepository::statusFromString(const std::string& status)
    -> domain::identity::InvitationStatus
{
    if (status == "accepted")
    {
        return domain::identity::InvitationStatus::Accepted;
    }
    if (status == "rejected")
    {
        return domain::identity::InvitationStatus::Rejected;
    }
    return domain::identity::InvitationStatus::Pending;
}
} // namespace nexustal::infrastructure::adapters
