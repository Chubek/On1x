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

    // ---- Construction / slicing / concat ----

    // List.New(3, 0) -> :Some[[0, 0, 0]]
    {
        const char* src = "List.New(3, 0)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_payload_of(s, -1) == 1);       // now [[0,0,0]] at top
        CHECK(on1x_list_get(s, -1, 0) == 1);      // now [0,0,0] at top
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 3) == 1);                // pop list, payload, some
    }

    // List.New(-1, 0) -> :None
    {
        const char* src = "List.New(-1, 0)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.New(0, 0) -> :Some[[]]
    {
        const char* src = "List.New(0, 0)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_payload_of(s, -1) == 1);       // now [[]] at top
        CHECK(on1x_list_get(s, -1, 0) == 1);      // now [] at top
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 0);
        CHECK(on1x_pop(s, 3) == 1);
    }

    // List.Copy
    {
        const char* src = "List.Copy([1, 2, 3])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Concat
    {
        const char* src = "List.Concat([1, 2], [3, 4])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 4);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Append
    {
        const char* src = "List.Append([1], [2, 3])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Slice
    {
        const char* src = "List.Slice([10, 20, 30, 40], 1, 3)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_payload_of(s, -1) == 1);       // now [sliced] at top
        CHECK(on1x_list_get(s, -1, 0) == 1);      // now sliced list at top
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 2);
        CHECK(on1x_pop(s, 3) == 1);
    }

    // Slice out of bounds -> :None
    {
        const char* src = "List.Slice([10, 20], 5, 10)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Access ----

    // List.First -> :Some[value]
    {
        const char* src = "List.First([10, 20, 30])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_payload_of(s, -1) == 1);        // now [10] at top
        CHECK(on1x_list_get(s, -1, 0) == 1);       // now 10 at top
        CHECK(on1x_as_int(s, -1) == 10);
        CHECK(on1x_pop(s, 3) == 1);                // pop int, payload, some
    }

    // List.First on empty -> :None
    {
        const char* src = "List.First([])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Last
    {
        const char* src = "List.Last([10, 20, 30])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Last on empty -> :None
    {
        const char* src = "List.Last([])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Take
    {
        const char* src = "List.Take([1, 2, 3, 4, 5], 2)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 2);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Drop
    {
        const char* src = "List.Drop([1, 2, 3, 4, 5], 2)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Insert returns :Bool
    {
        const char* src = "List.Insert([1, 2, 3], 1, 99)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 1);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Insert out of range -> :Bool false
    {
        const char* src = "List.Insert([1, 2, 3], 10, 99)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Remove returns :Some | :None
    {
        const char* src = "List.Remove([1, 2, 3], 0)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "List.Remove([1, 2, 3], 10)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Reverse
    {
        const char* src = "List.Reverse([1, 2, 3])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.ReverseInPlace returns :Unit
    {
        const char* src = "List.ReverseInPlace([1, 2, 3])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_UNIT);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Fill
    {
        const char* src = "List.Fill([1, 2, 3], 42)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Higher-Order ----

    // List.Map
    {
        const char* src = "let f = fn(x) { x + 1 }; List.Map([1, 2, 3], f)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Filter
    {
        const char* src = "let even = fn(x) { x % 2 == 0 }; List.Filter([1, 2, 3, 4], even)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 2);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Reduce(list, fn, init)  [note: fn comes before init in API]
    {
        const char* src = "let add = fn(acc, x) { acc + x }; List.Reduce([1, 2, 3, 4], add, 0)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 10);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.ForEach returns :Unit
    {
        const char* src = "let nop = fn(x) { x }; List.ForEach([1, 2, 3], nop)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_UNIT);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Any
    {
        const char* src = "let pos = fn(x) { x > 0 }; List.Any([-1, 0, 4], pos)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 1);
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "let pos = fn(x) { x > 0 }; List.Any([-1, -2, 0], pos)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.All
    {
        const char* src = "let pos = fn(x) { x > 0 }; List.All([1, 2, 3], pos)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 1);
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "let pos = fn(x) { x > 0 }; List.All([1, -2, 3], pos)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Find
    {
        const char* src = "let eq3 = fn(x) { x == 3 }; List.Find([1, 2, 3, 4], eq3)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "let eq9 = fn(x) { x == 9 }; List.Find([1, 2, 3], eq9)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.FindIndex
    {
        const char* src = "let eq3 = fn(x) { x == 3 }; List.FindIndex([1, 2, 3, 4], eq3)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "let eq9 = fn(x) { x == 9 }; List.FindIndex([1, 2, 3], eq9)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Partition
    {
        const char* src = "let pos = fn(x) { x > 0 }; List.Partition([1, -2, 3, -4], pos)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 2);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.IndexOf
    {
        const char* src = "List.IndexOf([10, 20, 30], 20)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "List.IndexOf([10, 20, 30], 99)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Count
    {
        const char* src = "let pos = fn(x) { x > 0 }; List.Count([1, -2, 3, 0], pos)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_int(s, -1) == 2);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Sort ----

    // List.Sort
    {
        const char* src = "List.Sort([3, 1, 2])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Sort empty -> :Some[[]]
    {
        const char* src = "List.Sort([])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.SortBy
    {
        const char* src = "let neg = fn(a, b) { -(a - b) }; List.SortBy([3, 1, 2], neg)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.IsSorted
    {
        const char* src = "List.IsSorted([1, 2, 3])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 1);
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "List.IsSorted([3, 1, 2])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);
    }
    {
        const char* src = "List.IsSorted([])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_as_bool(s, -1) == 1);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Unique
    {
        const char* src = "List.Unique([1, 2, 2, 3, 1])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Zip ----

    // List.Zip
    {
        const char* src = "List.Zip([1, 2], [3, 4])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 2);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Unzip
    {
        const char* src = "List.Unzip([[1, 3], [2, 4]])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Unzip malformed -> :None
    {
        const char* src = "List.Unzip([[1]])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Enumerate
    {
        const char* src = "List.Enumerate([10, 20, 30])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 3);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Flatten
    {
        const char* src = "List.Flatten([[1, 2], [3], [4, 5]])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Flatten non-list -> :None
    {
        const char* src = "List.Flatten([[1], 2])";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Chunk
    {
        const char* src = "List.Chunk([1, 2, 3, 4, 5], 2)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Chunk n<=0 -> :None
    {
        const char* src = "List.Chunk([1, 2], 0)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Window
    {
        const char* src = "List.Window([1, 2, 3, 4], 2)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_some(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Window n > len -> :None
    {
        const char* src = "List.Window([1], 5)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // List.Window n<=0 -> :None
    {
        const char* src = "List.Window([1, 2], 0)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_is_none(s, -1));
        CHECK(on1x_pop(s, 1) == 1);
    }

    // ---- Edge Cases ----

    // Empty list Map
    {
        const char* src = "List.Map([], fn(x) { x + 1 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Empty list Filter
    {
        const char* src = "List.Filter([], fn(x) { x > 0 })";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Take saturating
    {
        const char* src = "List.Take([1, 2], 10)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 2);
        CHECK(on1x_pop(s, 1) == 1);
    }

    // Drop saturating
    {
        const char* src = "List.Drop([1, 2], 10)";
        CHECK(on1x_eval(s, src, std::strlen(src), "test") == ON1X_OK);
        CHECK(on1x_type(s, -1) == ON1X_LIST);
        CHECK(on1x_len(s, -1) == 0);
        CHECK(on1x_pop(s, 1) == 1);
    }

    on1x_close(s);

    std::fprintf(stderr, "List tests: %d failures\n", failures);
    return failures;
}
