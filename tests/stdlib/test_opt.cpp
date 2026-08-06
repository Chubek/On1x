#include "on1x/on1x.h"
#include <cstdio>
#include <cstring>

#define CHECK(expr) do { if (!(expr)) { ++failures; std::fprintf(stderr, "CHECK failed [%s:%d]: %s\n", __FILE__, __LINE__, #expr); } } while(0)

int main() {
    int failures = 0;
    On1x_State* s = on1x_open();

    CHECK(on1x_open_std(s) == ON1X_OK);

    // Opt.Map(?42, fn(x) { x * 2 }) -> ?84
    {
        const char* src = "Opt.Map(?42, fn(x) { x * 2 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.Map(?, fn(x) { x * 2 }) -> ?
    {
        const char* src = "Opt.Map(?, fn(x) { x * 2 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.AndThen(?42, fn(x) { ?(x * 2) }) -> ?84
    {
        const char* src = "Opt.AndThen(?42, fn(x) { ?(x * 2) })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.AndThen(?, fn(x) { ?x }) -> ?
    {
        const char* src = "Opt.AndThen(?, fn(x) { ?x })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.OrElse(?42, fn() { ?99 }) -> ?42
    {
        const char* src = "Opt.OrElse(?42, fn() { ?99 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.OrElse(?, fn() { ?99 }) -> ?99
    {
        const char* src = "Opt.OrElse(?, fn() { ?99 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.Filter(?42, fn(x) { x > 10 }) -> ?42
    {
        const char* src = "Opt.Filter(?42, fn(x) { x > 10 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.Filter(?5, fn(x) { x > 10 }) -> ?
    {
        const char* src = "Opt.Filter(?5, fn(x) { x > 10 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.Filter(?, fn(x) { x > 10 }) -> ?
    {
        const char* src = "Opt.Filter(?, fn(x) { x > 10 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.ToList(?42) -> [42]
    {
        const char* src = "Opt.ToList(?42)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.ToList(?) -> []
    {
        const char* src = "Opt.ToList(?)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.FromList([42, 99]) -> ?42
    {
        const char* src = "Opt.FromList([42, 99])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.FromList([]) -> ?
    {
        const char* src = "Opt.FromList([])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.Zip(?1, ?"a") -> ?[1, "a"]
    {
        const char* src = "Opt.Zip(?1, ?\"a\")";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.Zip(?, ?1) -> ?
    {
        const char* src = "Opt.Zip(?, ?1)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.Zip(?1, ?) -> ?
    {
        const char* src = "Opt.Zip(?1, ?)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.Flatten(?(?42)) -> ?42
    {
        const char* src = "Opt.Flatten(?(?42))";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.Flatten(?(?)) -> ?   (Some[None] -> None)
    {
        const char* src = "Opt.Flatten(?(?))";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Opt.Flatten(?) -> ?
    {
        const char* src = "Opt.Flatten(?)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    on1x_close(s);

    std::fprintf(stderr, "Opt tests: %d failures\n", failures);
    return failures;
}
