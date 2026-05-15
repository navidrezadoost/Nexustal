#pragma once

#include <memory>
#include <optional>

#include "domain/identity_repository.hpp"
#include "domain/password_hasher.hpp"
#include "domain/wizard_contracts.hpp"

namespace nexustal::application
{
class WizardService : public domain::wizard::IWizardService
{
public:
    WizardService(std::shared_ptr<domain::identity::IUserRepository> userRepository,
                  std::shared_ptr<domain::services::IPasswordHasher> passwordHasher);

    domain::wizard::WizardStatus getStatus() override;
    bool configureDatabase(const domain::wizard::DatabaseConfig& config) override;
    bool createAdmin(const domain::wizard::AdminCredentials& credentials) override;
    bool registerCompany(const domain::wizard::CompanyProfile& profile) override;

private:
    std::shared_ptr<domain::identity::IUserRepository> userRepository_;
    std::shared_ptr<domain::services::IPasswordHasher> passwordHasher_;
    std::optional<domain::wizard::DatabaseConfig> configuredDatabase_;
    std::optional<domain::wizard::CompanyProfile> companyProfile_;
    bool adminCreated_ = false;
};
} // namespace nexustal::application
