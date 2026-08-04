#include "core/equality.hpp"
#include "core/hashing.hpp"
#include "core/iota.hpp"
#include "core/list.hpp"
#include "core/number.hpp"
#include "core/optional.hpp"
#include "core/reserved_tags.hpp"
#include "core/result.hpp"
#include "core/string.hpp"
#include "core/table.hpp"
#include "core/tagged_list.hpp"
#include "core/tostring.hpp"
#include "core/type.hpp"
#include "gc/gc.hpp"

#include <cstdint>
#include <cstdio>
#include <limits>
#include <stdexcept>

namespace {

int failures = 0;

#define CHECK(expression)                                                     \
    do {                                                                      \
        if (!(expression)) {                                                  \
            std::fprintf(stderr, "CHECK failed at %s:%d: %s\n",              \
                         __FILE__, __LINE__, #expression);                   \
            ++failures;                                                       \
        }                                                                     \
    } while (false)

void test_scalars(on1x::GcState* gc, const on1x::ReservedTags& tags) {
    CHECK(on1x::Value::integer(gc, -12).as_int() == -12);
    const auto large = on1x::Value::integer(gc, std::int64_t{1} << 47);
    CHECK(large.is_int());
    CHECK(large.as_int() == (std::int64_t{1} << 47));
    CHECK(on1x::value_equals(on1x::Value::integer(gc, 1), on1x::Value::floating(1.0)));
    CHECK(!on1x::value_equals(on1x::Value::floating(1.5), on1x::Value::integer(gc, 1)));
    CHECK(on1x::type_of(on1x::Value::integer(gc, 3), tags) == on1x::value_from_object(tags.integer));
    auto* string = on1x::new_string(gc, "A\xF0\x9F\x98\x80");
    CHECK(on1x::string_view(string) == "A\xF0\x9F\x98\x80");
    CHECK(on1x::to_string(on1x::value_from_object(string)) == "\"A\xF0\x9F\x98\x80\"");
    std::uint8_t byte = 0;
    CHECK(on1x::string_byte_at(string, 1, byte) && byte == 0xf0U);
    CHECK(on1x::string_codepoint_at(gc, string, 1) &&
          on1x::string_view(on1x::string_codepoint_at(gc, string, 1)) == "\xF0\x9F\x98\x80");
}

void test_lists_and_tags(
    on1x::GcState* gc,
    on1x::TagTable& table,
    const on1x::ReservedTags& tags) {
    auto* point = table.intern(gc, "Point");
    CHECK(point == table.intern(gc, "Point"));
    auto* list = on1x::new_tagged_list(gc, point, 2);
    CHECK(on1x::list_push(gc, list, on1x::Value::integer(gc, 3)));
    CHECK(on1x::list_push(gc, list, on1x::Value::integer(gc, 4)));
    const on1x::Value tagged = on1x::value_from_object(list);
    CHECK(on1x::type_of(tagged, tags) == on1x::value_from_object(tags.list));
    on1x::Value unwrapped;
    CHECK(on1x::unwrap_some(on1x::tag_of(gc, tags, tagged), tags, unwrapped));
    CHECK(unwrapped == on1x::value_from_object(point));
    const on1x::Value payload = on1x::payload_of(gc, tags, tagged);
    CHECK(on1x::as_list_const(payload)->constructor == nullptr);
    CHECK(on1x::as_list_const(payload)->length == 2);
}

void test_tables(on1x::GcState* gc) {
    auto* left = on1x::new_table(gc);
    auto* right = on1x::new_table(gc);
    CHECK(on1x::table_set(gc, left, on1x::Value::integer(gc, 1), on1x::Value::boolean(true)));
    CHECK(on1x::table_set(gc, right, on1x::Value::integer(gc, 1), on1x::Value::boolean(true)));
    CHECK(on1x::value_equals(on1x::value_from_object(left), on1x::value_from_object(right)));
    on1x::Value found;
    CHECK(on1x::table_get(left, on1x::Value::integer(gc, 1), found));
    CHECK(found.as_bool());
    CHECK(!on1x::table_set(gc, left, on1x::value_from_object(on1x::new_list(gc)), on1x::Value::unit()));
    CHECK(!on1x::is_hashable(on1x::value_from_object(left)));
}

void test_optionals_results_iota(on1x::GcState* gc, const on1x::ReservedTags& tags) {
    const on1x::Value some = on1x::make_some(gc, tags, on1x::Value::integer(gc, 9));
    const on1x::Value none = on1x::make_none(gc, tags);
    on1x::Value payload;
    CHECK(on1x::unwrap_some(some, tags, payload) && payload.as_int() == 9);
    CHECK(on1x::is_none(none, tags));
    CHECK(on1x::is_success(on1x::make_success(gc, tags, some), tags));
    CHECK(on1x::is_error(on1x::make_error(gc, tags, none), tags));

    const on1x::Value zero = on1x::make_iota(gc, tags, {on1x::Value::integer(gc, 0)});
    CHECK(on1x::as_list_const(zero)->length == 0);
    CHECK(on1x::is_none(on1x::make_iota(gc, tags, {on1x::Value::integer(gc, -3)}), tags));
    CHECK(on1x::is_none(on1x::make_iota(gc, tags, {on1x::Value::floating(2.5)}), tags));
    const on1x::Value descending = on1x::make_iota(
        gc, tags, {on1x::Value::integer(gc, 6), on1x::Value::integer(gc, 2), on1x::Value::integer(gc, -1)});
    const auto* range = on1x::as_list_const(descending);
    CHECK(range->length == 4 && range->items[0].as_int() == 6 && range->items[3].as_int() == 3);
}

void test_numbers(on1x::GcState* gc) {
    const auto maximum = std::numeric_limits<std::int64_t>::max();
    CHECK(on1x::numeric_apply(
              gc, on1x::ArithmeticOp::Add, on1x::Value::integer(gc, maximum), on1x::Value::integer(gc, 1)).as_int() ==
          std::numeric_limits<std::int64_t>::min());
    bool threw = false;
    try {
        (void)on1x::numeric_apply(
            gc, on1x::ArithmeticOp::Divide, on1x::Value::integer(gc, 1), on1x::Value::integer(gc, 0));
    } catch (const std::domain_error&) {
        threw = true;
    }
    CHECK(threw);
}

}  // namespace

int main() {
    on1x::GcState gc;
    on1x::gc_init(&gc);
    on1x::TagTable tags;
    const on1x::ReservedTags reserved = on1x::make_reserved_tags(&gc, tags);

    test_scalars(&gc, reserved);
    test_lists_and_tags(&gc, tags, reserved);
    test_tables(&gc);
    test_optionals_results_iota(&gc, reserved);
    test_numbers(&gc);

    on1x::gc_shutdown(&gc);
    if (failures != 0) {
        std::fprintf(stderr, "%d core test(s) failed\n", failures);
        return 1;
    }
    return 0;
}
