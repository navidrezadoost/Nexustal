#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "application/role_service.hpp"
#include "domain/endpoint_repository.hpp"

namespace nexustal::application
{
class EndpointService
{
public:
    EndpointService(std::shared_ptr<domain::endpoint::IEndpointRepository> endpointRepository,
                    std::shared_ptr<RoleService> roleService);

    [[nodiscard]] auto getProjectEndpoints(const domain::UUID& projectId, const domain::UUID& userId)
        -> std::vector<domain::endpoint::Endpoint>;
    [[nodiscard]] auto getEndpoint(const domain::UUID& endpointId, const domain::UUID& userId)
        -> std::optional<domain::endpoint::Endpoint>;
    [[nodiscard]] auto createEndpoint(const domain::UUID& projectId,
                                      domain::endpoint::Endpoint endpoint,
                                      const domain::UUID& userId) -> domain::UUID;
    auto updateEndpoint(const domain::UUID& endpointId,
                        domain::endpoint::Endpoint endpoint,
                        const domain::UUID& userId) -> bool;
    auto deleteEndpoint(const domain::UUID& endpointId, const domain::UUID& userId) -> bool;
    auto publishEndpoint(const domain::UUID& endpointId, const domain::UUID& userId) -> bool;

private:
    std::shared_ptr<domain::endpoint::IEndpointRepository> endpointRepository_;
    std::shared_ptr<RoleService> roleService_;
};
} // namespace nexustal::application
