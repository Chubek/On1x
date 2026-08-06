#include <on1x/on1x.h>
#include <cstdio>
#include <cstring>
#include <cmath>

namespace {
int failures = 0;
#define CHECK(expr) do { if (!(expr)) { ++failures; std::fprintf(stderr, "CHECK failed [%s:%d]: %s\n", __FILE__, __LINE__, #expr); } } while (false)
}

int main() {
    On1x_State* state = on1x_open();
    CHECK(state != nullptr);
    CHECK(on1x_open_std(state) == ON1X_OK);

    // Math.Sqrt(-1) -> :None
    const char* sqrt_neg1 = "Math.Sqrt(-1)";
    CHECK(on1x_eval(state, sqrt_neg1, std::strlen(sqrt_neg1), "sqrt_neg1") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    // Math.Log(0) -> :None
    const char* log_zero = "Math.Log(0)";
    CHECK(on1x_eval(state, log_zero, std::strlen(log_zero), "log_zero") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    // Math.Log(-1) -> :None
    const char* log_neg = "Math.Log(-1)";
    CHECK(on1x_eval(state, log_neg, std::strlen(log_neg), "log_neg") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    // Math.Log2(0) -> :None
    const char* log2_zero = "Math.Log2(0)";
    CHECK(on1x_eval(state, log2_zero, std::strlen(log2_zero), "log2_zero") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    // Math.Log10(0) -> :None
    const char* log10_zero = "Math.Log10(0)";
    CHECK(on1x_eval(state, log10_zero, std::strlen(log10_zero), "log10_zero") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    // Math.Asin(2) -> :None
    const char* asin_2 = "Math.Asin(2)";
    CHECK(on1x_eval(state, asin_2, std::strlen(asin_2), "asin_2") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    // Math.Acos(2) -> :None
    const char* acos_2 = "Math.Acos(2)";
    CHECK(on1x_eval(state, acos_2, std::strlen(acos_2), "acos_2") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    // Math.DivMod(x, 0) -> :None
    const char* divmod_zero = "Math.DivMod(10, 0)";
    CHECK(on1x_eval(state, divmod_zero, std::strlen(divmod_zero), "divmod_zero") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    // Math.Sqrt(4) -> :Some[2.0]
    const char* sqrt_4 = "Math.Sqrt(4)";
    CHECK(on1x_eval(state, sqrt_4, std::strlen(sqrt_4), "sqrt_4") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    // payload_of pushes payload, then list_get pushes element
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    double val = on1x_as_float(state, -1);
    CHECK(std::fabs(val - 2.0) < 1e-9);
    CHECK(on1x_pop(state, 3) == 1);

    // Math.Log(1) -> :Some[0.0]
    const char* log_1 = "Math.Log(1)";
    CHECK(on1x_eval(state, log_1, std::strlen(log_1), "log_1") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    val = on1x_as_float(state, -1);
    CHECK(std::fabs(val - 0.0) < 1e-9);
    CHECK(on1x_pop(state, 3) == 1);

    // Math.Asin(0.5) -> :Some[...]
    const char* asin_half = "Math.Asin(0.5)";
    CHECK(on1x_eval(state, asin_half, std::strlen(asin_half), "asin_half") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    // Math.DivMod(10, 3) -> :Some[[3, 1]]
    const char* divmod_valid = "Math.DivMod(10, 3)";
    CHECK(on1x_eval(state, divmod_valid, std::strlen(divmod_valid), "divmod_valid") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    // payload is [[3, 1]] — a list containing the inner list; unwrap inner list first
    CHECK(on1x_list_get(state, -1, 0) == 1);  // pushes [3, 1]
    CHECK(on1x_list_get(state, -1, 0) == 1);  // pushes 3
    CHECK(on1x_as_int(state, -1) == 3);
    CHECK(on1x_list_get(state, -2, 1) == 1);  // list [3,1] is now at -2; get element 1
    CHECK(on1x_as_int(state, -1) == 1);
    CHECK(on1x_pop(state, 5) == 1);

    // Math.Abs
    const char* abs_neg = "Math.Abs(-5)";
    CHECK(on1x_eval(state, abs_neg, std::strlen(abs_neg), "abs") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 5);
    CHECK(on1x_pop(state, 1) == 1);

    const char* abs_float = "Math.Abs(-3.14)";
    CHECK(on1x_eval(state, abs_float, std::strlen(abs_float), "abs_float") == ON1X_OK);
    CHECK(std::fabs(on1x_as_float(state, -1) - 3.14) < 1e-9);
    CHECK(on1x_pop(state, 1) == 1);

    // Math.Min/Max
    const char* min_chunk = "Math.Min(3, 7)";
    CHECK(on1x_eval(state, min_chunk, std::strlen(min_chunk), "min") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 3);
    CHECK(on1x_pop(state, 1) == 1);

    const char* max_chunk = "Math.Max(3, 7)";
    CHECK(on1x_eval(state, max_chunk, std::strlen(max_chunk), "max") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 7);
    CHECK(on1x_pop(state, 1) == 1);

    // Math.Sign
    const char* sign_neg = "Math.Sign(-42)";
    CHECK(on1x_eval(state, sign_neg, std::strlen(sign_neg), "sign_neg") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == -1);
    CHECK(on1x_pop(state, 1) == 1);

    const char* sign_zero = "Math.Sign(0)";
    CHECK(on1x_eval(state, sign_zero, std::strlen(sign_zero), "sign_zero") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 0);
    CHECK(on1x_pop(state, 1) == 1);

    const char* sign_pos = "Math.Sign(42)";
    CHECK(on1x_eval(state, sign_pos, std::strlen(sign_pos), "sign_pos") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);

    // Math.Gcd
    const char* gcd_chunk = "Math.Gcd(48, 18)";
    CHECK(on1x_eval(state, gcd_chunk, std::strlen(gcd_chunk), "gcd") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 6);
    CHECK(on1x_pop(state, 1) == 1);

    // Math.Clamp
    const char* clamp_in = "Math.Clamp(5, 0, 10)";
    CHECK(on1x_eval(state, clamp_in, std::strlen(clamp_in), "clamp_in") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 5);
    CHECK(on1x_pop(state, 1) == 1);

    const char* clamp_lo = "Math.Clamp(-5, 0, 10)";
    CHECK(on1x_eval(state, clamp_lo, std::strlen(clamp_lo), "clamp_lo") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 0);
    CHECK(on1x_pop(state, 1) == 1);

    const char* clamp_hi = "Math.Clamp(15, 0, 10)";
    CHECK(on1x_eval(state, clamp_hi, std::strlen(clamp_hi), "clamp_hi") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 10);
    CHECK(on1x_pop(state, 1) == 1);

    // Math.Pow
    const char* pow_chunk = "Math.Pow(2, 8)";
    CHECK(on1x_eval(state, pow_chunk, std::strlen(pow_chunk), "pow") == ON1X_OK);
    CHECK(std::fabs(on1x_as_float(state, -1) - 256.0) < 1e-9);
    CHECK(on1x_pop(state, 1) == 1);

    on1x_close(state);
    return failures;
}
