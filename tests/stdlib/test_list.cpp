#include <on1x/on1x.h>
#include <cstdio>
#include <cstring>

namespace {
int failures = 0;
#define CHECK(expr) do { if (!(expr)) { ++failures; std::fprintf(stderr, "CHECK failed [%s:%d]: %s\n", __FILE__, __LINE__, #expr); } } while (false)

On1x_State* setup() {
    On1x_State* s = on1x_open();
    CHECK(s != nullptr);
    CHECK(on1x_open_std(s) == ON1X_OK);
    return s;
}

// Helper: eval a chunk and check status
bool eval_ok(On1x_State* s, const char* src) {
    return on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK;
}

// Helper: eval and expect True
bool eval_true(On1x_State* s, const char* src) {
    if (!eval_ok(s, src)) return false;
    bool result = on1x_as_bool(s, -1);
    on1x_pop(s, 1);
    return result;
}

// Helper: eval and expect False
bool eval_false(On1x_State* s, const char* src) {
    if (!eval_ok(s, src)) return false;
    bool result = !on1x_as_bool(s, -1);
    on1x_pop(s, 1);
    return result;
}

// Helper: eval and expect :None
bool eval_none(On1x_State* s, const char* src) {
    if (!eval_ok(s, src)) return false;
    bool result = on1x_is_none(s, -1);
    on1x_pop(s, 1);
    return result;
}

// Helper: eval and expect :Some
bool eval_some(On1x_State* s, const char* src) {
    if (!eval_ok(s, src)) return false;
    bool result = on1x_is_some(s, -1);
    on1x_pop(s, 1);
    return result;
}
}  // namespace

