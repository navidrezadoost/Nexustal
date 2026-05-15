#pragma once

#include <cstdint>
#include <string>

namespace nexustal::domain::wizard
{
struct WizardStatus
{
    enum class Step : std::uint8_t
    {
        Database = 0,
        Admin = 1,
        Company = 2,
        Complete = 3,
    };

    Step current_step = Step::Database;
    bool is_complete = false;
};

struct DatabaseConfig
{
    std::string host;
    int port = 5432;
    std::string dbname;
    std::string user;
    std::string password;
};

struct AdminCredentials
{
    std::string username;
    std::string password;
};

struct CompanyProfile
{
    std::string name;
    std::string activity_scope;
    std::string installation_id;
};

class IWizardService
{
public:
    virtual ~IWizardService() = default;

    virtual WizardStatus getStatus() = 0;
    virtual bool configureDatabase(const DatabaseConfig& config) = 0;
    virtual bool createAdmin(const AdminCredentials& credentials) = 0;
    virtual bool registerCompany(const CompanyProfile& profile) = 0;
};
} // namespace nexustal::domain::wizard
