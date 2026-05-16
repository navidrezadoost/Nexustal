#pragma once

#include <chrono>
#include <functional>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "domain/common.hpp"

namespace nexustal::application::execution
{
class DynamicVariableEngine
{
public:
    [[nodiscard]] static auto resolve(const std::string& input) -> std::string
    {
        auto result = input;
        replaceAll(result, "{{$randomInt}}", [] {
            static thread_local std::mt19937 generator{std::random_device{}()};
            std::uniform_int_distribution<int> distribution{1000, 999999};
            return std::to_string(distribution(generator));
        });
        replaceAll(result, "{{$guid}}", [] {
            return domain::generate_uuid();
        });
        replaceAll(result, "{{$timestamp}}", [] {
            const auto now = std::chrono::system_clock::now();
            return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
        });
        replaceAll(result, "{{$isoTimestamp}}", [] {
            const auto now = std::chrono::system_clock::now();
            const auto raw = std::chrono::system_clock::to_time_t(now);
            std::tm timeInfo{};
#if defined(_WIN32)
            gmtime_s(&timeInfo, &raw);
#else
            gmtime_r(&raw, &timeInfo);
#endif
            std::ostringstream output;
            output << std::put_time(&timeInfo, "%Y-%m-%dT%H:%M:%SZ");
            return output.str();
        });
        replaceAll(result, "{{$randomEmail}}", [] {
            static thread_local std::mt19937 generator{std::random_device{}()};
            std::uniform_int_distribution<int> distribution{1000, 999999};
            return "user" + std::to_string(distribution(generator)) + "@example.test";
        });

        return result;
    }

    [[nodiscard]] static auto resolveJson(const nlohmann::json& input) -> nlohmann::json
    {
        if (input.is_string())
        {
            return resolve(input.get<std::string>());
        }

        if (input.is_array())
        {
            auto result = nlohmann::json::array();
            for (const auto& item : input)
            {
                result.push_back(resolveJson(item));
            }
            return result;
        }

        if (input.is_object())
        {
            auto result = nlohmann::json::object();
            for (auto iterator = input.begin(); iterator != input.end(); ++iterator)
            {
                result[iterator.key()] = resolveJson(iterator.value());
            }
            return result;
        }

        return input;
    }

private:
    static void replaceAll(std::string& input,
                           const std::string& token,
                           const std::function<std::string()>& generator)
    {
        std::size_t position = 0;
        while ((position = input.find(token, position)) != std::string::npos)
        {
            const auto replacement = generator();
            input.replace(position, token.size(), replacement);
            position += replacement.size();
        }
    }
};
} // namespace nexustal::application::execution