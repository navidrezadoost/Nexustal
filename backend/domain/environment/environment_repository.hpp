#pragma once

#include <optional>
#include <vector>

#include "domain/environment/environment.hpp"

namespace nexustal::domain::environment
{
class IEnvironmentRepository
{
public:
    virtual ~IEnvironmentRepository() = default;

    virtual std::vector<Environment> findByProject(const UUID& project_id) = 0;
    virtual std::optional<Environment> findById(const UUID& id) = 0;
    virtual UUID create(const Environment& env) = 0;
    virtual bool update(const Environment& env) = 0;
    virtual bool remove(const UUID& id) = 0;

    virtual GlobalVariables getGlobalVariables(const UUID& project_id) = 0;
    virtual bool setGlobalVariables(const GlobalVariables& globals) = 0;
};
} // namespace nexustal::domain::environment