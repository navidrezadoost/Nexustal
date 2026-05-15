#pragma once

#include <drogon/orm/DbClient.h>

#include "domain/identity_repository.hpp"

namespace nexustal::infrastructure::adapters
{
class PostgresInvitationRepository : public domain::identity::IInvitationRepository
{
public:
    explicit PostgresInvitationRepository(drogon::orm::DbClientPtr db);

    domain::UUID create(const domain::identity::Invitation& invitation) override;
    std::vector<domain::identity::Invitation> findPendingForUser(const domain::UUID& userId) override;
    bool updateStatus(const domain::UUID& invitationId, domain::identity::InvitationStatus status) override;

private:
    [[nodiscard]] auto mapInvitation(const drogon::orm::Row& row) const -> domain::identity::Invitation;
    [[nodiscard]] static auto statusToString(domain::identity::InvitationStatus status) -> std::string;
    [[nodiscard]] static auto statusFromString(const std::string& status) -> domain::identity::InvitationStatus;

    drogon::orm::DbClientPtr db_;
};
} // namespace nexustal::infrastructure::adapters
