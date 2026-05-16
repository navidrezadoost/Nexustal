#include "application/endpoint_service.hpp"

namespace nexustal::application
{
EndpointService::EndpointService(std::shared_ptr<domain::endpoint::IEndpointRepository> endpointRepository,
                                 std::shared_ptr<RoleService> roleService)
    : endpointRepository_(std::move(endpointRepository))
    , roleService_(std::move(roleService))
{
}

auto EndpointService::getProjectEndpoints(const domain::UUID& projectId, const domain::UUID& userId)
    -> std::vector<domain::endpoint::Endpoint>
{
    if (!roleService_->hasCapability(userId, projectId, domain::auth::Capability::ViewEndpoints))
    {
        return {};
    }

    return endpointRepository_->findByProject(projectId);
}

auto EndpointService::getEndpoint(const domain::UUID& endpointId, const domain::UUID& userId)
    -> std::optional<domain::endpoint::Endpoint>
{
    const auto endpoint = endpointRepository_->findById(endpointId);
    if (!endpoint.has_value())
    {
        return std::nullopt;
    }

    if (!roleService_->hasCapability(userId, endpoint->project_id, domain::auth::Capability::ViewEndpoints))
    {
        return std::nullopt;
    }

    return endpoint;
}

auto EndpointService::createEndpoint(const domain::UUID& projectId,
                                     domain::endpoint::Endpoint endpoint,
                                     const domain::UUID& userId) -> domain::UUID
{
    if (!roleService_->hasCapability(userId, projectId, domain::auth::Capability::EditEndpoints))
    {
        return {};
    }

    endpoint.project_id = projectId;
    endpoint.created_by = userId;
    return endpointRepository_->create(endpoint);
}

auto EndpointService::updateEndpoint(const domain::UUID& endpointId,
                                     domain::endpoint::Endpoint endpoint,
                                     const domain::UUID& userId) -> bool
{
    const auto current = endpointRepository_->findById(endpointId);
    if (!current.has_value())
    {
        return false;
    }

    if (!roleService_->hasCapability(userId, current->project_id, domain::auth::Capability::EditEndpoints))
    {
        return false;
    }

    endpoint.id = endpointId;
    endpoint.project_id = current->project_id;
    endpoint.created_by = current->created_by;
    endpoint.created_at = current->created_at;
    return endpointRepository_->update(endpoint);
}

auto EndpointService::deleteEndpoint(const domain::UUID& endpointId, const domain::UUID& userId) -> bool
{
    const auto current = endpointRepository_->findById(endpointId);
    if (!current.has_value())
    {
        return false;
    }

    return roleService_->hasCapability(userId, current->project_id, domain::auth::Capability::DeleteEndpoints) &&
           endpointRepository_->remove(endpointId);
}

auto EndpointService::publishEndpoint(const domain::UUID& endpointId, const domain::UUID& userId) -> bool
{
    const auto current = endpointRepository_->findById(endpointId);
    if (!current.has_value())
    {
        return false;
    }

    if (!roleService_->hasCapability(userId, current->project_id, domain::auth::Capability::PublishEndpoints))
    {
        return false;
    }

    auto updated = current.value();
    updated.status = "published";
    return endpointRepository_->update(updated);
}
} // namespace nexustal::application
