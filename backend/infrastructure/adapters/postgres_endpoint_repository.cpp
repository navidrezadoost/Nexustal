#include "infrastructure/adapters/postgres_endpoint_repository.hpp"

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>
#include <nlohmann/json.hpp>

#include "infrastructure/adapters/timestamp_codec.hpp"

namespace nexustal::infrastructure::adapters
{
namespace
{
using domain::endpoint::Endpoint;
using domain::endpoint::EndpointType;
using domain::endpoint::GraphqlDetails;
using domain::endpoint::RestDetails;
using domain::endpoint::SoapDetails;
using domain::endpoint::WebsocketDetails;

auto parseJsonField(const drogon::orm::Row& row, const char* fieldName, nlohmann::json fallback) -> nlohmann::json
{
    if (row[fieldName].isNull())
    {
        return fallback;
    }

    return nlohmann::json::parse(row[fieldName].as<std::string>());
}
} // namespace

PostgresEndpointRepository::PostgresEndpointRepository(drogon::orm::DbClientPtr db)
    : db_(std::move(db))
{
}

auto PostgresEndpointRepository::findByProject(const domain::UUID& project_id) -> std::vector<Endpoint>
{
    std::vector<Endpoint> endpoints;

    try
    {
        const auto result = db_->execSqlSync(
            "SELECT id, project_id, name, description, type, status, created_by, created_at, updated_at "
            "FROM endpoints WHERE project_id = $1 ORDER BY updated_at DESC",
            project_id);

        endpoints.reserve(result.size());
        for (const auto& row : result)
        {
            auto endpoint = mapBaseEndpoint(row);
            endpoint.details = loadDetails(endpoint.id, endpoint.type);
            endpoints.push_back(std::move(endpoint));
        }
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresEndpointRepository::findByProject failed: " << exception.base().what();
    }

    return endpoints;
}

auto PostgresEndpointRepository::findById(const domain::UUID& id) -> std::optional<Endpoint>
{
    try
    {
        const auto result = db_->execSqlSync(
            "SELECT id, project_id, name, description, type, status, created_by, created_at, updated_at "
            "FROM endpoints WHERE id = $1",
            id);

        if (result.empty())
        {
            return std::nullopt;
        }

        auto endpoint = mapBaseEndpoint(result[0]);
        endpoint.details = loadDetails(endpoint.id, endpoint.type);
        return endpoint;
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresEndpointRepository::findById failed: " << exception.base().what();
        return std::nullopt;
    }
}

auto PostgresEndpointRepository::create(const Endpoint& endpoint) -> domain::UUID
{
    try
    {
        auto transaction = db_->newTransaction();
        const auto result = transaction->execSqlSync(
            "INSERT INTO endpoints (id, project_id, name, description, type, status, created_by) "
            "VALUES ($1, $2, $3::jsonb, $4::jsonb, $5, $6, $7) RETURNING id",
            endpoint.id.empty() ? domain::generate_uuid() : endpoint.id,
            endpoint.project_id,
            endpoint.name.dump(),
            endpoint.description.dump(),
            domain::endpoint::endpointTypeToString(endpoint.type),
            endpoint.status,
            endpoint.created_by);

        const auto createdId = result[0]["id"].as<std::string>();
        std::visit([&](const auto& details) { insertTypeDetails(transaction, createdId, details); }, endpoint.details);
        transaction->commit();
        return createdId;
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresEndpointRepository::create failed: " << exception.base().what();
        return {};
    }
}

auto PostgresEndpointRepository::update(const Endpoint& endpoint) -> bool
{
    try
    {
        auto transaction = db_->newTransaction();
        const auto existing = findById(endpoint.id);
        if (!existing.has_value())
        {
            return false;
        }

        transaction->execSqlSync(
            "UPDATE endpoints SET name = $1::jsonb, description = $2::jsonb, type = $3, status = $4, updated_at = NOW() "
            "WHERE id = $5",
            endpoint.name.dump(),
            endpoint.description.dump(),
            domain::endpoint::endpointTypeToString(endpoint.type),
            endpoint.status,
            endpoint.id);

        deleteTypeDetails(transaction, endpoint.id, existing->type);
        std::visit([&](const auto& details) { insertTypeDetails(transaction, endpoint.id, details); }, endpoint.details);
        transaction->commit();
        return true;
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresEndpointRepository::update failed: " << exception.base().what();
        return false;
    }
}

auto PostgresEndpointRepository::remove(const domain::UUID& id) -> bool
{
    try
    {
        const auto result = db_->execSqlSync("DELETE FROM endpoints WHERE id = $1", id);
        return result.affectedRows() > 0;
    }
    catch (const drogon::orm::DrogonDbException& exception)
    {
        LOG_ERROR << "PostgresEndpointRepository::remove failed: " << exception.base().what();
        return false;
    }
}

auto PostgresEndpointRepository::searchByTags(const domain::UUID& project_id,
                                              const std::vector<domain::UUID>& tag_ids)
    -> std::vector<Endpoint>
{
    (void)project_id;
    (void)tag_ids;
    return {};
}

auto PostgresEndpointRepository::mapBaseEndpoint(const drogon::orm::Row& row) const -> Endpoint
{
    return Endpoint{
        .id = row["id"].as<std::string>(),
        .project_id = row["project_id"].as<std::string>(),
        .name = parseJsonField(row, "name", nlohmann::json::object()),
        .description = parseJsonField(row, "description", nlohmann::json::object()),
        .type = domain::endpoint::endpointTypeFromString(row["type"].as<std::string>()),
        .status = row["status"].as<std::string>(),
        .created_by = row["created_by"].as<std::string>(),
        .created_at = parseTimestamp(row["created_at"].as<std::string>()),
        .updated_at = parseTimestamp(row["updated_at"].as<std::string>()),
    };
}

auto PostgresEndpointRepository::loadDetails(const domain::UUID& endpoint_id,
                                             EndpointType type) const -> domain::endpoint::EndpointDetails
{
    switch (type)
    {
    case EndpointType::Graphql:
    {
        const auto result = db_->execSqlSync(
            "SELECT query, variables, schema_url FROM graphql_endpoints WHERE endpoint_id = $1",
            endpoint_id);
        if (result.empty())
        {
            return GraphqlDetails{};
        }

        return GraphqlDetails{
            .query = result[0]["query"].as<std::string>(),
            .variables = parseJsonField(result[0], "variables", nlohmann::json::object()),
            .schema_url = result[0]["schema_url"].isNull()
                              ? std::nullopt
                              : std::optional<std::string>{result[0]["schema_url"].as<std::string>()},
        };
    }
    case EndpointType::Soap:
    {
        const auto result = db_->execSqlSync(
            "SELECT wsdl_url, operation, request_xml FROM soap_endpoints WHERE endpoint_id = $1",
            endpoint_id);
        if (result.empty())
        {
            return SoapDetails{};
        }

        return SoapDetails{
            .wsdl_url = result[0]["wsdl_url"].as<std::string>(),
            .operation = result[0]["operation"].as<std::string>(),
            .request_xml = result[0]["request_xml"].as<std::string>(),
        };
    }
    case EndpointType::Websocket:
    {
        const auto result = db_->execSqlSync(
            "SELECT connection_url, protocols, sample_messages FROM websocket_endpoints WHERE endpoint_id = $1",
            endpoint_id);
        if (result.empty())
        {
            return WebsocketDetails{};
        }

        return WebsocketDetails{
            .connection_url = result[0]["connection_url"].as<std::string>(),
            .protocols = parseJsonField(result[0], "protocols", nlohmann::json::array()).get<std::vector<std::string>>(),
            .sample_messages = parseJsonField(result[0], "sample_messages", nlohmann::json::array()),
        };
    }
    case EndpointType::Rest:
    default:
    {
        const auto result = db_->execSqlSync(
            "SELECT method, url_path, query_params, headers, body FROM rest_endpoints WHERE endpoint_id = $1",
            endpoint_id);
        if (result.empty())
        {
            return RestDetails{};
        }

        const auto body = parseJsonField(result[0], "body", nlohmann::json::object());
        return RestDetails{
            .method = domain::endpoint::httpMethodFromString(result[0]["method"].as<std::string>()),
            .url_path = result[0]["url_path"].as<std::string>(),
            .query_params = parseJsonField(result[0], "query_params", nlohmann::json::array()),
            .headers = parseJsonField(result[0], "headers", nlohmann::json::array()),
            .body = body,
            .body_raw = body.contains("raw") ? body["raw"].get<std::string>() : std::string{},
        };
    }
    }
}

void PostgresEndpointRepository::insertTypeDetails(drogon::orm::TransactionPtr transaction,
                                                   const domain::UUID& endpoint_id,
                                                   const RestDetails& details) const
{
    auto body = details.body;
    if (!details.body_raw.empty())
    {
        body["raw"] = details.body_raw;
    }

    transaction->execSqlSync(
        "INSERT INTO rest_endpoints (endpoint_id, method, url_path, query_params, headers, body) "
        "VALUES ($1, $2, $3, $4::jsonb, $5::jsonb, $6::jsonb)",
        endpoint_id,
        domain::endpoint::httpMethodToString(details.method),
        details.url_path,
        details.query_params.dump(),
        details.headers.dump(),
        body.dump());
}

void PostgresEndpointRepository::insertTypeDetails(drogon::orm::TransactionPtr transaction,
                                                   const domain::UUID& endpoint_id,
                                                   const GraphqlDetails& details) const
{
    transaction->execSqlSync(
        "INSERT INTO graphql_endpoints (endpoint_id, query, variables, schema_url) VALUES ($1, $2, $3::jsonb, $4)",
        endpoint_id,
        details.query,
        details.variables.dump(),
        details.schema_url);
}

void PostgresEndpointRepository::insertTypeDetails(drogon::orm::TransactionPtr transaction,
                                                   const domain::UUID& endpoint_id,
                                                   const SoapDetails& details) const
{
    transaction->execSqlSync(
        "INSERT INTO soap_endpoints (endpoint_id, wsdl_url, operation, request_xml) VALUES ($1, $2, $3, $4)",
        endpoint_id,
        details.wsdl_url,
        details.operation,
        details.request_xml);
}

void PostgresEndpointRepository::insertTypeDetails(drogon::orm::TransactionPtr transaction,
                                                   const domain::UUID& endpoint_id,
                                                   const WebsocketDetails& details) const
{
    transaction->execSqlSync(
        "INSERT INTO websocket_endpoints (endpoint_id, connection_url, protocols, sample_messages) "
        "VALUES ($1, $2, $3::jsonb, $4::jsonb)",
        endpoint_id,
        details.connection_url,
        nlohmann::json(details.protocols).dump(),
        details.sample_messages.dump());
}

void PostgresEndpointRepository::deleteTypeDetails(drogon::orm::TransactionPtr transaction,
                                                   const domain::UUID& endpoint_id,
                                                   EndpointType type) const
{
    switch (type)
    {
    case EndpointType::Graphql:
        transaction->execSqlSync("DELETE FROM graphql_endpoints WHERE endpoint_id = $1", endpoint_id);
        break;
    case EndpointType::Soap:
        transaction->execSqlSync("DELETE FROM soap_endpoints WHERE endpoint_id = $1", endpoint_id);
        break;
    case EndpointType::Websocket:
        transaction->execSqlSync("DELETE FROM websocket_endpoints WHERE endpoint_id = $1", endpoint_id);
        break;
    case EndpointType::Rest:
    default:
        transaction->execSqlSync("DELETE FROM rest_endpoints WHERE endpoint_id = $1", endpoint_id);
        break;
    }
}
} // namespace nexustal::infrastructure::adapters
