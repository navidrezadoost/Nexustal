#pragma once

#include <drogon/orm/DbClient.h>

#include "domain/endpoint_repository.hpp"

namespace nexustal::infrastructure::adapters
{
class PostgresEndpointRepository : public domain::endpoint::IEndpointRepository
{
public:
    explicit PostgresEndpointRepository(drogon::orm::DbClientPtr db);

    std::vector<domain::endpoint::Endpoint> findByProject(const domain::UUID& project_id) override;
    std::optional<domain::endpoint::Endpoint> findById(const domain::UUID& id) override;
    domain::UUID create(const domain::endpoint::Endpoint& endpoint) override;
    bool update(const domain::endpoint::Endpoint& endpoint) override;
    bool remove(const domain::UUID& id) override;
    std::vector<domain::endpoint::Endpoint> searchByTags(const domain::UUID& project_id,
                                                         const std::vector<domain::UUID>& tag_ids) override;

private:
    [[nodiscard]] auto mapBaseEndpoint(const drogon::orm::Row& row) const -> domain::endpoint::Endpoint;
    [[nodiscard]] auto loadDetails(const domain::UUID& endpoint_id,
                                   domain::endpoint::EndpointType type) const -> domain::endpoint::EndpointDetails;
    void insertTypeDetails(drogon::orm::TransactionPtr transaction,
                           const domain::UUID& endpoint_id,
                           const domain::endpoint::RestDetails& details) const;
    void insertTypeDetails(drogon::orm::TransactionPtr transaction,
                           const domain::UUID& endpoint_id,
                           const domain::endpoint::GraphqlDetails& details) const;
    void insertTypeDetails(drogon::orm::TransactionPtr transaction,
                           const domain::UUID& endpoint_id,
                           const domain::endpoint::SoapDetails& details) const;
    void insertTypeDetails(drogon::orm::TransactionPtr transaction,
                           const domain::UUID& endpoint_id,
                           const domain::endpoint::WebsocketDetails& details) const;
    void deleteTypeDetails(drogon::orm::TransactionPtr transaction,
                           const domain::UUID& endpoint_id,
                           domain::endpoint::EndpointType type) const;

    drogon::orm::DbClientPtr db_;
};
} // namespace nexustal::infrastructure::adapters
