#include "api/api_common.hpp"
#include "core/string.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "stdlib/time/time.hpp"
#include "stdlib/time/time_impl.hpp"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <string>
#include <thread>

namespace on1x::stdlib {
namespace {

On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

// Now() -> :Int (Unix seconds)
On1x_Status time_now(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 0, "Time.Now")) return ON1X_ERR;
    auto now = std::chrono::system_clock::now();
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    return stack_push(s, Value::integer(&s->gc, static_cast<std::int64_t>(secs))) ? ON1X_OK : ON1X_ERR;
}

// NowMillis() -> :Int (Unix milliseconds)
On1x_Status time_now_millis(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 0, "Time.NowMillis")) return ON1X_ERR;
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    return stack_push(s, Value::integer(&s->gc, static_cast<std::int64_t>(ms))) ? ON1X_OK : ON1X_ERR;
}

// NowNanos() -> :Int (Unix nanoseconds)
On1x_Status time_now_nanos(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 0, "Time.NowNanos")) return ON1X_ERR;
    auto now = std::chrono::system_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    return stack_push(s, Value::integer(&s->gc, static_cast<std::int64_t>(ns))) ? ON1X_OK : ON1X_ERR;
}

// Monotonic() -> :Int (nanoseconds, unspecified epoch)
On1x_Status time_monotonic(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 0, "Time.Monotonic")) return ON1X_ERR;
    auto now = std::chrono::steady_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    return stack_push(s, Value::integer(&s->gc, static_cast<std::int64_t>(ns))) ? ON1X_OK : ON1X_ERR;
}

// Sleep(ms: :Int) -> :Unit
On1x_Status time_sleep(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Time.Sleep")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || !v.is_int())
        return bad(s, "Time.Sleep expects an Int (milliseconds)");
    std::int64_t ms = v.as_int();
    if (ms < 0) return bad(s, "Time.Sleep: negative duration");
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

const On1x_FnDesc fns[] = {
    {"Now",       time_now},
    {"NowMillis", time_now_millis},
    {"NowNanos",  time_now_nanos},
    {"Monotonic", time_monotonic},
    {"Sleep",     time_sleep},
    {"Breakdown", time_detail::time_breakdown},
    {"Format",    time_detail::time_format},
    {"Parse",     time_detail::time_parse},
};

const On1x_ModuleDesc desc{"Time", ON1X_CAP_TIME, fns, sizeof(fns) / sizeof(*fns)};

}  // namespace

const On1x_ModuleDesc* time_module() noexcept { return &desc; }

}  // namespace on1x::stdlib
