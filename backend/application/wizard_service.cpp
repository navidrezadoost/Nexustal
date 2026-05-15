#include "application/wizard_service.hpp"

#include <chrono>

namespace nexustal::application
{
WizardService::WizardService(std::shared_ptr<domain::identity::IUserRepository> userRepository,
                             std::shared_ptr<domain::services::IPasswordHasher> passwordHasher)
    : userRepository_(std::move(userRepository))
    , passwordHasher_(std::move(passwordHasher))
{
}

auto WizardService::getStatus() -> domain::wizard::WizardStatus
{
    if (!configuredDatabase_.has_value())
    {
        return domain::wizard::WizardStatus{.current_step = domain::wizard::WizardStatus::Step::Database,
                                            .is_complete = false};
    }

    if (!adminCreated_)
    {
        return domain::wizard::WizardStatus{.current_step = domain::wizard::WizardStatus::Step::Admin,
                                            .is_complete = false};
    }

    if (!companyProfile_.has_value())
    {
        return domain::wizard::WizardStatus{.current_step = domain::wizard::WizardStatus::Step::Company,
                                            .is_complete = false};
    }

    return domain::wizard::WizardStatus{.current_step = domain::wizard::WizardStatus::Step::Complete,
                                        .is_complete = true};
}

bool WizardService::configureDatabase(const domain::wizard::DatabaseConfig& config)
{
    if (config.host.empty() || config.dbname.empty() || config.user.empty())
    {
        return false;
    }

    configuredDatabase_ = config;
    return true;
}

bool WizardService::createAdmin(const domain::wizard::AdminCredentials& credentials)
{
    if (credentials.username.empty() || credentials.password.empty())
    {
        return false;
    }

    if (userRepository_->findByUsername(credentials.username).has_value())
    {
        return false;
    }

    domain::identity::User admin;
    admin.id = domain::generate_uuid();
    admin.username = credentials.username;
    admin.password_hash = passwordHasher_->hash(credentials.password);
    admin.is_admin = true;
    admin.is_active = true;
    admin.token_version = domain::generate_uuid();
    admin.created_at = std::chrono::system_clock::now();

    adminCreated_ = !userRepository_->create(admin).empty();
    return adminCreated_;
}

bool WizardService::registerCompany(const domain::wizard::CompanyProfile& profile)
{
    if (profile.name.empty())
    {
        return false;
    }

    companyProfile_ = profile;
    return true;
}
} // namespace nexustal::application
