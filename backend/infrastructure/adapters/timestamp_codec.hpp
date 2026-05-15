#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

#include "domain/common.hpp"

namespace nexustal::infrastructure::adapters
{
inline auto parseTimestamp(const std::string& value) -> nexustal::domain::Timestamp
{
    auto normalized = value;
    if (const auto timezonePosition = normalized.find_first_of(".+Z"); timezonePosition != std::string::npos)
    {
        normalized = normalized.substr(0, timezonePosition);
    }

    std::tm timeInfo{};
    std::istringstream input{normalized};
    input >> std::get_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
    if (input.fail())
    {
        return nexustal::domain::Timestamp{};
    }

    return std::chrono::system_clock::from_time_t(std::mktime(&timeInfo));
}

inline auto formatTimestamp(const nexustal::domain::Timestamp& value) -> std::string
{
    const auto rawTime = std::chrono::system_clock::to_time_t(value);
    std::tm timeInfo{};
#if defined(_WIN32)
    gmtime_s(&timeInfo, &rawTime);
#else
    gmtime_r(&rawTime, &timeInfo);
#endif

    std::ostringstream output;
    output << std::put_time(&timeInfo, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
}
} // namespace nexustal::infrastructure::adapters
