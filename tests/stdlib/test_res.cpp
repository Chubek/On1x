#include "on1x/on1x.h"
#include <cstdio>
#include <cstring>

#include <string>

#define CHECK(expr) do { if (!(expr)) { ++failures; std::fprintf(stderr, "CHECK failed [%s:%d]: %s\n", __FILE__, __LINE__, #expr); } } while(0)

int main() {
    int failures = 0;
    On1x_State* s = on1x_open();

    CHECK(on1x_open_std(s) == ON1X_OK);

    // Res.Map(:Success[42], fn(x) { x * 2 }) -> :Success[84]
    {
        // First, test that basic effect capture works
        CHECK(on1x_eval(s, "~ 42\n~", std::strlen("~ 42\n~"), "test") == ON1X_OK);
        CHECK(on1x_is_success(s, -1));
        CHECK(on1x_pop(s, 1) == 1);

        // Test that Res module is accessible and works with direct values
        CHECK(on1x_eval(s, "Res.Map", std::strlen("Res.Map"), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_FN);
        CHECK(on1x_pop(s, 1) == 1);

        // Test that IsSuccess works in follower (like API test)
        CHECK(on1x_eval(s, "~ 42\nIsSuccess(~)", std::strlen("~ 42\nIsSuccess(~)"), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 1);
        CHECK(on1x_pop(s, 1) == 1);

        // Test referencing Res.Map in follower
        CHECK(on1x_eval(s, "~ 42\nRes.Map", std::strlen("~ 42\nRes.Map"), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_FN);
        CHECK(on1x_pop(s, 1) == 1);

        // Test Res.Map called with literal values
        CHECK(on1x_eval(s, "~ 42\n~", std::strlen("~ 42\n~"), "test") == ON1X_OK);
        CHECK(on1x_is_success(s, -1));
        CHECK(on1x_pop(s, 1) == 1);

        // Test: effect capture inside function body
        {
            On1x_Status st = on1x_eval(s, "let f = fn() { ~ 42\n~ }\nf()",
                std::strlen("let f = fn() { ~ 42\n~ }\nf()"), "test");
            printf("DIAG: fn() effect capture: st=%d top=%d\n", st, on1x_top(s));
            if (st == ON1X_OK) {
                printf("DIAG:   type=%d is_success=%d\n", on1x_type(s,-1), on1x_is_success(s,-1));
                CHECK(on1x_pop(s, 1) == 1);
            }
        }

        // Test: effect capture inside function with parameter
        {
            On1x_Status st = on1x_eval(s, "let f = fn(x) { ~ (x * 2)\n~ }\nf(42)",
                std::strlen("let f = fn(x) { ~ (x * 2)\n~ }\nf(42)"), "test");
            printf("DIAG: fn(x) effect capture: st=%d top=%d\n", st, on1x_top(s));
            if (st == ON1X_OK) {
                printf("DIAG:   type=%d is_success=%d\n", on1x_type(s,-1), on1x_is_success(s,-1));
                CHECK(on1x_pop(s, 1) == 1);
            }
        }

        // Test: Res.Map with literal args in follower (no ~ as arg)
        {
            On1x_Status st = on1x_eval(s, "~ 42\nRes.Map(42, fn(x) { x * 2 })",
                std::strlen("~ 42\nRes.Map(42, fn(x) { x * 2 })"), "test");
            printf("DIAG: Res.Map(42,...) st=%d top=%d\n", st, on1x_top(s));
            CHECK(st == ON1X_OK);
            if (st == ON1X_OK) CHECK(on1x_pop(s, 1) == 1);
        }

        // Now test with Res.Map in follower
        const char* src = "~ 42\nRes.Map(~, fn(x) { x * 2 })";
        {
            On1x_Status st = on1x_eval(s, src, std::strlen(src), "test");
            printf("DIAG: Res.Map(~,...) st=%d top=%d is_error=%d\n", st, on1x_top(s), on1x_is_error(s, -1));
            if (st != ON1X_OK && on1x_is_error(s, -1)) {
                int rc = on1x_payload_of(s, -1);
                if (rc && on1x_is_some(s, -1)) {
                    rc = on1x_payload_of(s, -1);
                    if (rc && on1x_type(s, -1) == ON1X_STRING) {
                        size_t len; const char* err = on1x_as_string(s, -1, &len);
                        printf("DIAG: ERROR MSG: %.*s\n", (int)len, err);
                    }
                }
            }
            CHECK(st == ON1X_OK);
            if (st == ON1X_OK) {
                CHECK(on1x_is_success(s, -1));
                CHECK(on1x_pop(s, 1) == 1);
            }
        }
    }

    // Res.Map(:Error, fn(x) { x * 2 }) -> :Error (passes through)
    {
        const char* src = "~ 1 / 0\nRes.Map(~, fn(x) { x * 2 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_error(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Res.MapError(:Error["boom"], fn(x) { ... }) -> :Error[...]
    {
        const char* src = "~ 1 / 0\nRes.MapError(~, fn(x) { 99 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_error(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Res.MapError(:Success[42], fn(x) { ... }) -> :Success[42] (passes through)
    {
        const char* src = "~ 42\nRes.MapError(~, fn(x) { 99 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_success(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Res.AndThen(:Success[42], fn(x) { :Success[x * 2] }) -> :Success[84]
    {
        const char* src = "~ 42\nRes.AndThen(~, fn(x) { ~ (x * 2)\n~ })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_success(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Res.AndThen(:Error, fn(x) { ... }) -> :Error (passes through)
    {
        const char* src = "~ 1 / 0\nRes.AndThen(~, fn(x) { ~ 42\n~ })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_error(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Res.OrElse(:Success[42], fn(e) { ... }) -> :Success[42] (passes through)
    {
        const char* src = "~ 42\nRes.OrElse(~, fn(e) { ~ 99\n~ })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_success(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Res.OrElse(:Error, fn(e) { :Success[99] }) -> :Success[99]
    {
        const char* src = "~ 1 / 0\nRes.OrElse(~, fn(e) { ~ 99\n~ })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_success(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Res.Payload(:Success[42]) -> :Some[42]
    {
        const char* src = "~ 42\nRes.Payload(~)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Res.Payload(:Error["boom"]) -> :Some["boom"]
    {
        const char* src = "~ 1 / 0\nRes.Payload(~)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Res.ToOpt(:Success[42]) -> :Some[42]
    {
        const char* src = "~ 42\nRes.ToOpt(~)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Res.ToOpt(:Error) -> :None
    {
        const char* src = "~ 1 / 0\nRes.ToOpt(~)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Res.UnwrapOr(:Success[42], 99) -> 42
    {
        const char* src = "~ 42\nRes.UnwrapOr(~, 99)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Res.UnwrapOr(:Error, 99) -> 99
    {
        const char* src = "~ 1 / 0\nRes.UnwrapOr(~, 99)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 99);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Res.Collect([:Success[1], :Success[2]]) -> :Success[[1, 2]]
    {
        const char* src = "~ 1\nlet a = ~;\n~ 2\nlet b = ~;\nRes.Collect([a, b])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_success(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Res.Collect with error -> :Error
    {
        const char* src = "~ 1\nlet a = ~;\n~ 1 / 0\nlet b = ~;\nRes.Collect([a, b])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_error(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    on1x_close(s);

    std::fprintf(stderr, "Res tests: %d failures\n", failures);
    return failures;
}
