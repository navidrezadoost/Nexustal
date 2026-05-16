#pragma once

#include <functional>
#include <string>

#include "infrastructure/sandbox/quickjs_sandbox.hpp"

namespace nexustal::infrastructure::sandbox
{
struct NtExecutionBindings
{
    std::function<void(const std::string&)> log;
    std::function<std::string(const std::string&)> environmentGet;
    std::function<void(const std::string&, const std::string&)> environmentSet;
};

void installNtApi(QuickJsSandbox& sandbox, const NtExecutionBindings& bindings);
} // namespace nexustal::infrastructure::sandbox