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
    size_t len = 0;

    // Str.CharLen: codepoints
    const char* charlen_ascii = "Str.CharLen(\"hello\")";
    CHECK(on1x_eval(state, charlen_ascii, std::strlen(charlen_ascii), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 5);
    CHECK(on1x_pop(state, 1) == 1);

    // Multi-byte: "é" is 1 codepoint
    const char* charlen_utf8 = "Str.CharLen(\"é\")";
    CHECK(on1x_eval(state, charlen_utf8, std::strlen(charlen_utf8), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);

    // Str.ByteLen: raw bytes (é = 2 bytes in UTF-8)
    const char* bytelen_chunk = "Str.ByteLen(\"é\")";
    CHECK(on1x_eval(state, bytelen_chunk, std::strlen(bytelen_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 2);
    CHECK(on1x_pop(state, 1) == 1);

    // Str.Upper
    const char* upper_chunk = "Str.Upper(\"hello\")";
    CHECK(on1x_eval(state, upper_chunk, std::strlen(upper_chunk), "test") == ON1X_OK);
    CHECK(std::strcmp(on1x_as_string(state, -1, &len), "HELLO") == 0);
    CHECK(on1x_pop(state, 1) == 1);

    // Str.Lower
    const char* lower_chunk = "Str.Lower(\"HELLO\")";
    CHECK(on1x_eval(state, lower_chunk, std::strlen(lower_chunk), "test") == ON1X_OK);
    CHECK(std::strcmp(on1x_as_string(state, -1, &len), "hello") == 0);
    CHECK(on1x_pop(state, 1) == 1);

    // Str.Trim
    const char* trim_chunk = "Str.Trim(\"  hello  \")";
    CHECK(on1x_eval(state, trim_chunk, std::strlen(trim_chunk), "test") == ON1X_OK);
    CHECK(std::strcmp(on1x_as_string(state, -1, &len), "hello") == 0);
    CHECK(on1x_pop(state, 1) == 1);

    // Str.ToInt valid
    const char* toint_chunk = "Str.ToInt(\"42\")";
    CHECK(on1x_eval(state, toint_chunk, std::strlen(toint_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 3) == 1);

    // Str.ToInt invalid -> :None
    const char* toint_bad = "Str.ToInt(\"abc\")";
    CHECK(on1x_eval(state, toint_bad, std::strlen(toint_bad), "test") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    // Str.FromInt
    const char* fromint_chunk = "Str.FromInt(-42)";
    CHECK(on1x_eval(state, fromint_chunk, std::strlen(fromint_chunk), "test") == ON1X_OK);
    CHECK(std::strcmp(on1x_as_string(state, -1, &len), "-42") == 0);
    CHECK(on1x_pop(state, 1) == 1);

    // Str.ToTag
    const char* totag_chunk = "Str.ToTag(\"point\")";
    CHECK(on1x_eval(state, totag_chunk, std::strlen(totag_chunk), "test") == ON1X_OK);
    CHECK(on1x_type(state, -1) == ON1X_TAG);
    CHECK(std::strcmp(on1x_as_string(state, -1, &len), "point") == 0);
    CHECK(on1x_pop(state, 1) == 1);

    // Str.Repeat
    const char* repeat_chunk = "Str.Repeat(\"ab\", 3)";
    CHECK(on1x_eval(state, repeat_chunk, std::strlen(repeat_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    CHECK(std::strcmp(on1x_as_string(state, -1, &len), "ababab") == 0);
    CHECK(on1x_pop(state, 3) == 1);

    // Str.Repeat negative -> :None
    const char* repeat_neg = "Str.Repeat(\"ab\", -1)";
    CHECK(on1x_eval(state, repeat_neg, std::strlen(repeat_neg), "test") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    on1x_close(state);
    return failures;
}
