#include "infrastructure/sandbox/quickjs_sandbox.hpp"

#include <chrono>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include <nlohmann/json.hpp>

#if defined(NEXUSTAL_HAS_QUICKJS)
#include <quickjs.h>
#endif

namespace nexustal::infrastructure::sandbox
{
struct QuickJsSandbox::State
{
    explicit State(int timeoutMs)
        : execution_timeout_ms(timeoutMs)
    {
    }

    int execution_timeout_ms = 5000;
    std::unordered_map<std::string, NativeHandler> handlers;
    std::unordered_map<std::string, std::string> injected_variables;

#if defined(NEXUSTAL_HAS_QUICKJS)
    JSRuntime* runtime = nullptr;
    JSContext* context = nullptr;
    std::chrono::steady_clock::time_point execution_started_at{};
#endif
};

#if defined(NEXUSTAL_HAS_QUICKJS)
namespace
{
auto jsonStringify(JSContext* context, JSValueConst value) -> std::string
{
    JSValue json = JS_JSONStringify(context, JS_DupValue(context, value), JS_UNDEFINED, JS_UNDEFINED);
    if (JS_IsException(json))
    {
        JS_FreeValue(context, json);
        return "null";
    }

    const char* encoded = JS_ToCString(context, json);
    const std::string result = encoded != nullptr ? encoded : "null";
    if (encoded != nullptr)
    {
        JS_FreeCString(context, encoded);
    }
    JS_FreeValue(context, json);
    return result;
}

auto exceptionMessage(JSContext* context) -> std::string
{
    JSValue exception = JS_GetException(context);
    const auto message = jsonStringify(context, exception);
    JS_FreeValue(context, exception);
    return message;
}

void setGlobalJsonValue(JSContext* context, const std::string& name, const std::string& jsonValue)
{
    JSValue parsed = JS_ParseJSON(context, jsonValue.c_str(), jsonValue.size(), name.c_str());
    if (JS_IsException(parsed))
    {
        JS_FreeValue(context, JS_GetException(context));
        parsed = JS_NewStringLen(context, jsonValue.data(), jsonValue.size());
    }

    JSValue global = JS_GetGlobalObject(context);
    JS_SetPropertyStr(context, global, name.c_str(), parsed);
    JS_FreeValue(context, global);
}

auto interruptHandler(JSRuntime*, void* opaque) -> int
{
    auto* state = static_cast<QuickJsSandbox::State*>(opaque);
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - state->execution_started_at).count();
    return elapsed > state->execution_timeout_ms ? 1 : 0;
}

auto invokeNative(JSContext* context,
                  JSValueConst,
                  int argc,
                  JSValueConst* argv) -> JSValue
{
    auto* state = static_cast<QuickJsSandbox::State*>(JS_GetRuntimeOpaque(JS_GetRuntime(context)));
    if (state == nullptr || argc < 1)
    {
        return JS_ThrowInternalError(context, "sandbox state is not initialized");
    }

    const char* nameCString = JS_ToCString(context, argv[0]);
    const char* payloadCString = argc >= 2 ? JS_ToCString(context, argv[1]) : nullptr;
    const std::string name = nameCString != nullptr ? nameCString : "";
    const std::string payload = payloadCString != nullptr ? payloadCString : "null";

    if (nameCString != nullptr)
    {
        JS_FreeCString(context, nameCString);
    }
    if (payloadCString != nullptr)
    {
        JS_FreeCString(context, payloadCString);
    }

    const auto handler = state->handlers.find(name);
    if (handler == state->handlers.end())
    {
        return JS_ThrowReferenceError(context, "native handler is not registered: %s", name.c_str());
    }

    try
    {
        const auto result = handler->second(payload);
        return JS_NewStringLen(context, result.data(), result.size());
    }
    catch (const std::exception& error)
    {
        return JS_ThrowInternalError(context, "%s", error.what());
    }
}
} // namespace
#endif

