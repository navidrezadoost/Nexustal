#pragma once

#include <drogon/orm/DbClient.h>

#include "domain/identity_repository.hpp"

namespace nexustal::infrastructure::adapters
{
class PostgresTeamRepository : public domain::identity::ITeamRepository
{
public:
    explicit PostgresTeamRepository(drogon::orm::DbClientPtr db);

    domain::UUID create(const domain::identity::Team& team) override;
    std::optional<domain::identity::Team> findById(const domain::UUID& id) override;
    std::vector<domain::identity::Team> findByOwner(const domain::UUID& owner_id) override;
    bool addMember(const domain::UUID& team_id, const domain::UUID& user_id, domain::identity::UserRole role) override;

private:
    [[nodiscard]] auto mapTeam(const drogon::orm::Row& row) const -> domain::identity::Team;

    drogon::orm::DbClientPtr db_;
};
} // namespace nexustal::infrastructure::adapters
