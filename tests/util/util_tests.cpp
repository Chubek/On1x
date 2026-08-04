#include "util/arena.hpp"
#include "util/bitops.hpp"
#include "util/hash.hpp"
#include "util/platform.hpp"
#include "util/small_vector.hpp"
#include "util/span.hpp"
#include "util/string_builder.hpp"
#include "util/utf8.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace {

int failures = 0;

#define CHECK(expression)                                                     \
    do {                                                                      \
        if (!(expression)) {                                                  \
            std::fprintf(stderr, "CHECK failed at %s:%d: %s\n",              \
                         __FILE__, __LINE__, #expression);                     \
            ++failures;                                                       \
        }                                                                     \
    } while (false)

struct Pair {
    std::int64_t left;
    std::int64_t right;
};

void test_arena() {
    on1x::Arena arena(32);
    auto* first = arena.make<Pair>(1, 2);
    auto* second = static_cast<std::uint64_t*>(
        arena.allocate(sizeof(std::uint64_t), alignof(std::uint64_t)));

    CHECK(first->left == 1);
    CHECK(first->right == 2);
    CHECK(reinterpret_cast<std::uintptr_t>(second) % alignof(std::uint64_t) == 0);
    CHECK(arena.bytes_allocated() == sizeof(Pair) + sizeof(std::uint64_t));

    arena.reset();
    CHECK(arena.bytes_allocated() == 0);
}

void test_small_vector() {
    on1x::SmallVector<std::string, 2> values;
    values.emplace_back("one");
    values.emplace_back("two");
    values.emplace_back("three");

    CHECK(values.size() == 3);
    CHECK(values.capacity() >= 3);
    CHECK(values[0] == "one");
    CHECK(values.back() == "three");

    on1x::SmallVector<std::string, 2> copy = values;
    values.pop_back();
    CHECK(values.size() == 2);
    CHECK(copy.size() == 3);

    on1x::SmallVector<std::string, 2> moved = std::move(copy);
    CHECK(moved.size() == 3);
    CHECK(moved[2] == "three");
    CHECK(copy.empty());
}

void test_span_and_bitops() {
    int values[] = {2, 4, 6};
    on1x::Span<int> span(values);

    CHECK(span.size() == 3);
    CHECK(span[1] == 4);
    CHECK(on1x::is_power_of_two(64));
    CHECK(!on1x::is_power_of_two(63));
    CHECK(on1x::align_up(65, 16) == 80);
}

void test_hashing() {
    CHECK(on1x::hash_string("") == on1x::fnv1a_offset_basis);
    CHECK(on1x::hash_string("hello") == 0xa430d84680aabd0bULL);
    CHECK(on1x::hash_string("hello") != on1x::hash_string("Hello"));
}

void test_utf8() {
    const std::string text = "A\xC2\xA2\xE2\x82\xAC\xF0\x9F\x98\x80";
    CHECK(on1x::utf8::validate(text));
    CHECK(!on1x::utf8::validate("\xC0\xAF"));
    CHECK(!on1x::utf8::validate("\xED\xA0\x80"));
    CHECK(!on1x::utf8::validate("\xF4\x90\x80\x80"));
    CHECK(!on1x::utf8::validate("\xE2\x82"));

    on1x::utf8::DecodeResult result;
    CHECK(on1x::utf8::decode_next(text, 1, result));
    CHECK(result.code_point == U'\u00A2');
    CHECK(result.width == 2);

    std::string encoded;
    CHECK(on1x::utf8::append(encoded, U'\U0001F600'));
    CHECK(encoded == "\xF0\x9F\x98\x80");
    CHECK(!on1x::utf8::append(encoded, static_cast<char32_t>(0x110000)));
}

void test_string_builder() {
    on1x::StringBuilder builder(16);
    builder.append("On");
    builder.append('1');
    builder.append("x ");
    CHECK(builder.append_code_point(U'\u2713'));
    CHECK(builder.view() == "On1x \xE2\x9C\x93");

    std::string result = builder.take();
    CHECK(result == "On1x \xE2\x9C\x93");
    CHECK(builder.empty());
}

void test_platform() {
    CHECK(on1x::page_size() > 0);
    CHECK(on1x::operating_system() != on1x::OperatingSystem::Other);
    CHECK(on1x::architecture() != on1x::Architecture::Other);
}

}

int main() {
    test_arena();
    test_small_vector();
    test_span_and_bitops();
    test_hashing();
    test_utf8();
    test_string_builder();
    test_platform();

    if (failures != 0) {
        std::fprintf(stderr, "%d utility test(s) failed\n", failures);
        return 1;
    }
    return 0;
}
