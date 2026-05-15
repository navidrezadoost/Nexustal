#include "infrastructure/adapters/postgres_user_repository.hpp"

#include <drogon/orm/Exception.h>
#include <drogon/drogon.h>

#include "infrastructure/adapters/timestamp_codec.hpp"

namespace nexustal::infrastructure::adapters
{
using domain::UUID;
using domain::identity::User;
using domain::identity::UserSearchCriteria;

PostgresUserRepository::PostgresUserRepository(drogon::orm::DbClientPtr db)
    : db_(std::move(db))
{
}

auto PostgresUserRepository::findByUsername(const std::string& username) -> std::optional<User>
{
    try
    {
        const auto result = db_->execSqlSync(
            "SELECT id, username, password_hash, is_admin, is_active, token_version, created_at FROM users WHERE username = $1",
            username);

        if (result.empty())
        {
            return std::nullopt;
        }

        return mapUser(result[0]);
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresUserRepository::findByUsername failed: " << exception.base().what();
        return std::nullopt;
    }
}

auto PostgresUserRepository::findById(const UUID& id) -> std::optional<User>
{
    try
    {
        const auto result = db_->execSqlSync(
            "SELECT id, username, password_hash, is_admin, is_active, token_version, created_at FROM users WHERE id = $1",
            id);

        if (result.empty())
        {
            return std::nullopt;
        }

        return mapUser(result[0]);
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresUserRepository::findById failed: " << exception.base().what();
        return std::nullopt;
    }
}

auto PostgresUserRepository::search(const UserSearchCriteria& criteria) -> std::vector<User>
{
    std::vector<User> users;

    try
    {
        if (criteria.exclude_user_id.has_value())
        {
            const auto result = db_->execSqlSync(
                "SELECT id, username, password_hash, is_admin, is_active, token_version, created_at "
                "FROM users WHERE username ILIKE $1 AND id <> $2 ORDER BY username ASC",
                "%" + criteria.query + "%",
                criteria.exclude_user_id.value());

            users.reserve(result.size());
            for (const auto& row : result)
            {
                users.push_back(mapUser(row));
            }

            return users;
        }

        const auto result = db_->execSqlSync(
            "SELECT id, username, password_hash, is_admin, is_active, token_version, created_at "
            "FROM users WHERE username ILIKE $1 ORDER BY username ASC",
            "%" + criteria.query + "%");

        users.reserve(result.size());
        for (const auto& row : result)
        {
            users.push_back(mapUser(row));
        }
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresUserRepository::search failed: " << exception.base().what();
    }

    return users;
}

auto PostgresUserRepository::create(const User& user) -> UUID
{
    const auto userId = user.id.empty() ? domain::generate_uuid() : user.id;
    const auto tokenVersion = user.token_version.empty() ? domain::generate_uuid() : user.token_version;

    const auto result = db_->execSqlSync(
        "INSERT INTO users (id, username, password_hash, is_admin, is_active, token_version) "
        "VALUES ($1, $2, $3, $4, $5, $6) RETURNING id",
        userId,
        user.username,
        user.password_hash,
        user.is_admin,
        user.is_active,
        tokenVersion);

    if (result.empty())
    {
        return {};
    }

    return result[0]["id"].as<std::string>();
}

auto PostgresUserRepository::update(const User& user) -> bool
{
    try
    {
        const auto result = db_->execSqlSync(
            "UPDATE users SET username = $1, password_hash = $2, is_admin = $3, is_active = $4, token_version = $5 WHERE id = $6",
            user.username,
            user.password_hash,
            user.is_admin,
            user.is_active,
            user.token_version,
            user.id);

        return result.affectedRows() > 0;
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresUserRepository::update failed: " << exception.base().what();
        return false;
    }
}

auto PostgresUserRepository::mapUser(const drogon::orm::Row& row) const -> User
{
    return User{
        .id = row["id"].as<std::string>(),
        .username = row["username"].as<std::string>(),
        .password_hash = row["password_hash"].as<std::string>(),
        .is_admin = row["is_admin"].as<bool>(),
        .is_active = row["is_active"].as<bool>(),
        .token_version = row["token_version"].as<std::string>(),
        .created_at = parseTimestamp(row["created_at"].as<std::string>()),
    };
}
} // namespace nexustal::infrastructure::adapters
