#include "application/invitation_service.hpp"

#include <algorithm>
#include <chrono>

#include "domain/auth/capabilities.hpp"
#include "domain/common.hpp"

namespace nexustal::application
{
InvitationService::InvitationService(std::shared_ptr<domain::identity::IUserRepository> userRepository,
                                     std::shared_ptr<domain::identity::ITeamRepository> teamRepository,
                                     std::shared_ptr<domain::identity::IProjectRepository> projectRepository,
                                     std::shared_ptr<domain::identity::IInvitationRepository> invitationRepository,
                                     std::shared_ptr<domain::notification::INotificationService> notificationService)
    : userRepository_(std::move(userRepository))
    , teamRepository_(std::move(teamRepository))
    , projectRepository_(std::move(projectRepository))
    , invitationRepository_(std::move(invitationRepository))
    , notificationService_(std::move(notificationService))
{
}

auto InvitationService::sendInvitation(const domain::UUID& fromUser,
                                       const domain::UUID& toUser,
                                       const domain::UUID& projectId,
                                       domain::identity::UserRole role) -> domain::UUID
{
    if (!userRepository_->findById(fromUser).has_value() || !userRepository_->findById(toUser).has_value())
    {
        return {};
    }

    domain::identity::Invitation invitation{
        .id = domain::generate_uuid(),
        .from_user_id = fromUser,
        .to_user_id = toUser,
        .project_id = projectId,
        .team_id = std::nullopt,
        .role = role,
        .status = domain::identity::InvitationStatus::Pending,
        .created_at = std::chrono::system_clock::now(),
    };

    const auto createdId = invitationRepository_->create(invitation);
    if (!createdId.empty() && notificationService_)
    {
        notificationService_->sendNotification(domain::notification::Notification{
            .id = domain::generate_uuid(),
            .user_id = toUser,
            .title = "Invitation received",
            .message = "You have a new invitation waiting.",
            .type = "invitation",
            .is_read = false,
            .created_at = std::chrono::system_clock::now(),
        });
    }

    return createdId;
}

auto InvitationService::getPendingForUser(const domain::UUID& userId) const
    -> std::vector<domain::identity::Invitation>
{
    return invitationRepository_->findPendingForUser(userId);
}

auto InvitationService::accept(const domain::UUID& invitationId, const domain::UUID& userId) -> bool
{
    const auto pending = invitationRepository_->findPendingForUser(userId);
    const auto iterator = std::find_if(pending.begin(), pending.end(), [&](const auto& invitation) {
        return invitation.id == invitationId;
    });

    if (iterator == pending.end())
    {
        return false;
    }

    if (!invitationRepository_->updateStatus(invitationId, domain::identity::InvitationStatus::Accepted))
    {
        return false;
    }

    bool membershipPersisted = false;
    if (iterator->team_id.has_value() && teamRepository_)
    {
        membershipPersisted = teamRepository_->addMember(iterator->team_id.value(), userId, iterator->role);
    }
    else if (iterator->project_id.has_value() && projectRepository_)
    {
        membershipPersisted = projectRepository_->addMember(iterator->project_id.value(), userId, iterator->role);
    }

    if (!membershipPersisted)
    {
        return false;
    }

    if (notificationService_)
    {
        notificationService_->sendNotification(domain::notification::Notification{
            .id = domain::generate_uuid(),
            .user_id = iterator->from_user_id,
            .title = "Invitation accepted",
            .message = "A user accepted your invitation.",
            .type = "invitation_accepted",
            .is_read = false,
            .created_at = std::chrono::system_clock::now(),
        });
    }

    return true;
}

auto InvitationService::reject(const domain::UUID& invitationId, const domain::UUID& userId) -> bool
{
    const auto pending = invitationRepository_->findPendingForUser(userId);
    const auto iterator = std::find_if(pending.begin(), pending.end(), [&](const auto& invitation) {
        return invitation.id == invitationId;
    });

    if (iterator == pending.end())
    {
        return false;
    }

    return invitationRepository_->updateStatus(invitationId, domain::identity::InvitationStatus::Rejected);
}
} // namespace nexustal::application
