#pragma once

#include <drogon/orm/DbClient.h>

#include "domain/environment/environment_repository.hpp"

namespace nexustal::infrastructure::adapters
{
class PostgresEnvironmentRepository : public domain::environment::IEnvironmentRepository
{
public:
    explicit PostgresEnvironmentRepository(drogon::orm::DbClientPtr db);

    std::vector<domain::environment::Environment> findByProject(const domain::UUID& project_id) override;
    std::optional<domain::environment::Environment> findById(const domain::UUID& id) override;
    domain::UUID create(const domain::environment::Environment& env) override;
    bool update(const domain::environment::Environment& env) override;
    bool remove(const domain::UUID& id) override;
    domain::environment::GlobalVariables getGlobalVariables(const domain::UUID& project_id) override;
    bool setGlobalVariables(const domain::environment::GlobalVariables& globals) override;

private:
    [[nodiscard]] auto mapEnvironment(const drogon::orm::Row& row) const -> domain::environment::Environment;

    drogon::orm::DbClientPtr db_;
};
} // namespace nexustal::infrastructure::adapters