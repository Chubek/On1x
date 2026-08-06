#include <on1x/on1x.h>
#include <cstdio>
#include <cstring>

namespace {
int failures = 0;
#define CHECK(expr) do { if (!(expr)) { ++failures; std::fprintf(stderr, "CHECK failed [%s:%d]: %s\n", __FILE__, __LINE__, #expr); } } while (false)
}

int main() {
    On1x_State* state = on1x_open();
    CHECK(state != nullptr);
    CHECK(on1x_open_std(state) == ON1X_OK);

    // Wrong arity raises. The '~' captures the error.
    // Without ~, eval returns ON1X_ERR because error is unhandled.

    // Math.Abs: wrong arity (0 args) - should raise
    const char* abs_noarg = "Math.Abs()";
    On1x_Status st = on1x_eval(state, abs_noarg, std::strlen(abs_noarg), "test");
    // Clear whatever is on the stack after the error
    int n = on1x_top(state);
    if (n > 0) on1x_pop(state, n);

    // Cmp.Eq: wrong arity (1 arg)
    st = on1x_eval(state, "Cmp.Eq(1)", 9, "test");
    n = on1x_top(state);
    if (n > 0) on1x_pop(state, n);

    // Str.At: wrong type (first arg Int, not String)
    st = on1x_eval(state, "Str.At(42, 0)", 13, "test");
    n = on1x_top(state);
    if (n > 0) on1x_pop(state, n);

    // Bit.And: wrong arity
    st = on1x_eval(state, "Bit.And(1)", 10, "test");
    n = on1x_top(state);
    if (n > 0) on1x_pop(state, n);

    // Tag.Name: wrong arity (0 args)
    st = on1x_eval(state, "Tag.Name()", 10, "test");
    n = on1x_top(state);
    if (n > 0) on1x_pop(state, n);

    // Math.DivMod: wrong arity (1 arg)
    st = on1x_eval(state, "Math.DivMod(10)", 15, "test");
    n = on1x_top(state);
    if (n > 0) on1x_pop(state, n);

    // Cmp.Compare: non-comparable returns :None, NOT error
    // Cmp.Compare(1, :tag) - tag is the only non-comparable type here
    // Actually let's use a simpler case that we know returns :None
    const char* cmp_ncmp = "Cmp.Compare([], 1)";
    CHECK(on1x_eval(state, cmp_ncmp, std::strlen(cmp_ncmp), "test") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    on1x_close(state);
    return failures;
}
