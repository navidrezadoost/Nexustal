#include "infrastructure/adapters/postgres_team_repository.hpp"

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>

#include "domain/auth/capabilities.hpp"
#include "infrastructure/adapters/timestamp_codec.hpp"

namespace nexustal::infrastructure::adapters
{
PostgresTeamRepository::PostgresTeamRepository(drogon::orm::DbClientPtr db)
    : db_(std::move(db))
{
}

auto PostgresTeamRepository::create(const domain::identity::Team& team) -> domain::UUID
{
    try
    {
        const auto result = db_->execSqlSync(
            "INSERT INTO teams (id, name, description, owner_id) VALUES ($1, $2, $3, $4) RETURNING id",
            team.id,
            team.name,
            team.description,
            team.owner_id);

        return result.empty() ? domain::UUID{} : result[0]["id"].as<std::string>();
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresTeamRepository::create failed: " << exception.base().what();
        return {};
    }
}

auto PostgresTeamRepository::findById(const domain::UUID& id) -> std::optional<domain::identity::Team>
{
    try
    {
        const auto result = db_->execSqlSync(
            "SELECT id, name, description, owner_id, created_at FROM teams WHERE id = $1",
            id);
        return result.empty() ? std::nullopt : std::optional<domain::identity::Team>{mapTeam(result[0])};
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresTeamRepository::findById failed: " << exception.base().what();
        return std::nullopt;
    }
}

auto PostgresTeamRepository::findByOwner(const domain::UUID& owner_id) -> std::vector<domain::identity::Team>
{
    std::vector<domain::identity::Team> teams;

    try
    {
        const auto result = db_->execSqlSync(
            "SELECT id, name, description, owner_id, created_at FROM teams WHERE owner_id = $1 ORDER BY created_at DESC",
            owner_id);
        teams.reserve(result.size());
        for (const auto& row : result)
        {
            teams.push_back(mapTeam(row));
        }
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresTeamRepository::findByOwner failed: " << exception.base().what();
    }

    return teams;
}

auto PostgresTeamRepository::addMember(const domain::UUID& team_id,
                                       const domain::UUID& user_id,
                                       domain::identity::UserRole role) -> bool
{
    try
    {
        const auto result = db_->execSqlSync(
            "INSERT INTO team_members (team_id, user_id, role) VALUES ($1, $2, $3) "
            "ON CONFLICT (team_id, user_id) DO UPDATE SET role = EXCLUDED.role",
            team_id,
            user_id,
            std::string{domain::auth::toString(role)});

        return result.affectedRows() > 0;
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresTeamRepository::addMember failed: " << exception.base().what();
        return false;
    }
}

auto PostgresTeamRepository::mapTeam(const drogon::orm::Row& row) const -> domain::identity::Team
{
    return domain::identity::Team{
        .id = row["id"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .description = row["description"].isNull() ? std::string{} : row["description"].as<std::string>(),
        .owner_id = row["owner_id"].as<std::string>(),
        .created_at = parseTimestamp(row["created_at"].as<std::string>()),
    };
}
} // namespace nexustal::infrastructure::adapters
