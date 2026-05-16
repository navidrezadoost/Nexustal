#pragma once

#include <memory>
#include <vector>

#include "domain/identity_repository.hpp"
#include "domain/notification_port.hpp"

namespace nexustal::application
{
class InvitationService
{
public:
    InvitationService(std::shared_ptr<domain::identity::IUserRepository> userRepository,
                      std::shared_ptr<domain::identity::ITeamRepository> teamRepository,
                      std::shared_ptr<domain::identity::IProjectRepository> projectRepository,
                      std::shared_ptr<domain::identity::IInvitationRepository> invitationRepository,
                      std::shared_ptr<domain::notification::INotificationService> notificationService);

    [[nodiscard]] auto sendInvitation(const domain::UUID& fromUser,
                                      const domain::UUID& toUser,
                                      const domain::UUID& projectId,
                                      domain::identity::UserRole role)
        -> domain::UUID;

    [[nodiscard]] auto getPendingForUser(const domain::UUID& userId) const
        -> std::vector<domain::identity::Invitation>;

    auto accept(const domain::UUID& invitationId, const domain::UUID& userId) -> bool;
    auto reject(const domain::UUID& invitationId, const domain::UUID& userId) -> bool;

private:
    std::shared_ptr<domain::identity::IUserRepository> userRepository_;
    std::shared_ptr<domain::identity::ITeamRepository> teamRepository_;
    std::shared_ptr<domain::identity::IProjectRepository> projectRepository_;
    std::shared_ptr<domain::identity::IInvitationRepository> invitationRepository_;
    std::shared_ptr<domain::notification::INotificationService> notificationService_;
};
} // namespace nexustal::application
