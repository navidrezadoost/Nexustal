#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

#include "application/endpoint_service.hpp"
#include "domain/common.hpp"
#include "domain/endpoint_repository.hpp"
#include "domain/identity_repository.hpp"

namespace
{
class FakeEndpointRepository final : public nexustal::domain::endpoint::IEndpointRepository
{
public:
    std::vector<nexustal::domain::endpoint::Endpoint> findByProject(const nexustal::domain::UUID& project_id) override
    {
        std::vector<nexustal::domain::endpoint::Endpoint> result;
        for (const auto& endpoint : endpoints)
        {
            if (endpoint.project_id == project_id)
            {
                result.push_back(endpoint);
            }
        }
        return result;
    }

    std::optional<nexustal::domain::endpoint::Endpoint> findById(const nexustal::domain::UUID& id) override
    {
        for (const auto& endpoint : endpoints)
        {
            if (endpoint.id == id)
            {
                return endpoint;
            }
        }
        return std::nullopt;
    }

    nexustal::domain::UUID create(const nexustal::domain::endpoint::Endpoint& endpoint) override
    {
        endpoints.push_back(endpoint);
        return endpoint.id;
    }

    bool update(const nexustal::domain::endpoint::Endpoint& endpoint) override
    {
        for (auto& current : endpoints)
        {
            if (current.id == endpoint.id)
            {
                current = endpoint;
                return true;
            }
        }
        return false;
    }

    bool remove(const nexustal::domain::UUID& id) override
    {
        endpoints.erase(std::remove_if(endpoints.begin(), endpoints.end(), [&](const auto& endpoint) {
            return endpoint.id == id;
        }), endpoints.end());
        return true;
    }

    std::vector<nexustal::domain::endpoint::Endpoint> searchByTags(const nexustal::domain::UUID&,
                                                                   const std::vector<nexustal::domain::UUID>&) override
    {
        return {};
    }

    std::vector<nexustal::domain::endpoint::Endpoint> endpoints;
};

class FakeProjectRepository final : public nexustal::domain::identity::IProjectRepository
{
public:
    std::optional<nexustal::domain::identity::UserRole> findRoleForUser(const nexustal::domain::UUID&,
                                                                        const nexustal::domain::UUID&) override
    {
        return role;
    }

    bool addMember(const nexustal::domain::UUID&, const nexustal::domain::UUID&, nexustal::domain::identity::UserRole) override
    {
        return true;
    }

    nexustal::domain::identity::UserRole role = nexustal::domain::identity::UserRole::BackendDev;
};

auto makeEndpoint() -> nexustal::domain::endpoint::Endpoint
{
    nexustal::domain::endpoint::Endpoint endpoint;
    endpoint.id = nexustal::domain::generate_uuid();
    endpoint.project_id = "project-1";
    endpoint.created_by = "user-1";
    endpoint.name = nlohmann::json{{"en", "Get Users"}};
    endpoint.details = nexustal::domain::endpoint::RestDetails{.method = nexustal::domain::endpoint::HttpMethod::Get,
                                                               .url_path = "/api/users"};
    return endpoint;
}
} // namespace

TEST(EndpointServiceTest, BackendDeveloperCanCreateEndpoint)
{
    auto endpoints = std::make_shared<FakeEndpointRepository>();
    auto projects = std::make_shared<FakeProjectRepository>();
    auto roles = std::make_shared<nexustal::application::RoleService>(projects, nullptr);
    nexustal::application::EndpointService service{endpoints, roles};

    auto endpoint = makeEndpoint();
    const auto createdId = service.createEndpoint("project-1", endpoint, "user-1");

    EXPECT_EQ(createdId, endpoint.id);
}

TEST(EndpointServiceTest, ViewerCannotDeleteEndpoint)
{
    auto endpoints = std::make_shared<FakeEndpointRepository>();
    auto projects = std::make_shared<FakeProjectRepository>();
    projects->role = nexustal::domain::identity::UserRole::Viewer;
    auto roles = std::make_shared<nexustal::application::RoleService>(projects, nullptr);
    nexustal::application::EndpointService service{endpoints, roles};

    const auto endpoint = makeEndpoint();
    endpoints->endpoints.push_back(endpoint);

    EXPECT_FALSE(service.deleteEndpoint(endpoint.id, "user-1"));
}
