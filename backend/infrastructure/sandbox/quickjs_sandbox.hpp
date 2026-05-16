#pragma once

#include <functional>
#include <memory>
#include <string>

namespace nexustal::infrastructure::sandbox
{
class QuickJsSandbox
{
public:
    using NativeHandler = std::function<std::string(const std::string&)>;

    explicit QuickJsSandbox(int executionTimeoutMs = 5000);
    ~QuickJsSandbox();

    QuickJsSandbox(const QuickJsSandbox&) = delete;
    auto operator=(const QuickJsSandbox&) -> QuickJsSandbox& = delete;
    QuickJsSandbox(QuickJsSandbox&&) noexcept;
    auto operator=(QuickJsSandbox&&) noexcept -> QuickJsSandbox&;

    [[nodiscard]] auto execute(const std::string& script,
                               const std::string& contextJson = "{}") -> std::string;
    void registerFunction(const std::string& name, NativeHandler handler);
    void injectVariable(const std::string& name, const std::string& jsonValue);
    void setExecutionTimeout(int executionTimeoutMs);
    [[nodiscard]] auto isAvailable() const -> bool;

private:
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace nexustal::infrastructure::sandbox