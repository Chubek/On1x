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
    CHECK(on1x_open_std(s) == ON1X_OK);

    // ---- Rand.New creates a generator from a seed ----
    {
        const char* src = "Rand.New(42)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Rand.Int returns [new_rng, int_val] ----
    {
        const char* src = "let g = Rand.New(42)\nRand.Int(g)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Rand.Float returns [new_rng, float_val] in [0,1) ----
    {
        const char* src = "let g = Rand.New(99)\nRand.Float(g)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Rand.Bool returns [new_rng, bool_val] ----
    {
        const char* src = "let g = Rand.New(1)\nRand.Bool(g)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Rand.IntBelow(g, 10) returns :Some[[new_rng, int_val]] ----
    {
        const char* src = "let g = Rand.New(7)\nRand.IntBelow(g, 10)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Rand.IntBelow(g, 0) returns :None ----
    {
        const char* src = "let g = Rand.New(7)\nRand.IntBelow(g, 0)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Rand.Range(g, 5, 10) returns :Some[[new_rng, int_val]] ----
    {
        const char* src = "let g = Rand.New(3)\nRand.Range(g, 5, 10)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Rand.Range(g, 10, 5) returns :None (lo >= hi) ----
    {
        const char* src = "let g = Rand.New(3)\nRand.Range(g, 10, 5)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Rand.Choice(g, [1, 2, 3]) returns :Some[[new_rng, item]] ----
    {
        const char* src = "let g = Rand.New(5)\nRand.Choice(g, [1, 2, 3])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Rand.Choice(g, []) returns :None ----
    {
        const char* src = "let g = Rand.New(5)\nRand.Choice(g, [])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Rand.Shuffle(g, [1, 2, 3]) returns [new_rng, shuffled_list] ----
    {
        const char* src = "let g = Rand.New(11)\nRand.Shuffle(g, [1, 2, 3])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Rand.SeedFromSystem returns :Some[seed] or :None ----
#if !ON1X_STDLIB_PURE_ONLY
    {
        const char* src = "Rand.SeedFromSystem()";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_pop(s, 1) == 1);
    }
#endif

    on1x_close(s);
    return failures;
}
