#pragma once

#include <cstdint>
#include <string_view>
#include <unordered_set>

#include "domain/identity.hpp"

namespace nexustal::domain::auth
{
enum class Capability : std::uint8_t
{
    ViewEndpoints = 0,
    EditEndpoints,
    DeleteEndpoints,
    PublishEndpoints,
    RunTests,
    ManageTeam,
};

struct CapabilityHash
{
    auto operator()(Capability capability) const noexcept -> std::size_t
    {
        return static_cast<std::size_t>(capability);
    }
};

inline auto capabilitiesForRole(identity::UserRole role) -> std::unordered_set<Capability, CapabilityHash>
{
    switch (role)
    {
    case identity::UserRole::Admin:
        return {
            Capability::ViewEndpoints,
            Capability::EditEndpoints,
            Capability::DeleteEndpoints,
            Capability::PublishEndpoints,
            Capability::RunTests,
            Capability::ManageTeam,
        };
    case identity::UserRole::BackendDev:
        return {
            Capability::ViewEndpoints,
            Capability::EditEndpoints,
            Capability::PublishEndpoints,
            Capability::RunTests,
        };
    case identity::UserRole::FrontendDev:
        return {
            Capability::ViewEndpoints,
            Capability::EditEndpoints,
            Capability::RunTests,
        };
    case identity::UserRole::Viewer:
    default:
        return {
            Capability::ViewEndpoints,
            Capability::RunTests,
        };
    }
}

inline auto roleFromString(std::string_view role) -> identity::UserRole
{
    if (role == "admin")
    {
        return identity::UserRole::Admin;
    }
    if (role == "backend_dev")
    {
        return identity::UserRole::BackendDev;
    }
    if (role == "frontend_dev")
    {
        return identity::UserRole::FrontendDev;
    }
    return identity::UserRole::Viewer;
}

inline auto toString(identity::UserRole role) -> std::string_view
{
    switch (role)
    {
    case identity::UserRole::Admin:
        return "admin";
    case identity::UserRole::BackendDev:
        return "backend_dev";
    case identity::UserRole::FrontendDev:
        return "frontend_dev";
    case identity::UserRole::Viewer:
    default:
        return "viewer";
    }
}

inline auto hasCapability(identity::UserRole role, Capability capability) -> bool
{
    return capabilitiesForRole(role).contains(capability);
}
} // namespace nexustal::domain::auth
