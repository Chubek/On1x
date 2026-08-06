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

    // Cmp.Eq on Ints
    const char* eq1 = "Cmp.Eq(1, 1)";
    CHECK(on1x_eval(state, eq1, std::strlen(eq1), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);

    const char* eq2 = "Cmp.Eq(1, 2)";
    CHECK(on1x_eval(state, eq2, std::strlen(eq2), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 0);
    CHECK(on1x_pop(state, 1) == 1);

    // Cmp.Eq on Int/Float: 1 == 1.0
    const char* eq_mixed = "Cmp.Eq(1, 1.0)";
    CHECK(on1x_eval(state, eq_mixed, std::strlen(eq_mixed), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);

    // Cmp.Eq on Strings
    const char* eq_str = "Cmp.Eq(\"hello\", \"hello\")";
    CHECK(on1x_eval(state, eq_str, std::strlen(eq_str), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);

    const char* eq_str_ne = "Cmp.Eq(\"hello\", \"world\")";
    CHECK(on1x_eval(state, eq_str_ne, std::strlen(eq_str_ne), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 0);
    CHECK(on1x_pop(state, 1) == 1);

    // Cmp.Eq on Lists (structural)
    const char* eq_list = "Cmp.Eq([1, 2, 3], [1, 2, 3])";
    CHECK(on1x_eval(state, eq_list, std::strlen(eq_list), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);

    const char* eq_list_ne = "Cmp.Eq([1, 2], [1, 2, 3])";
    CHECK(on1x_eval(state, eq_list_ne, std::strlen(eq_list_ne), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 0);
    CHECK(on1x_pop(state, 1) == 1);

    // Cmp.DeepEq on Tables
    const char* deepeq = "Cmp.DeepEq(%{ :x => 1 }, %{ :x => 1 })";
    CHECK(on1x_eval(state, deepeq, std::strlen(deepeq), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);

    const char* deepeq_ne = "Cmp.DeepEq(%{ :x => 1 }, %{ :x => 2 })";
    CHECK(on1x_eval(state, deepeq_ne, std::strlen(deepeq_ne), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 0);
    CHECK(on1x_pop(state, 1) == 1);

    // Cmp.DeepEq nested
    const char* deepeq_nest = "Cmp.DeepEq([1, [2, 3]], [1, [2, 3]])";
    CHECK(on1x_eval(state, deepeq_nest, std::strlen(deepeq_nest), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);

    // Cmp.Compare: ints
    const char* cmp12 = "Cmp.Compare(1, 2)";
    CHECK(on1x_eval(state, cmp12, std::strlen(cmp12), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == -1);
    CHECK(on1x_pop(state, 3) == 1);

    const char* cmp21 = "Cmp.Compare(2, 1)";
    CHECK(on1x_eval(state, cmp21, std::strlen(cmp21), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 1);
    CHECK(on1x_pop(state, 3) == 1);

    const char* cmp_eq = "Cmp.Compare(1, 1)";
    CHECK(on1x_eval(state, cmp_eq, std::strlen(cmp_eq), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 0);
    CHECK(on1x_pop(state, 3) == 1);

    // Cmp.Hash on hashable values -> :Some
    const char* hash_int = "Cmp.Hash(42)";
    CHECK(on1x_eval(state, hash_int, std::strlen(hash_int), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    const char* hash_str = "Cmp.Hash(\"hello\")";
    CHECK(on1x_eval(state, hash_str, std::strlen(hash_str), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    // Cmp.Hash on unhashable (List) -> :None
    const char* hash_list = "Cmp.Hash([1, 2])";
    CHECK(on1x_eval(state, hash_list, std::strlen(hash_list), "test") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    // Cmp.MinOf / Cmp.MaxOf
    const char* min_chunk = "Cmp.MinOf([3, 1, 4, 1, 5])";
    CHECK(on1x_eval(state, min_chunk, std::strlen(min_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 1);
    CHECK(on1x_pop(state, 3) == 1);

    const char* max_chunk = "Cmp.MaxOf([3, 1, 4, 1, 5])";
    CHECK(on1x_eval(state, max_chunk, std::strlen(max_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 5);
    CHECK(on1x_pop(state, 3) == 1);

    // Cmp.MinOf on empty -> :None
    const char* min_empty = "Cmp.MinOf([])";
    CHECK(on1x_eval(state, min_empty, std::strlen(min_empty), "test") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    on1x_close(state);
    return failures;
}
