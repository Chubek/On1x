#include "core/value.hpp"
#include "ffi/ffi.hpp"
#include "gc/gc.hpp"

#include <cstdio>

namespace {

int failures = 0;
#define CHECK(expression) do { if (!(expression)) { ++failures; std::fprintf(stderr, "CHECK failed: %s\n", #expression); } } while (false)

extern "C" long long add_long_longs(long long left, long long right) {
    return left + right;
}

extern "C" double double_value(double value) {
    return value * 2.0;
}

}  // namespace

int main() {
    on1x::GcState gc;
    on1x::gc_init(&gc);
    on1x::ffi::FfiContext ffi;
    CHECK(ffi.available());

    const on1x::Value integer_args[] = {
        on1x::Value::integer(&gc, 18),
        on1x::Value::integer(&gc, 24),
    };
    on1x::Value result;
    CHECK(ffi.call(&gc, reinterpret_cast<void*>(add_long_longs), "ll)l", integer_args, 2, result));
    CHECK(result.is_int() && result.as_int() == 42);

    const on1x::Value float_args[] = {on1x::Value::floating(2.5)};
    CHECK(ffi.call(&gc, reinterpret_cast<void*>(double_value), "d)d", float_args, 1, result));
    CHECK(result.is_float() && result.as_float() == 5.0);
    CHECK(!ffi.call(&gc, reinterpret_cast<void*>(double_value), "x)d", float_args, 1, result));
    CHECK(!ffi.call(&gc, reinterpret_cast<void*>(double_value), "d)d", integer_args, 2, result));

    on1x::gc_shutdown(&gc);
    return failures == 0 ? 0 : 1;
}
