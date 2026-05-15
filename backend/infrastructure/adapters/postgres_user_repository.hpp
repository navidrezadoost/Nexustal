#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <drogon/orm/DbClient.h>

#include "domain/identity_repository.hpp"

namespace nexustal::infrastructure::adapters
{
class PostgresUserRepository : public domain::identity::IUserRepository
{
public:
    explicit PostgresUserRepository(drogon::orm::DbClientPtr db);

    std::optional<domain::identity::User> findByUsername(const std::string& username) override;
    std::optional<domain::identity::User> findById(const domain::UUID& id) override;
    std::vector<domain::identity::User> search(const domain::identity::UserSearchCriteria& criteria) override;
    domain::UUID create(const domain::identity::User& user) override;
    bool update(const domain::identity::User& user) override;

private:
    [[nodiscard]] auto mapUser(const drogon::orm::Row& row) const -> domain::identity::User;

    drogon::orm::DbClientPtr db_;
};
} // namespace nexustal::infrastructure::adapters
