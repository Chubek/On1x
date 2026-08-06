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

    // Bit.And
    const char* and_chunk = "Bit.And(12, 10)";
    CHECK(on1x_eval(state, and_chunk, std::strlen(and_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 8);
    CHECK(on1x_pop(state, 1) == 1);

    // Bit.Or
    const char* or_chunk = "Bit.Or(12, 10)";
    CHECK(on1x_eval(state, or_chunk, std::strlen(or_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 14);
    CHECK(on1x_pop(state, 1) == 1);

    // Bit.Xor
    const char* xor_chunk = "Bit.Xor(12, 10)";
    CHECK(on1x_eval(state, xor_chunk, std::strlen(xor_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 6);
    CHECK(on1x_pop(state, 1) == 1);

    // Bit.Not
    const char* not_chunk = "Bit.Not(0)";
    CHECK(on1x_eval(state, not_chunk, std::strlen(not_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == -1);
    CHECK(on1x_pop(state, 1) == 1);

    // Bit.Shl
    const char* shl_chunk = "Bit.Shl(1, 3)";
    CHECK(on1x_eval(state, shl_chunk, std::strlen(shl_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 8);
    CHECK(on1x_pop(state, 1) == 1);

    // Bit.Shr
    const char* shr_chunk = "Bit.Shr(8, 3)";
    CHECK(on1x_eval(state, shr_chunk, std::strlen(shr_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);

    // Bit.Sar (arithmetic shift on negative)
    const char* sar_chunk = "Bit.Sar(-8, 3)";
    CHECK(on1x_eval(state, sar_chunk, std::strlen(sar_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == -1);
    CHECK(on1x_pop(state, 1) == 1);

    // Bit.PopCount
    const char* popcount_chunk = "Bit.PopCount(11)";
    CHECK(on1x_eval(state, popcount_chunk, std::strlen(popcount_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 3);
    CHECK(on1x_pop(state, 1) == 1);

    // Bit.Clz / Bit.Ctz
    const char* clz_chunk = "Bit.Clz(0)";
    CHECK(on1x_eval(state, clz_chunk, std::strlen(clz_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 64);
    CHECK(on1x_pop(state, 1) == 1);

    const char* ctz_chunk = "Bit.Ctz(0)";
    CHECK(on1x_eval(state, ctz_chunk, std::strlen(ctz_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 64);
    CHECK(on1x_pop(state, 1) == 1);

    // Bit.Test
    const char* test_chunk = "Bit.Test(8, 3)";
    CHECK(on1x_eval(state, test_chunk, std::strlen(test_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 3) == 1);

    const char* test_off = "Bit.Test(8, 0)";
    CHECK(on1x_eval(state, test_off, std::strlen(test_off), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_bool(state, -1) == 0);
    CHECK(on1x_pop(state, 3) == 1);

    // Bit.Set
    const char* set_chunk = "Bit.Set(0, 3)";
    CHECK(on1x_eval(state, set_chunk, std::strlen(set_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 8);
    CHECK(on1x_pop(state, 3) == 1);

    // Bit.Clear
    const char* clear_chunk = "Bit.Clear(15, 1)";
    CHECK(on1x_eval(state, clear_chunk, std::strlen(clear_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 13);
    CHECK(on1x_pop(state, 3) == 1);

    // Bit.Toggle
    const char* toggle_chunk = "Bit.Toggle(10, 0)";
    CHECK(on1x_eval(state, toggle_chunk, std::strlen(toggle_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 11);
    CHECK(on1x_pop(state, 3) == 1);

    // Out-of-range bit index -> :None
    const char* test_oob = "Bit.Test(0, 64)";
    CHECK(on1x_eval(state, test_oob, std::strlen(test_oob), "test") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    on1x_close(state);
    return failures;
}
