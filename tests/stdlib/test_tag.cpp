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

    // Tag.Name
    const char* name_chunk = "Tag.Name(:point)";
    CHECK(on1x_eval(state, name_chunk, std::strlen(name_chunk), "test") == ON1X_OK);
    CHECK(std::strcmp(on1x_as_string(state, -1, &len), "point") == 0);
    CHECK(len == 5);
    CHECK(on1x_pop(state, 1) == 1);

    // Tag.FromString
    const char* from_chunk = "Tag.FromString(\"point\")";
    CHECK(on1x_eval(state, from_chunk, std::strlen(from_chunk), "test") == ON1X_OK);
    CHECK(on1x_type(state, -1) == ON1X_TAG);
    CHECK(std::strcmp(on1x_as_string(state, -1, &len), "point") == 0);
    CHECK(on1x_pop(state, 1) == 1);

    // Tag.FromString identity (interning)
    const char* interning_chunk = "Cmp.Eq(Tag.FromString(\"hey\"), :hey)";
    CHECK(on1x_eval(state, interning_chunk, std::strlen(interning_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);

    // Tag.Attach on a plain list
    const char* attach_chunk = "Tag.Attach(:ok, [1, 2])";
    CHECK(on1x_eval(state, attach_chunk, std::strlen(attach_chunk), "test") == ON1X_OK);
    CHECK(on1x_type(state, -1) == ON1X_LIST);
    // tag_of returns :Some[tag] (or :None); unwrap to get the constructor tag
    CHECK(on1x_tag_of(state, -1) == 1);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_list_get(state, -1, 0) == 1);
    CHECK(std::strcmp(on1x_as_string(state, -1, &len), "ok") == 0);
    CHECK(on1x_pop(state, 3) == 1);
    // now check the payload list
    CHECK(on1x_payload_of(state, -1) == 1);
    CHECK(on1x_len(state, -1) == 2);
    CHECK(on1x_pop(state, 2) == 1);

    // Tag.Is: test if list has specific constructor
    const char* is_chunk = "Tag.Is(:ok[1, 2], :ok)";
    CHECK(on1x_eval(state, is_chunk, std::strlen(is_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);

    const char* is_not_chunk = "Tag.Is(:ok[1, 2], :err)";
    CHECK(on1x_eval(state, is_not_chunk, std::strlen(is_not_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 0);
    CHECK(on1x_pop(state, 1) == 1);

    // Tag.Is on untagged list
    const char* is_plain_chunk = "Tag.Is([1, 2], :ok)";
    CHECK(on1x_eval(state, is_plain_chunk, std::strlen(is_plain_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 0);
    CHECK(on1x_pop(state, 1) == 1);

    on1x_close(state);
    return failures;
}
