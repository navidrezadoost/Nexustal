#include "infrastructure/sandbox/nt_api_bindings.hpp"

#include <nlohmann/json.hpp>

namespace nexustal::infrastructure::sandbox
{
void installNtApi(QuickJsSandbox& sandbox, const NtExecutionBindings& bindings)
{
    sandbox.injectVariable("__nt_tests", "[]");

    sandbox.registerFunction("__nt_log__", [bindings](const std::string& payload) {
        if (bindings.log)
        {
            const auto parsed = nlohmann::json::parse(payload, nullptr, false);
            bindings.log(parsed.is_discarded() ? payload : parsed.dump());
        }
        return std::string{"null"};
    });

    sandbox.registerFunction("__nt_env_get__", [bindings](const std::string& payload) {
        if (!bindings.environmentGet)
        {
            return std::string{"null"};
        }

        const auto parsed = nlohmann::json::parse(payload, nullptr, false);
        const auto key = parsed.is_object() ? parsed.value("key", std::string{}) : std::string{};
        const auto value = bindings.environmentGet(key);
        return value.empty() ? std::string{"null"} : value;
    });

    sandbox.registerFunction("__nt_env_set__", [bindings](const std::string& payload) {
        if (bindings.environmentSet)
        {
            const auto parsed = nlohmann::json::parse(payload, nullptr, false);
            const auto key = parsed.is_object() ? parsed.value("key", std::string{}) : std::string{};
            const auto value = parsed.is_object() && parsed.contains("value") ? parsed["value"].dump() : "null";
            bindings.environmentSet(key, value);
        }

        return std::string{"null"};
    });

    sandbox.execute(R"JS(
(() => {
  const testResults = globalThis.__nt_tests || [];
  globalThis.__nt_tests = testResults;

  const fail = (message) => {
    throw new Error(message);
  };

  const currentResponse = () => {
    if (globalThis.__nt_context && globalThis.__nt_context.response) {
      return globalThis.__nt_context.response;
    }
    return {};
  };

  const jsonPathLookup = (value, path) => {
    if (!path) {
      return value;
    }

    return path.split('.').filter(Boolean).reduce((current, part) => {
      if (current === undefined || current === null) {
        return undefined;
      }
      return current[part];
    }, value);
  };

  globalThis.nt = {
    test(name, fn) {
      try {
        fn();
        testResults.push({ name, status: "passed" });
      } catch (error) {
        const message = String(error && error.message ? error.message : error);
        testResults.push({ name, status: "failed", message });
        return false;
      }
      return true;
    },
    console: {
      log(...parts) {
        return globalThis.__nt_log__(parts);
      }
    },
    environment: {
      get(key) {
        return globalThis.__nt_env_get__({ key });
      },
      set(key, value) {
        return globalThis.__nt_env_set__({ key, value });
      }
    },
    expect(actual) {
      return {
        to: {
          equal(expected) {
            if (actual !== expected) {
              fail(`Expected ${JSON.stringify(actual)} to equal ${JSON.stringify(expected)}`);
            }
          }
        }
      };
    },
    response: {
      to: {
        have: {
          status(code) {
            const response = currentResponse();
            if (response.status !== code) {
              fail(`Expected response status ${code} but got ${response.status}`);
            }
          },
          header(key, value) {
            const response = currentResponse();
            const headers = response.headers || {};
            const actual = headers[key] ?? headers[String(key).toLowerCase()];
            if (actual !== value) {
              fail(`Expected header ${key} to equal ${JSON.stringify(value)} but got ${JSON.stringify(actual)}`);
            }
          },
          jsonBody(path, value) {
            const response = currentResponse();
            const actual = jsonPathLookup(response.jsonBody || {}, path);
            if (JSON.stringify(actual) !== JSON.stringify(value)) {
              fail(`Expected response json at ${path} to equal ${JSON.stringify(value)} but got ${JSON.stringify(actual)}`);
            }
          },
          responseTimeLessThan(ms) {
            const response = currentResponse();
            const actual = response.responseTimeMs ?? Number.MAX_SAFE_INTEGER;
            if (actual >= ms) {
              fail(`Expected response time less than ${ms} but got ${actual}`);
            }
          }
        }
      }
    }
  };
})();
)JS");
}
} // namespace nexustal::infrastructure::sandbox