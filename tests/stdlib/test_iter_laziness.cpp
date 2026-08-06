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

    // ---- Iter.FromList creates iterator ----
    {
        const char* src = "Iter.FromList([1, 2, 3])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);  // Tagged list
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Iter.Next on empty returns :None immediately ----
    {
        const char* src = "Iter.Next(Iter.FromList([]))";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Iter.FromIota creates iterator ----
    {
        const char* src = "Iter.FromIota(Iota(0, 5))";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Iter.Next from FromList returns :Some for first element ----
    {
        const char* src = "Iter.Next(Iter.FromList([10, 20, 30]))";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Iter.Take returns an iterator ----
    {
        const char* src = "Iter.Take(Iter.FromList([1,2,3,4,5]), 2)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);  // Tagged list iterator
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Iter.Count counts elements ----
    {
        const char* src = "Iter.Count(Iter.FromList([1, 2, 3]))";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Iter.Collect collects to list ----
    {
        const char* src = "Iter.Collect(Iter.FromList([1, 2, 3]))";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_pop(s, 1) == 1);
    }

    on1x_close(s);
    return failures;
}
