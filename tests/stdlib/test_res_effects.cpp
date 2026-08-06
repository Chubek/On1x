#include <on1x/on1x.h>
#include <on1x/on1x.h>
#include <cstdio>
#include <cstring>

namespace {
int failures = 0;
#define CHECK(expr) do { if (!(expr)) { ++failures; std::fprintf(stderr, "CHECK failed [%s:%d]: %s\n", __FILE__, __LINE__, #expr); } } while (false)
}

int main() {
    On1x_State* s = on1x_open();
    CHECK(s != nullptr);
    CHECK(on1x_open_std(s) == ON1X_OK);

    // ---- Res.Map transforms the success payload ----
    {
        const char* src = "~ 42\nRes.Map(~, fn(x) { x * 2 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_success(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Res.Map passes through :Error unchanged ----
    {
        const char* src = "~ 1 / 0\nRes.Map(~, fn(x) { x * 2 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_error(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Res.MapError transforms the error ----
    {
        const char* src = "~ 1 / 0\nRes.MapError(~, fn(e) { e })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_error(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Res.AndThen chains on success ----
    {
        const char* src = "~ 10\nRes.AndThen(~, fn(x) { ~ (x + 1)\n~ })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_success(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Res.AndThen short-circuits on error ----
    {
        const char* src = "~ 1 / 0\nRes.AndThen(~, fn(x) { ~ (x + 1)\n~ })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_error(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Res.OrElse returns success unchanged ----
    {
        const char* src = "~ 42\nRes.OrElse(~, fn(e) { ~ 0\n~ })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_success(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Res.OrElse calls fallback on error ----
    {
        const char* src = "~ 1 / 0\nRes.OrElse(~, fn(e) { ~ 99\n~ })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_success(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Res.Payload extracts value from either arm ----
    {
        const char* src = "~ 42\nRes.Payload(~)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "~ 1 / 0\nRes.Payload(~)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Res.ToOpt converts Success to Some ----
    {
        const char* src = "~ 42\nRes.ToOpt(~)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Res.ToOpt converts Error to None ----
    {
        const char* src = "~ 1 / 0\nRes.ToOpt(~)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Res.UnwrapOr extracts success payload ----
    {
        const char* src = "~ 42\nRes.UnwrapOr(~, 0)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Res.UnwrapOr returns default on error ----
    {
        const char* src = "~ 1 / 0\nRes.UnwrapOr(~, 0)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Res.Collect on all-success list ----
    {
        const char* src = "~ 1\nlet a = ~;\n~ 2\nlet b = ~;\n~ 3\nlet c = ~;\nRes.Collect([a, b, c])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_success(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Res.Collect stops at first error ----
    {
        const char* src = "~ 1\nlet a = ~;\n~ 1 / 0\nlet b = ~;\n~ 3\nlet c = ~;\nRes.Collect([a, b, c])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_error(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    on1x_close(s);
    return failures;
}
