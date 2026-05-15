#include "nexustal/platform/migration_runner.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace nexustal::platform
{
MigrationRunner::MigrationRunner(drogon::orm::DbClientPtr dbClient, std::filesystem::path migrationsRoot)
    : dbClient_(std::move(dbClient))
    , migrationsRoot_(std::move(migrationsRoot))
{
}

void MigrationRunner::ensureMigrationTable() const
{
    dbClient_->execSqlSync(
        "CREATE TABLE IF NOT EXISTS schema_migrations ("
        "version VARCHAR(255) PRIMARY KEY, "
        "applied_at TIMESTAMPTZ NOT NULL DEFAULT NOW())");
}

auto MigrationRunner::appliedVersions() const -> std::vector<std::string>
{
    ensureMigrationTable();

    std::vector<std::string> versions;
    const auto result = dbClient_->execSqlSync("SELECT version FROM schema_migrations ORDER BY version ASC");
    versions.reserve(result.size());

    for (const auto& row : result)
    {
        versions.push_back(row["version"].as<std::string>());
    }

    return versions;
}

auto MigrationRunner::discover() const -> std::vector<MigrationStep>
{
    if (!std::filesystem::exists(migrationsRoot_))
    {
        throw std::runtime_error("Migration directory not found: " + migrationsRoot_.string());
    }

    std::vector<MigrationStep> migrations;

    for (const auto& entry : std::filesystem::directory_iterator(migrationsRoot_))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".sql")
        {
            continue;
        }

        migrations.push_back(MigrationStep{
            .version = entry.path().stem().string(),
            .path = entry.path(),
        });
    }

    std::ranges::sort(migrations, [](const MigrationStep& left, const MigrationStep& right) {
        return left.version < right.version;
    });

    return migrations;
}

auto MigrationRunner::run() const -> std::size_t
{
    const auto migrations = discover();
    const auto alreadyApplied = appliedVersions();

    std::size_t appliedCount = 0;

    for (const auto& migration : migrations)
    {
        if (std::ranges::binary_search(alreadyApplied, migration.path.filename().string()))
        {
            continue;
        }

        applyMigration(migration);
        ++appliedCount;
    }

    return appliedCount;
}

void MigrationRunner::applyMigration(const MigrationStep& migration) const
{
    std::ifstream stream{migration.path};
    if (!stream.is_open())
    {
        throw std::runtime_error("Failed to open migration: " + migration.path.string());
    }

    const std::string contents{
        std::istreambuf_iterator<char>{stream},
        std::istreambuf_iterator<char>{},
    };

    if (contents.empty())
    {
        throw std::runtime_error("Migration is empty: " + migration.path.string());
    }

    dbClient_->execSqlSync("BEGIN");
    try
    {
        dbClient_->execSqlSync(contents);
        dbClient_->execSqlSync(
            "INSERT INTO schema_migrations (version) VALUES ($1)",
            migration.path.filename().string());
        dbClient_->execSqlSync("COMMIT");
    }
    catch (...)
    {
        try
        {
            dbClient_->execSqlSync("ROLLBACK");
        }
        catch (...)
        {
        }
        throw;
    }
}

auto MigrationRunner::root() const -> const std::filesystem::path&
{
    return migrationsRoot_;
}
} // namespace nexustal::platform
