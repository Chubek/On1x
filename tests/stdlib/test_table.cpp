#include <on1x/on1x.h>
#include <cstdio>
#include <cstring>

namespace {
int failures = 0;
#define CHECK(expr) do { if (!(expr)) { ++failures; std::fprintf(stderr, "CHECK failed [%s:%d]: %s\n", __FILE__, __LINE__, #expr); } } while (false)
}  // namespace

int main() {
    On1x_State* s = on1x_open();
    CHECK(s != nullptr);
    CHECK(on1x_open_std(s) == ON1X_OK);

    // ---- Construction ----

    // Table.New
    {
        const char* src = "Table.New()";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_TABLE);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Table.Copy
    {
        const char* src = "Table.Copy(%{ :a => 1, :b => 2 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_TABLE);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Table.Merge
    {
        const char* src = "Table.Merge(%{ :a => 1 }, %{ :b => 2 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_TABLE);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Table.Merge — b wins
    {
        const char* src = "Table.Merge(%{ :a => 1 }, %{ :a => 99 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_TABLE);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Query ----

    // Table.HasKey
    {
        const char* src = "Table.HasKey(%{ :x => 1 }, :x)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 1);
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "Table.HasKey(%{ :x => 1 }, :y)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Table.GetOr
    {
        const char* src = "Table.GetOr(%{ :x => 42 }, :x, 0)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 42);
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "Table.GetOr(%{ :x => 42 }, :y, 0)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Table.Remove
    {
        const char* src = "Table.Remove(%{ :x => 1 }, :x)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "Table.Remove(%{ :x => 1 }, :y)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Entries / FromEntries ----

    // Table.Entries
    {
        const char* src = "Table.Entries(%{ :a => 1 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 1);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Table.FromEntries
    {
        const char* src = "Table.FromEntries([[:a, 1], [:b, 2]])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Table.FromEntries with unhashable key -> :None
    {
        const char* src = "Table.FromEntries([[[1], 2]])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Table.FromEntries with malformed pair -> :None
    {
        const char* src = "Table.FromEntries([[:a]])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Invert ----

    // Table.Invert
    {
        const char* src = "Table.Invert(%{ :a => 1, :b => 2 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Table.Invert with unhashable value -> :None
    {
        const char* src = "Table.Invert(%{ :a => [1] })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Pick / Omit ----

    // Table.Pick
    {
        const char* src = "Table.Pick(%{ :a => 1, :b => 2, :c => 3 }, [:a, :c])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_TABLE);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Table.Omit
    {
        const char* src = "Table.Omit(%{ :a => 1, :b => 2, :c => 3 }, [:a, :c])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_TABLE);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- IsHashable ----

    // Table.IsHashable
    {
        const char* src = "Table.IsHashable(42)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 1);
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "Table.IsHashable(\"hello\")";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 1);
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "Table.IsHashable(:tag)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 1);
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "Table.IsHashable([1, 2])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "Table.IsHashable(%{ :x => 1 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Higher-Order ----

    // Table.MapValues
    {
        const char* src = "let dbl = fn(x) { x * 2 }; Table.MapValues(%{ :a => 1, :b => 2 }, dbl)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_TABLE);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Table.FilterKeys
    {
        const char* src = "let pred = fn(k) { k == :a }; Table.FilterKeys(%{ :a => 1, :b => 2 }, pred)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_TABLE);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Table.ReduceEntries
    {
        const char* src = "let sumVals = fn(acc, k, v) { acc + v }; Table.ReduceEntries(%{ :a => 1, :b => 2, :c => 3 }, sumVals, 0)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 6);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Table.ForEachEntry — empty table returns :Unit
    {
        const char* src = "Table.ForEachEntry(%{}, fn(k, v) { k })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_UNIT);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Table.ForEachEntry — non-empty table returns :Unit
    {
        const char* src = "Table.ForEachEntry(%{ :a => 1 }, fn(k, v) { 0 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_UNIT);
        CHECK(on1x_pop(s, 1) == 1);
    }

    on1x_close(s);

    std::fprintf(stderr, "Table tests: %d failures\n", failures);
    return failures;
}
