#include "infrastructure/adapters/postgres_project_repository.hpp"

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>

#include "domain/auth/capabilities.hpp"

namespace nexustal::infrastructure::adapters
{
PostgresProjectRepository::PostgresProjectRepository(drogon::orm::DbClientPtr db)
    : db_(std::move(db))
{
}

auto PostgresProjectRepository::findRoleForUser(const domain::UUID& project_id,
                                                const domain::UUID& user_id)
    -> std::optional<domain::identity::UserRole>
{
    try
    {
        const auto result = db_->execSqlSync(
            "SELECT tm.role FROM team_members tm "
            "JOIN projects p ON p.team_id = tm.team_id "
            "WHERE p.id = $1 AND tm.user_id = $2",
            project_id,
            user_id);

        if (result.empty())
        {
            return std::nullopt;
        }

        return domain::auth::roleFromString(result[0]["role"].as<std::string>());
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresProjectRepository::findRoleForUser failed: " << exception.base().what();
        return std::nullopt;
    }
}

auto PostgresProjectRepository::addMember(const domain::UUID& project_id,
                                          const domain::UUID& user_id,
                                          domain::identity::UserRole role) -> bool
{
    try
    {
        const auto result = db_->execSqlSync(
            "INSERT INTO team_members (team_id, user_id, role) "
            "SELECT team_id, $2, $3 FROM projects WHERE id = $1 "
            "ON CONFLICT (team_id, user_id) DO UPDATE SET role = EXCLUDED.role",
            project_id,
            user_id,
            std::string{domain::auth::toString(role)});

        return result.affectedRows() > 0;
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresProjectRepository::addMember failed: " << exception.base().what();
        return false;
    }
}
} // namespace nexustal::infrastructure::adapters
