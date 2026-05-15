#pragma once

#include <drogon/orm/DbClient.h>

#include <filesystem>
#include <string>
#include <vector>

namespace nexustal::platform
{
struct MigrationStep
{
    std::string version;
    std::filesystem::path path;
};

class MigrationRunner
{
public:
    MigrationRunner(drogon::orm::DbClientPtr dbClient, std::filesystem::path migrationsRoot);

    [[nodiscard]] auto discover() const -> std::vector<MigrationStep>;
    [[nodiscard]] auto run() const -> std::size_t;
    [[nodiscard]] auto root() const -> const std::filesystem::path&;

private:
    void ensureMigrationTable() const;
    [[nodiscard]] auto appliedVersions() const -> std::vector<std::string>;
    void applyMigration(const MigrationStep& migration) const;

    drogon::orm::DbClientPtr dbClient_;
    std::filesystem::path migrationsRoot_;
};
} // namespace nexustal::platform
