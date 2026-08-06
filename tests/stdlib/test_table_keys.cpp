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

    // ---- Table.FromEntries rejects List keys (returns :None) ----
    {
        const char* src = "Table.FromEntries([[ [1], 42 ]])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        // :None means the key was rejected
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Table.FromEntries rejects Table keys (returns :None) ----
    {
        const char* src = "Table.FromEntries([[ %{ :a => 1 }, 42 ]])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Table.FromEntries accepts Tag keys ----
    {
        const char* src = "Table.FromEntries([[ :valid_tag, 99 ]])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        // Wrapped in :Some[table]
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Table.Invert rejects List values-as-keys (returns :None) ----
    {
        const char* src = "Table.Invert(%{ :a => [1] })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Cmp.Hash rejects List (returns :None) ----
    {
        const char* src = "Cmp.Hash([1, 2, 3])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Cmp.Hash rejects Table (returns :None) ----
    {
        const char* src = "Cmp.Hash(%{ :x => 1 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Cmp.Hash accepts hashable values (returns :Some[hash]) ----
    {
        const char* src = "Cmp.Hash(42)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Fn.Memo works with hashable args ----
    {
        const char* src = "Fn.Memo(fn(x){x+1})";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_FN);
        CHECK(on1x_pop(s, 1) == 1);
    }

    on1x_close(s);
    return failures;
}
