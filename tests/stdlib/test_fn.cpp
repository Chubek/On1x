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

    // ---- Fn.Identity returns its argument ----
    {
        const char* src = "Fn.Identity(42)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 42);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Fn.Const creates a constant function ----
    {
        const char* src = "let f = Fn.Const(99)\nf()";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 99);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Fn.Apply applies a function to argument list ----
    {
        const char* src = "let f = fn(x, y) { x + y }\nFn.Apply(f, [3, 4])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 7);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Fn.Compose: f(g(x)) ----
    {
        const char* src = "let f = fn(x) { x * 2 }\nlet g = fn(x) { x + 1 }\nlet h = Fn.Compose(f, g)\nh(5)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 12);  // f(g(5)) = f(6) = 12
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Fn.Pipe takes a list of functions: g(f(x)) ----
    {
        const char* src = "let f = fn(x) { x * 2 }\nlet g = fn(x) { x + 1 }\nlet h = Fn.Pipe([f, g])\nh(5)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 11);  // g(f(5)) = g(10) = 11
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Fn.Partial with bound args list ----
    {
        const char* src = "let f = fn(x, y) { x - y }\nlet g = Fn.Partial(f, [10])\ng(3)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 7);  // f(10, 3) = 7
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Fn.Flip swaps first two arguments ----
    {
        const char* src = "let f = fn(x, y) { x - y }\nlet g = Fn.Flip(f)\ng(3, 10)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 7);  // f(10, 3) = 7
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Fn.Memo memoizes and returns correct result ----
    {
        const char* src = "let f = fn(x) { x * x }\nlet g = Fn.Memo(f)\ng(4)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 16);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Fn.Memo returns cached result on repeated call ----
    {
        const char* src = "let f = fn(x) { x * x }\nlet g = Fn.Memo(f)\ng(4)\ng(4)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 16);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Fn.Arity returns the arity of a function ----
    {
        const char* src = "let f = fn(x, y) { x + y }\nFn.Arity(f)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 2);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Fn.IsVariadic returns a Bool ----
    {
        const char* src = "Fn.IsVariadic(Fn.Identity)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_BOOL);
        CHECK(on1x_pop(s, 1) == 1);
    }

    on1x_close(s);
    return failures;
}
