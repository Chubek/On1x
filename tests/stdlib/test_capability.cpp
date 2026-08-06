#include <on1x/on1x.h>
#include <on1x/on1x_config.h>
#include <cstdio>
#include <cstring>

namespace {
int failures = 0;
#define CHECK(expr) do { if (!(expr)) { ++failures; std::fprintf(stderr, "CHECK failed [%s:%d]: %s\n", __FILE__, __LINE__, #expr); } } while (false)
}

int main() {
    On1x_State* s = on1x_open();
    CHECK(s != nullptr);

    // Pure modules install without any capability grants
    CHECK(on1x_open_std(s) == ON1X_OK);

    // Verify pure modules are accessible
    {
        const char* src = "Cmp.Eq(1, 1)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 1);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Impure modules (Io, Os, Time, Fs) are NOT installed by on1x_open_std
    // because they require capabilities.
    // Trying to access them should raise an error.
    {
        const char* src = "Io.Print(\"test\")";
        On1x_Status st = on1x_eval(s, src, std::strlen(src), "test");
        CHECK(st == ON1X_ERR);  // Io is not installed
        on1x_pop(s, on1x_top(s));
    }

    // Os module should not be present
    {
        const char* src = "Os.GetEnv(\"PATH\")";
        On1x_Status st = on1x_eval(s, src, std::strlen(src), "test");
        CHECK(st == ON1X_ERR);
        on1x_pop(s, on1x_top(s));
    }

    // Time module should not be present
    {
        const char* src = "Time.Now()";
        On1x_Status st = on1x_eval(s, src, std::strlen(src), "test");
        CHECK(st == ON1X_ERR);
        on1x_pop(s, on1x_top(s));
    }

    // Fs module should not be present
    {
        const char* src = "Fs.ReadFile(\"/tmp/test\")";
        On1x_Status st = on1x_eval(s, src, std::strlen(src), "test");
        CHECK(st == ON1X_ERR);
        on1x_pop(s, on1x_top(s));
    }

    // Explicit capability grants should make modules installable
#if !ON1X_STDLIB_PURE_ONLY
    {
        On1x_Status st = on1x_grant(s, ON1X_CAP_IO);
        CHECK(st == ON1X_OK);
        st = on1x_open_io(s);
        CHECK(st == ON1X_OK);
        // Now Io should be accessible
        const char* src = "Io.Print(\"hello\")";
        st = on1x_eval(s, src, std::strlen(src), "test");
        CHECK(st == ON1X_OK);  // Io.Print succeeds
        CHECK(on1x_pop(s, 1) == 1);
    }
#endif // !ON1X_STDLIB_PURE_ONLY

    on1x_close(s);
    return failures;
}
