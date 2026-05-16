#include "infrastructure/adapters/postgres_environment_repository.hpp"

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>

#include "infrastructure/adapters/timestamp_codec.hpp"

namespace nexustal::infrastructure::adapters
{
namespace
{
auto parseVariables(const drogon::orm::Row& row, const char* fieldName) -> std::vector<domain::environment::Variable>
{
    if (row[fieldName].isNull())
    {
        return {};
    }

    return domain::environment::variablesFromJson(nlohmann::json::parse(row[fieldName].as<std::string>()));
}
} // namespace

PostgresEnvironmentRepository::PostgresEnvironmentRepository(drogon::orm::DbClientPtr db)
    : db_(std::move(db))
{
}

auto PostgresEnvironmentRepository::findByProject(const domain::UUID& project_id)
    -> std::vector<domain::environment::Environment>
{
    std::vector<domain::environment::Environment> environments;

    try
    {
        const auto result = db_->execSqlSync(
            "SELECT id, project_id, created_by, name, variables, created_at, updated_at "
            "FROM environments WHERE project_id = $1 ORDER BY created_at ASC",
            project_id);

        environments.reserve(result.size());
        for (const auto& row : result)
        {
            environments.push_back(mapEnvironment(row));
        }
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresEnvironmentRepository::findByProject failed: " << exception.base().what();
    }

    return environments;
}

auto PostgresEnvironmentRepository::findById(const domain::UUID& id) -> std::optional<domain::environment::Environment>
{
    try
    {
        const auto result = db_->execSqlSync(
            "SELECT id, project_id, created_by, name, variables, created_at, updated_at FROM environments WHERE id = $1",
            id);
        return result.empty() ? std::nullopt : std::optional<domain::environment::Environment>{mapEnvironment(result[0])};
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresEnvironmentRepository::findById failed: " << exception.base().what();
        return std::nullopt;
    }
}

auto PostgresEnvironmentRepository::create(const domain::environment::Environment& env) -> domain::UUID
{
    try
    {
        const auto result = db_->execSqlSync(
            "INSERT INTO environments (id, project_id, name, variables, created_by) "
            "VALUES ($1, $2, $3, $4::jsonb, $5) RETURNING id",
            env.id.empty() ? domain::generate_uuid() : env.id,
            env.project_id,
            env.name,
            domain::environment::variablesToJson(env.variables).dump(),
            env.created_by);

        return result.empty() ? domain::UUID{} : result[0]["id"].as<std::string>();
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresEnvironmentRepository::create failed: " << exception.base().what();
        return {};
    }
}

auto PostgresEnvironmentRepository::update(const domain::environment::Environment& env) -> bool
{
    try
    {
        const auto result = db_->execSqlSync(
            "UPDATE environments SET name = $1, variables = $2::jsonb, updated_at = NOW() WHERE id = $3",
            env.name,
            domain::environment::variablesToJson(env.variables).dump(),
            env.id);
        return result.affectedRows() > 0;
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresEnvironmentRepository::update failed: " << exception.base().what();
        return false;
    }
}

auto PostgresEnvironmentRepository::remove(const domain::UUID& id) -> bool
{
    try
    {
        const auto result = db_->execSqlSync("DELETE FROM environments WHERE id = $1", id);
        return result.affectedRows() > 0;
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresEnvironmentRepository::remove failed: " << exception.base().what();
        return false;
    }
}

auto PostgresEnvironmentRepository::getGlobalVariables(const domain::UUID& project_id)
    -> domain::environment::GlobalVariables
{
    try
    {
        const auto result = db_->execSqlSync(
            "SELECT project_id, variables FROM project_global_variables WHERE project_id = $1",
            project_id);
        if (result.empty())
        {
            return domain::environment::GlobalVariables{.project_id = project_id};
        }

        return domain::environment::GlobalVariables{
            .project_id = result[0]["project_id"].as<std::string>(),
            .variables = parseVariables(result[0], "variables"),
        };
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresEnvironmentRepository::getGlobalVariables failed: " << exception.base().what();
        return domain::environment::GlobalVariables{.project_id = project_id};
    }
}

auto PostgresEnvironmentRepository::setGlobalVariables(const domain::environment::GlobalVariables& globals) -> bool
{
    try
    {
        const auto result = db_->execSqlSync(
            "INSERT INTO project_global_variables (project_id, variables, updated_at) VALUES ($1, $2::jsonb, NOW()) "
            "ON CONFLICT (project_id) DO UPDATE SET variables = EXCLUDED.variables, updated_at = NOW()",
            globals.project_id,
            domain::environment::variablesToJson(globals.variables).dump());
        return result.affectedRows() > 0;
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresEnvironmentRepository::setGlobalVariables failed: " << exception.base().what();
        return false;
    }
}

auto PostgresEnvironmentRepository::mapEnvironment(const drogon::orm::Row& row) const
    -> domain::environment::Environment
{
    return domain::environment::Environment{
        .id = row["id"].as<std::string>(),
        .project_id = row["project_id"].as<std::string>(),
        .created_by = row["created_by"].as<std::string>(),
        .name = row["name"].as<std::string>(),
        .variables = parseVariables(row, "variables"),
        .created_at = parseTimestamp(row["created_at"].as<std::string>()),
        .updated_at = parseTimestamp(row["updated_at"].as<std::string>()),
    };
}
} // namespace nexustal::infrastructure::adapters