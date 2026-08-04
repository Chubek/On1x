#include <on1x/on1x.h>

#include <cstdio>
#include <cstring>
#include <cmath>

namespace {
int failures = 0;
#define CHECK(expression) do { if (!(expression)) { ++failures; std::fprintf(stderr, "CHECK failed: %s\n", #expression); } } while (false)

On1x_Status add_ints(On1x_State* state, int argc) {
    if (argc != 2) return on1x_error(state, "Add expects two arguments");
    on1x_push_int(state, on1x_as_int(state, 1) + on1x_as_int(state, 2));
    return ON1X_OK;
}

On1x_Status fail_native(On1x_State* state, int) {
    return on1x_error(state, "intentional failure");
}

On1x_Status broken_native(On1x_State*, int) {
    return ON1X_OK;
}
}

int main() {
    On1x_State* state = on1x_open();
    CHECK(state != nullptr);
    on1x_push_int(state, 42);
    CHECK(on1x_top(state) == 1);
    CHECK(on1x_type(state, -1) == ON1X_INT);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_dup(state, -1) == ON1X_OK);
    CHECK(on1x_pop(state, 2) == 1 && on1x_top(state) == 0);
    CHECK(on1x_eval(state, "\"ok\"", 4, "test") == ON1X_OK);
    size_t length = 0;
    CHECK(std::strcmp(on1x_as_string(state, -1, &length), "ok") == 0 && length == 2);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "1+2", 3, "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 3);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "@", 1, "test") == ON1X_ERR);
    CHECK(on1x_type(state, -1) == ON1X_LIST);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    on1x_new_list(state);
    on1x_push_int(state, 7);
    CHECK(on1x_list_push(state, -2) == ON1X_OK);
    CHECK(on1x_len(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 7);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_tag_list(state, "Pair", 4, -1) == ON1X_OK);
    CHECK(on1x_tag_of(state, -1) == 1 && on1x_is_some(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_len(state, -1) == 1);
    CHECK(on1x_pop(state, 2) == 1);

    on1x_new_table(state);
    CHECK(on1x_push_tag(state, "key", 3) == ON1X_OK);
    on1x_push_int(state, 11);
    CHECK(on1x_table_set(state, -3) == ON1X_OK);
    CHECK(on1x_push_tag(state, "key", 3) == ON1X_OK);
    CHECK(on1x_table_get(state, -2) == 1 && on1x_is_some(state, -1));
    CHECK(on1x_pop(state, 2) == 1);

    on1x_new_table(state);
    on1x_new_list(state);
    on1x_push_int(state, 1);
    CHECK(on1x_table_set(state, -3) == ON1X_ERR);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 2) == 1);

    on1x_push_int(state, 9);
    CHECK(on1x_push_some(state) == ON1X_OK && on1x_is_some(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    on1x_push_none(state);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    on1x_push_unit(state);
    CHECK(on1x_push_success(state) == ON1X_OK && on1x_is_success(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    on1x_push_unit(state);
    CHECK(on1x_push_error_result(state) == ON1X_OK && on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    CHECK(on1x_register(state, "Add", add_ints) == ON1X_OK);
    CHECK(on1x_eval(state, "Add", 3, "test") == ON1X_OK);
    on1x_push_int(state, 2);
    on1x_push_int(state, 3);
    CHECK(on1x_call(state, -3, 2) == ON1X_OK);
    CHECK(on1x_top(state) == 1 && on1x_as_int(state, -1) == 5);
    CHECK(on1x_pop(state, 1) == 1);

    CHECK(on1x_register(state, "Fail", fail_native) == ON1X_OK);
    CHECK(on1x_eval(state, "Fail", 4, "test") == ON1X_OK);
    CHECK(on1x_call(state, -1, 0) == ON1X_ERR);
    CHECK(on1x_top(state) == 1 && on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    CHECK(on1x_register(state, "Broken", broken_native) == ON1X_OK);
    CHECK(on1x_eval(state, "Broken", 6, "test") == ON1X_OK);
    CHECK(on1x_call(state, -1, 0) == ON1X_ERR);
    CHECK(on1x_top(state) == 1 && on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    CHECK(on1x_push_string(state, "rooted", 6) == ON1X_OK);
    const On1x_Ref reference = on1x_ref_create(state, -1);
    CHECK(reference != 0);
    CHECK(on1x_pop(state, 1) == 1);
    on1x_gc_collect(state);
    CHECK(on1x_ref_push(state, reference) == ON1X_OK);
    CHECK(std::strcmp(on1x_as_string(state, -1, &length), "rooted") == 0 && length == 6);
    CHECK(on1x_ref_release(state, reference) == 1);
    CHECK(on1x_pop(state, 1) == 1);
#if defined(ON1X_TEST_EXPECT_FFI)
    CHECK(on1x_ffi_available() == 1);
    On1x_FfiLibrary* ffi_library = nullptr;
    CHECK(on1x_ffi_open(state, "libm.so.6", 9, &ffi_library) == ON1X_OK && ffi_library != nullptr);
    void* cosine = on1x_ffi_symbol(ffi_library, "cos", 3);
    CHECK(cosine != nullptr);
    on1x_push_float(state, 0.0);
    CHECK(on1x_ffi_call(state, cosine, "d)d", 3, 1) == ON1X_OK);
    CHECK(on1x_top(state) == 1 && std::fabs(on1x_as_float(state, -1) - 1.0) < 1e-12);
    CHECK(on1x_pop(state, 1) == 1);
    on1x_push_float(state, 0.0);
    CHECK(on1x_ffi_call(state, cosine, "?", 1, 1) == ON1X_OK && on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    on1x_push_int(state, 0);
    CHECK(on1x_ffi_call(state, cosine, "d)d", 3, 1) == ON1X_ERR && on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    on1x_ffi_close(ffi_library);
#else
    CHECK(on1x_ffi_available() == 0);
#endif
    CHECK(std::strcmp(on1x_version_string(), "0.1.0") == 0);
    on1x_close(state);
    return failures == 0 ? 0 : 1;
}