int main() {
    // ---- Construction ----
    {
        On1x_State* s = setup();
        // List.New(3, 0) creates [0, 0, 0]
        CHECK(eval_ok(s, "List.New(3, 0)"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_payload_of(s, -1) == 1);
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 2) == 1);

        // List.New(-1, 0) -> :None
        CHECK(eval_none(s, "List.New(-1, 0)"));

        // List.New(0, 0) -> :Some[[]]
        CHECK(eval_ok(s, "List.New(0, 0)"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_payload_of(s, -1) == 1);
        CHECK(on1x_len(s, -1) == 0);
        CHECK(on1x_pop(s, 2) == 1);

        // List.Copy
        CHECK(eval_ok(s, "let a = [1, 2, 3]; List.Copy(a)"));
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);

        // List.Concat
        CHECK(eval_ok(s, "List.Concat([1, 2], [3, 4])"));
        CHECK(on1x_len(s, -1) == 4);
        CHECK(on1x_pop(s, 1) == 1);

        // List.Append
        CHECK(eval_ok(s, "List.Append([1], [2, 3])"));
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);

        // List.Slice
        CHECK(eval_ok(s, "List.Slice([10, 20, 30, 40], 1, 3)"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_payload_of(s, -1) == 1);
        CHECK(on1x_len(s, -1) == 2);
        CHECK(on1x_pop(s, 2) == 1);

        // Slice out of bounds -> :None
        CHECK(eval_none(s, "List.Slice([10, 20], 5, 10)"));

        on1x_close(s);
    }

    // ---- Access ----
    {
        On1x_State* s = setup();

        CHECK(eval_ok(s, "List.First([10, 20, 30])"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_payload_of(s, -1) == 1);
        CHECK(on1x_list_get(s, -1, 0) == 1);
        CHECK(on1x_as_int(s, -1) == 10);
        CHECK(on1x_pop(s, 3) == 1);

        CHECK(eval_none(s, "List.First([])"));
        CHECK(eval_ok(s, "List.Last([10, 20, 30])"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
        CHECK(eval_none(s, "List.Last([])"));

        // Take / Drop
        CHECK(eval_ok(s, "List.Take([1, 2, 3, 4, 5], 2)"));
        CHECK(on1x_len(s, -1) == 2);
        CHECK(on1x_pop(s, 1) == 1);

        CHECK(eval_ok(s, "List.Drop([1, 2, 3, 4, 5], 2)"));
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);

        // Insert returns :Bool
        CHECK(eval_true(s, "List.Insert([1, 2, 3], 1, 99)"));
        // Insert at out of range -> :Bool false
        CHECK(eval_false(s, "List.Insert([1, 2, 3], 10, 99)"));

        // Remove returns :Some | :None
        CHECK(eval_ok(s, "List.Remove([1, 2, 3], 0)"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
        CHECK(eval_none(s, "List.Remove([1, 2, 3], 10)"));

        // Reverse
        CHECK(eval_ok(s, "List.Reverse([1, 2, 3])"));
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);

        // ReverseInPlace returns :Unit
        CHECK(eval_ok(s, "List.ReverseInPlace([1, 2, 3])"));
        CHECK(on1x_is_unit(s, -1));
        CHECK(on1x_pop(s, 1) == 1);

        // Fill
        CHECK(eval_ok(s, "List.Fill([1, 2, 3], 42)"));
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);

        on1x_close(s);
    }

    // ---- Higher-Order ----
    {
        On1x_State* s = setup();

        // Map
        CHECK(eval_ok(s, "let f = fn(x) { x + 1 }; List.Map([1, 2, 3], f)"));
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);

        // Filter
        CHECK(eval_ok(s, "let even = fn(x) { x % 2 == 0 }; List.Filter([1, 2, 3, 4], even)"));
        CHECK(on1x_len(s, -1) == 2);
        CHECK(on1x_pop(s, 1) == 1);

        // Reduce
        CHECK(eval_ok(s, "let add = fn(acc, x) { acc + x }; List.Reduce([1, 2, 3], add, 0)"));
        CHECK(on1x_as_int(s, -1) == 6);
        CHECK(on1x_pop(s, 1) == 1);

        // ForEach
        CHECK(eval_ok(s, "let counter = [0]; let inc = fn(x) { counter[0] = counter[0] + x; }; List.ForEach([1, 2, 3], inc)"));
        CHECK(on1x_is_unit(s, -1));
        CHECK(on1x_pop(s, 1) == 1);

        // Any / All
        CHECK(eval_true(s, "let pos = fn(x) { x > 0 }; List.Any([-1, 0, 4], pos)"));
        CHECK(eval_false(s, "let pos = fn(x) { x > 0 }; List.Any([-1, -2, 0], pos)"));
        CHECK(eval_true(s, "let pos = fn(x) { x > 0 }; List.All([1, 2, 3], pos)"));
        CHECK(eval_false(s, "let pos = fn(x) { x > 0 }; List.All([1, -2, 3], pos)"));

        // Find
        CHECK(eval_ok(s, "let eq3 = fn(x) { x == 3 }; List.Find([1, 2, 3, 4], eq3)"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
        CHECK(eval_none(s, "let eq9 = fn(x) { x == 9 }; List.Find([1, 2, 3], eq9)"));

        // FindIndex
        CHECK(eval_ok(s, "let eq3 = fn(x) { x == 3 }; List.FindIndex([1, 2, 3, 4], eq3)"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
        CHECK(eval_none(s, "let eq9 = fn(x) { x == 9 }; List.FindIndex([1, 2, 3], eq9)"));

        // Partition
        CHECK(eval_ok(s, "let pos = fn(x) { x > 0 }; List.Partition([1, -2, 3, -4], pos)"));
        CHECK(on1x_len(s, -1) == 2);
        CHECK(on1x_pop(s, 1) == 1);

        // IndexOf
        CHECK(eval_ok(s, "List.IndexOf([10, 20, 30], 20)"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
        CHECK(eval_none(s, "List.IndexOf([10, 20, 30], 99)"));

        // Count
        CHECK(eval_ok(s, "let pos = fn(x) { x > 0 }; List.Count([1, -2, 3, 0], pos)"));
        CHECK(on1x_as_int(s, -1) == 2);
        CHECK(on1x_pop(s, 1) == 1);

        on1x_close(s);
    }

    // ---- Sort ----
    {
        On1x_State* s = setup();

        // Sort
        CHECK(eval_ok(s, "List.Sort([3, 1, 2])"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
        // Empty list sort
        CHECK(eval_ok(s, "List.Sort([])"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
        // Incomparable elements -> :None (List and Table don't compare)
        CHECK(eval_none(s, "List.Sort([[1], [2]])"));

        // SortBy
        CHECK(eval_ok(s, "let neg = fn(a, b) { -(a - b) }; List.SortBy([3, 1, 2], neg)"));
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);

        // IsSorted
        CHECK(eval_true(s, "List.IsSorted([1, 2, 3])"));
        CHECK(eval_false(s, "List.IsSorted([3, 1, 2])"));
        CHECK(eval_true(s, "List.IsSorted([])"));

        // Unique
        CHECK(eval_ok(s, "List.Unique([1, 2, 2, 3, 1])"));
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);

        on1x_close(s);
    }

    // ---- Zip ----
    {
        On1x_State* s = setup();

        // Zip
        CHECK(eval_ok(s, "List.Zip([1, 2], ['a', 'b'])"));
        CHECK(on1x_len(s, -1) == 2);
        CHECK(on1x_pop(s, 1) == 1);

        // Unzip
        CHECK(eval_ok(s, "List.Unzip([[1, 'a'], [2, 'b']])"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
        // Unzip malformed -> :None
        CHECK(eval_none(s, "List.Unzip([[1]])"));

        // Enumerate
        CHECK(eval_ok(s, "List.Enumerate(['a', 'b', 'c'])"));
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);

        // Flatten
        CHECK(eval_ok(s, "List.Flatten([[1, 2], [3], [4, 5]])"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
        // Flatten non-list element -> :None
        CHECK(eval_none(s, "List.Flatten([[1], 2])"));

        // Chunk
        CHECK(eval_ok(s, "List.Chunk([1, 2, 3, 4, 5], 2)"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
        CHECK(eval_none(s, "List.Chunk([1, 2], 0)"));

        // Window
        CHECK(eval_ok(s, "List.Window([1, 2, 3, 4], 2)"));
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
        CHECK(eval_none(s, "List.Window([1], 5)"));
        CHECK(eval_none(s, "List.Window([1, 2], 0)"));

        on1x_close(s);
    }

    // ---- Edge Cases ----
    {
        On1x_State* s = setup();

        // Empty list
        CHECK(eval_ok(s, "List.First([])"));
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);

        CHECK(eval_ok(s, "List.Map([], fn(x) { x + 1 })"));
        CHECK(on1x_len(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);

        CHECK(eval_ok(s, "List.Filter([], fn(x) { x > 0 })"));
        CHECK(on1x_len(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);

        // Take/Drop saturating
        CHECK(eval_ok(s, "List.Take([1, 2], 10)"));
        CHECK(on1x_len(s, -1) == 2);
        CHECK(on1x_pop(s, 1) == 1);

        CHECK(eval_ok(s, "List.Drop([1, 2], 10)"));
        CHECK(on1x_len(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);

        // Negative Take/Drop
        CHECK(eval_ok(s, "List.Take([1, 2], -5)"));
        CHECK(on1x_len(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);

        on1x_close(s);
    }

    std::fprintf(stderr, "List tests: %d failures\n", failures);
    return failures;
}