QuickJsSandbox::QuickJsSandbox(int executionTimeoutMs)
    : state_(std::make_unique<State>(executionTimeoutMs))
{
#if defined(NEXUSTAL_HAS_QUICKJS)
    state_->runtime = JS_NewRuntime();
    if (state_->runtime == nullptr)
    {
        throw std::runtime_error("failed to create QuickJS runtime");
    }

    state_->context = JS_NewContext(state_->runtime);
    if (state_->context == nullptr)
    {
        JS_FreeRuntime(state_->runtime);
        throw std::runtime_error("failed to create QuickJS context");
    }

    JS_SetRuntimeOpaque(state_->runtime, state_.get());
    JS_SetInterruptHandler(state_->runtime, interruptHandler, state_.get());

    JSValue global = JS_GetGlobalObject(state_->context);
    JS_SetPropertyStr(
        state_->context,
        global,
        "__nexustal_call",
        JS_NewCFunction(state_->context, invokeNative, "__nexustal_call", 2));
    JS_FreeValue(state_->context, global);
#endif
}

QuickJsSandbox::~QuickJsSandbox()
{
#if defined(NEXUSTAL_HAS_QUICKJS)
    if (state_ != nullptr)
    {
        if (state_->context != nullptr)
        {
            JS_FreeContext(state_->context);
        }
        if (state_->runtime != nullptr)
        {
            JS_FreeRuntime(state_->runtime);
        }
    }
#endif
}

QuickJsSandbox::QuickJsSandbox(QuickJsSandbox&&) noexcept = default;

auto QuickJsSandbox::operator=(QuickJsSandbox&&) noexcept -> QuickJsSandbox& = default;

auto QuickJsSandbox::execute(const std::string& script, const std::string& contextJson) -> std::string
{
#if defined(NEXUSTAL_HAS_QUICKJS)
    state_->execution_started_at = std::chrono::steady_clock::now();
    setGlobalJsonValue(state_->context, "context", contextJson);
    setGlobalJsonValue(state_->context, "__nt_context", contextJson);

    for (const auto& [name, value] : state_->injected_variables)
    {
        setGlobalJsonValue(state_->context, name, value);
    }

    JSValue result = JS_Eval(
        state_->context,
        script.c_str(),
        script.size(),
        "<nexustal-script>",
        JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result))
    {
        JS_FreeValue(state_->context, result);
        throw std::runtime_error("QuickJS execution failed: " + exceptionMessage(state_->context));
    }

    const auto serialized = jsonStringify(state_->context, result);
    JS_FreeValue(state_->context, result);
    return serialized;
#else
    (void)script;
    (void)contextJson;
    throw std::runtime_error("QuickJS support is not enabled in this build");
#endif
}

void QuickJsSandbox::registerFunction(const std::string& name, NativeHandler handler)
{
    state_->handlers.insert_or_assign(name, std::move(handler));

#if defined(NEXUSTAL_HAS_QUICKJS)
    const auto wrapperScript = std::string{"globalThis["} + nlohmann::json(name).dump() +
                               "] = function(payload) { "
                               "const serialized = payload === undefined ? 'null' : JSON.stringify(payload); "
                               "const raw = __nexustal_call(" +
                               nlohmann::json(name).dump() +
                               ", serialized); "
                               "try { return JSON.parse(raw); } catch (error) { return raw; } "
                               "};";

    JSValue result = JS_Eval(
        state_->context,
        wrapperScript.c_str(),
        wrapperScript.size(),
        "<nexustal-native-wrapper>",
        JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result))
    {
        JS_FreeValue(state_->context, result);
        throw std::runtime_error("failed to register native wrapper: " + exceptionMessage(state_->context));
    }
    JS_FreeValue(state_->context, result);
#endif
}

void QuickJsSandbox::injectVariable(const std::string& name, const std::string& jsonValue)
{
    state_->injected_variables.insert_or_assign(name, jsonValue);

#if defined(NEXUSTAL_HAS_QUICKJS)
    setGlobalJsonValue(state_->context, name, jsonValue);
#endif
}

void QuickJsSandbox::setExecutionTimeout(int executionTimeoutMs)
{
    state_->execution_timeout_ms = executionTimeoutMs;
}

auto QuickJsSandbox::isAvailable() const -> bool
{
#if defined(NEXUSTAL_HAS_QUICKJS)
    return state_ != nullptr && state_->runtime != nullptr && state_->context != nullptr;
#else
    return false;
#endif
}
} // namespace nexustal::infrastructure::sandbox