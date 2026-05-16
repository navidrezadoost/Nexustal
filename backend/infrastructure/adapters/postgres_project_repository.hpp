#pragma once

#include <drogon/orm/DbClient.h>

#include "domain/identity_repository.hpp"

namespace nexustal::infrastructure::adapters
{
class PostgresProjectRepository : public domain::identity::IProjectRepository
{
public:
    explicit PostgresProjectRepository(drogon::orm::DbClientPtr db);

    std::optional<domain::identity::UserRole> findRoleForUser(const domain::UUID& project_id,
                                                              const domain::UUID& user_id) override;
    bool addMember(const domain::UUID& project_id, const domain::UUID& user_id, domain::identity::UserRole role) override;

private:
    drogon::orm::DbClientPtr db_;
};
} // namespace nexustal::infrastructure::adapters
