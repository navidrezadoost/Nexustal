#include "application/auth_service.hpp"
#include "application/endpoint_service.hpp"
#include "application/invitation_service.hpp"
#include "application/role_service.hpp"
#include "application/wizard_service.hpp"
#include "controllers/auth_controller.hpp"
#include "controllers/endpoint_controller.hpp"
#include "controllers/invitation_controller.hpp"
#include "controllers/sse_controller.hpp"
#include "controllers/wizard_controller.hpp"
#include "filters/jwt_auth_filter.hpp"
#include "infrastructure/adapters/bcrypt_password_hasher.hpp"
#include "infrastructure/adapters/jwt_token_service.hpp"
#include "infrastructure/adapters/postgres_endpoint_repository.hpp"
#include "infrastructure/adapters/postgres_invitation_repository.hpp"
#include "infrastructure/adapters/postgres_project_repository.hpp"
#include "infrastructure/adapters/postgres_team_repository.hpp"
#include "infrastructure/adapters/postgres_user_repository.hpp"
#include "infrastructure/adapters/redis_notification_service.hpp"
#include "infrastructure/adapters/sse_connection_manager.hpp"
#include "nexustal/platform/migration_runner.hpp"

#include <cstdlib>
#include <drogon/drogon.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string_view>

namespace
{
auto env_or(std::string_view key, std::string_view fallback) -> std::string
{
    if (const auto* value = std::getenv(key.data()); value != nullptr)
    {
        return value;
    }

    return std::string{fallback};
}
} // namespace

int main()
{
    const auto installMode = env_or("INSTALL_MODE", "false");
    const auto httpPort = env_or("NEXUSTAL_HTTP_PORT", "8080");
    const auto wsPort = env_or("NEXUSTAL_WS_PORT", "9001");
    const auto dbHost = env_or("DB_HOST", "localhost");
    const auto dbPort = env_or("DB_PORT", "5432");
    const auto dbName = env_or("DB_NAME", "nexustal");
    const auto dbUser = env_or("DB_USER", "nexustal");
    const auto dbPassword = env_or("DB_PASSWORD", "nexustal_dev");
    const auto jwtSecret = env_or("JWT_SECRET", "development-only-secret-change-me");
    const auto redisHost = env_or("REDIS_HOST", "localhost");
    const auto redisPort = env_or("REDIS_PORT", "6379");

    try
    {
        const auto connectionInfo = "host=" + dbHost + " port=" + dbPort + " dbname=" + dbName +
                                    " user=" + dbUser + " password=" + dbPassword;

        auto dbClient = drogon::orm::DbClient::newPgClient(connectionInfo, 1);
        const auto migrationsRoot = std::filesystem::path{"migrations"};
        nexustal::platform::MigrationRunner runner{dbClient, migrationsRoot};
        const auto appliedMigrations = runner.run();
        const auto migrations = runner.discover();

        std::cout << "Nexustal engine bootstrap" << '\n';
        std::cout << "  install mode: " << installMode << '\n';
        std::cout << "  http port:    " << httpPort << '\n';
        std::cout << "  ws port:      " << wsPort << '\n';
        std::cout << "  db host:      " << dbHost << ':' << dbPort << '/' << dbName << '\n';
        std::cout << "  migrations:   " << appliedMigrations << " file(s) applied from "
                  << runner.root().string() << '\n';

        for (const auto& migration : migrations)
        {
            std::cout << "    - " << migration.version << " => " << migration.path.filename().string()
                      << '\n';
        }

        auto userRepository = std::make_shared<nexustal::infrastructure::adapters::PostgresUserRepository>(dbClient);
        auto teamRepository = std::make_shared<nexustal::infrastructure::adapters::PostgresTeamRepository>(dbClient);
        auto projectRepository = std::make_shared<nexustal::infrastructure::adapters::PostgresProjectRepository>(dbClient);
        auto invitationRepository =
            std::make_shared<nexustal::infrastructure::adapters::PostgresInvitationRepository>(dbClient);
        auto endpointRepository =
            std::make_shared<nexustal::infrastructure::adapters::PostgresEndpointRepository>(dbClient);
        auto passwordHasher =
            std::make_shared<nexustal::infrastructure::adapters::BCryptPasswordHasher>();
        drogon::nosql::RedisClientPtr redisClient;
        std::shared_ptr<nexustal::domain::notification::INotificationService> notificationService;
        std::shared_ptr<nexustal::infrastructure::adapters::SseConnectionManager> sseConnectionManager;

        try
        {
            redisClient = drogon::nosql::RedisClient::newRedisClient(redisHost, std::stoi(redisPort));
            sseConnectionManager =
                std::make_shared<nexustal::infrastructure::adapters::SseConnectionManager>(redisClient);
            nexustal::controllers::SseController::configure(sseConnectionManager);
            notificationService =
                std::make_shared<nexustal::infrastructure::adapters::RedisNotificationService>(redisClient);
        }
        catch (const std::exception& error)
        {
            std::cerr << "  warning:      redis bootstrap skipped: " << error.what() << '\n';
        }

        auto tokenService = std::make_shared<nexustal::infrastructure::adapters::JwtTokenService>(
            jwtSecret,
            redisClient,
            userRepository);
        auto authService = std::make_shared<nexustal::application::AuthService>(
            userRepository,
            passwordHasher,
            tokenService);
        auto roleService = std::make_shared<nexustal::application::RoleService>(projectRepository, redisClient);
        auto endpointService =
            std::make_shared<nexustal::application::EndpointService>(endpointRepository, roleService);
        auto invitationService = std::make_shared<nexustal::application::InvitationService>(
            userRepository,
            teamRepository,
            projectRepository,
            invitationRepository,
            notificationService);
        auto wizardService =
            std::make_shared<nexustal::application::WizardService>(userRepository, passwordHasher);

        nexustal::controllers::AuthController::configure(authService);
        nexustal::controllers::EndpointController::configure(endpointService);
        nexustal::controllers::InvitationController::configure(invitationService);
        nexustal::controllers::WizardController::configure(wizardService);
        nexustal::filters::JwtAuthFilter::configure(authService, userRepository, redisClient);

        drogon::app().addListener("0.0.0.0", static_cast<uint16_t>(std::stoi(httpPort)));
        std::cout << "  status:       starting HTTP listener on " << httpPort << '\n';
        drogon::app().run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "Bootstrap failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
