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

    // ---- Math domain errors return :None, not nil ----
    {
        const char* src = "Math.Sqrt(-1)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Str.Find returns :None when not found ----
    {
        const char* src = "Str.Find(\"hello\", \"z\")";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Str.Find returns :Some when found ----
    {
        const char* src = "Str.Find(\"hello\", \"ll\")";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- List.First on empty returns :None ----
    {
        const char* src = "List.First([])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- List.Last on empty returns :None ----
    {
        const char* src = "List.Last([])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- List.Sort on empty returns empty list, not nil ----
    {
        const char* src = "List.Sort([])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        // Verify it returns a list
        int ty = on1x_type(s, -1);
        CHECK(ty == ON1X_LIST);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Iter.Next on exhausted iterator returns :None ----
    {
        const char* src = "Iter.Next(Iter.FromList([]))";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Rand.Choice on empty list returns :None ----
    {
        const char* src = "Rand.Choice(Rand.New(42), [])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Opt.FromList on empty returns :None ----
    {
        const char* src = "Opt.FromList([])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    on1x_close(s);
    return failures;
}
