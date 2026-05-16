#pragma once

#include <optional>
#include <vector>

#include "domain/endpoint.hpp"

namespace nexustal::domain::endpoint
{
class IEndpointRepository
{
public:
    virtual ~IEndpointRepository() = default;

    virtual std::vector<Endpoint> findByProject(const UUID& project_id) = 0;
    virtual std::optional<Endpoint> findById(const UUID& id) = 0;
    virtual UUID create(const Endpoint& endpoint) = 0;
    virtual bool update(const Endpoint& endpoint) = 0;
    virtual bool remove(const UUID& id) = 0;
    virtual std::vector<Endpoint> searchByTags(const UUID& project_id, const std::vector<UUID>& tag_ids) = 0;
};
} // namespace nexustal::domain::endpoint
