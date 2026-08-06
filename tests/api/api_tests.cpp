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
    CHECK(on1x_open_std(state) == ON1X_OK);
    const char* math_chunk = "Math.Sqrt(-1)";
    CHECK(on1x_eval(state, math_chunk, std::strlen(math_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    const char* bit_chunk = "Bit.Test(8, 3)";
    CHECK(on1x_eval(state, bit_chunk, std::strlen(bit_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 3) == 1);
    const char* tag_chunk = "Tag.Name(:point)";
    CHECK(on1x_eval(state, tag_chunk, std::strlen(tag_chunk), "test") == ON1X_OK);
    size_t length = 0;
    CHECK(std::strcmp(on1x_as_string(state, -1, &length), "point") == 0 && length == 5);
    CHECK(on1x_pop(state, 1) == 1);
    const char* str_chunk = "Str.CharLen(\"é\")";
    CHECK(on1x_eval(state, str_chunk, std::strlen(str_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);
    on1x_gc_collect(state);
    const char* native_after_gc_chunk = "IsNone(?)";
    CHECK(on1x_eval(
        state, native_after_gc_chunk, std::strlen(native_after_gc_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);
    const char* cmp_eq_chunk = "Cmp.Eq([1, %{ :x => 2 }], [1, %{ :x => 2 }])";
    CHECK(on1x_eval(
        state, cmp_eq_chunk, std::strlen(cmp_eq_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);
    const char* cmp_compare_chunk = "Cmp.Compare(1, 2)";
    CHECK(on1x_eval(
        state, cmp_compare_chunk, std::strlen(cmp_compare_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == -1);
    CHECK(on1x_pop(state, 3) == 1);
    const char* cmp_hash_chunk = "Cmp.Hash([1])";
    CHECK(on1x_eval(state, cmp_hash_chunk, std::strlen(cmp_hash_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    const char* cmp_min_chunk = "Cmp.MinOf([3, 1, 2])";
    CHECK(on1x_eval(state, cmp_min_chunk, std::strlen(cmp_min_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 1);
    CHECK(on1x_pop(state, 3) == 1);
    const char* cmp_max_chunk = "Cmp.MaxOf([])";
    CHECK(on1x_eval(state, cmp_max_chunk, std::strlen(cmp_max_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    on1x_push_int(state, 42);
    CHECK(on1x_top(state) == 1);
    CHECK(on1x_type(state, -1) == ON1X_INT);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_dup(state, -1) == ON1X_OK);
    CHECK(on1x_pop(state, 2) == 1 && on1x_top(state) == 0);
    CHECK(on1x_eval(state, "\"ok\"", 4, "test") == ON1X_OK);
    CHECK(std::strcmp(on1x_as_string(state, -1, &length), "ok") == 0 && length == 2);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "1+2", 3, "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 3);
    CHECK(on1x_pop(state, 1) == 1);
    const char* binding_chunk = "let answer = 40 + 2\nanswer = answer + 1\nanswer";
    CHECK(on1x_eval(state, binding_chunk, std::strlen(binding_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 43);
    CHECK(on1x_pop(state, 1) == 1);
    const char* function_chunk =
        "fn add(a, b) { return a + b }\n"
        "add(20, 22)";
    CHECK(on1x_eval(state, function_chunk, std::strlen(function_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 1) == 1);
    const char* enum_chunk =
        "let Color = enum { Red = Iota, Green = Iota, Blue = 7 }\n"
        "Color";
    CHECK(on1x_eval(state, enum_chunk, std::strlen(enum_chunk), "test") == ON1X_OK);
    CHECK(on1x_type(state, -1) == ON1X_TABLE);
    CHECK(on1x_push_tag(state, "Red", 3) == ON1X_OK);
    CHECK(on1x_table_get(state, -2) == 1 && on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 0);
    CHECK(on1x_pop(state, 4) == 1);
    const char* enum_reset_chunk =
        "let First = enum { A = Iota, B = Iota }\n"
        "let Second = enum { C = Iota }\n"
        "Second";
    CHECK(on1x_eval(state, enum_reset_chunk, std::strlen(enum_reset_chunk), "test") == ON1X_OK);
    CHECK(on1x_push_tag(state, "C", 1) == ON1X_OK);
    CHECK(on1x_table_get(state, -2) == 1 && on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 0);
    CHECK(on1x_pop(state, 4) == 1);
    const char* for_chunk =
        "fn sum() { let total = 0\n"
        "           for value in [1, 2, 3] { total = total + value }\n"
        "           total }\n"
        "sum()";
    CHECK(on1x_eval(state, for_chunk, std::strlen(for_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 6);
    CHECK(on1x_pop(state, 1) == 1);
    const char* table_for_chunk =
        "fn count() { let total = 0\n"
        "             for key in %{ :one => 1, :two => 2 } { total = total + 1 }\n"
        "             total }\n"
        "count()";
    CHECK(on1x_eval(state, table_for_chunk, std::strlen(table_for_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 2);
    CHECK(on1x_pop(state, 1) == 1);
    const char* loop_control_chunk =
        "while true { break }\n"
        "for value in [1, 2] { continue }\n"
        "42";
    CHECK(on1x_eval(state, loop_control_chunk, std::strlen(loop_control_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "while 1 { break }", 17, "test") == ON1X_ERR);
    CHECK(on1x_pop(state, 1) == 1);
    const char* closure_chunk =
        "let make_adder = fn(base) { fn(value) { base + value } }\n"
        "let add_two = make_adder(2)\n"
        "add_two(40)";
    CHECK(on1x_eval(state, closure_chunk, std::strlen(closure_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 1) == 1);
    const char* deep_closure_chunk =
        "let outer = fn(base) { fn() { fn(value) { base + value } } }\n"
        "outer(2)()(40)";
    CHECK(on1x_eval(state, deep_closure_chunk, std::strlen(deep_closure_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 1) == 1);
    const char* rest_chunk = "fn collect(first, rest..) { rest }\ncollect(1, 2, 3)";
    CHECK(on1x_eval(state, rest_chunk, std::strlen(rest_chunk), "test") == ON1X_OK);
    CHECK(on1x_type(state, -1) == ON1X_LIST && on1x_len(state, -1) == 2);
    CHECK(on1x_pop(state, 1) == 1);
    const char* range_for_chunk =
        "fn sum_range() { let total = 0\n"
        "                 for value in 1 .. 4 { total = total + value }\n"
        "                 total }\n"
        "sum_range()";
    CHECK(on1x_eval(state, range_for_chunk, std::strlen(range_for_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 6);
    CHECK(on1x_pop(state, 1) == 1);
    const char* nested_loop_chunk =
        "fn nested() { let total = 0\n"
        "              for outer in [1, 2] { for inner in [1, 2] { break }\n"
        "                                     total = total + 1 }\n"
        "              total }\n"
        "nested()";
    CHECK(on1x_eval(state, nested_loop_chunk, std::strlen(nested_loop_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 2);
    CHECK(on1x_pop(state, 1) == 1);
    const char* early_return_chunk =
        "fn choose(flag) { if flag { return 42 }\n"
        "                 0 }\n"
        "choose(true)";
    CHECK(on1x_eval(state, early_return_chunk, std::strlen(early_return_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 1) == 1);
    const char* conditional_chunk =
        "let value = 10\n"
        "if true { let value = 20\n"
        "          value + 1 } else { 0 }\n"
        "value";
    CHECK(on1x_eval(state, conditional_chunk, std::strlen(conditional_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 10);
    CHECK(on1x_pop(state, 1) == 1);
    const char* false_branch = "if false { 1 } else { 2 }";
    CHECK(on1x_eval(state, false_branch, std::strlen(false_branch), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 2);
    CHECK(on1x_pop(state, 1) == 1);
    const char* no_else = "if false { 1 }";
    CHECK(on1x_eval(state, no_else, std::strlen(no_else), "test") == ON1X_OK);
    CHECK(on1x_type(state, -1) == ON1X_UNIT);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "if 1 { 2 }", 10, "test") == ON1X_ERR);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    const char* arithmetic_chunk = "1 + 2 * 3 - 8 / 2 + 5 % 2";
    CHECK(on1x_eval(state, arithmetic_chunk, std::strlen(arithmetic_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 4);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "-2.5 + 1.0", 10, "test") == ON1X_OK);
    CHECK(std::fabs(on1x_as_float(state, -1) + 1.5) < 1e-12);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "\"on\" + \"1x\"", 11, "test") == ON1X_OK);
    CHECK(std::strcmp(on1x_as_string(state, -1, &length), "on1x") == 0 && length == 4);
    CHECK(on1x_pop(state, 1) == 1);
    const char* logical_chunk = "1 == 1.0 and 2 < 3 and not false";
    CHECK(on1x_eval(state, logical_chunk, std::strlen(logical_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);
    const char* short_circuit_chunk = "false and missing\ntrue or missing";
    CHECK(on1x_eval(state, short_circuit_chunk, std::strlen(short_circuit_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "true and 1", 10, "test") == ON1X_ERR);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "1 / 0", 5, "test") == ON1X_ERR);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    const char* captured_success_chunk = "~ 42\n~";
    CHECK(on1x_eval(
        state, captured_success_chunk, std::strlen(captured_success_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_success(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 5) == 1);
    const char* captured_failure_chunk = "~ 1 / 0\n~";
    CHECK(on1x_eval(
        state, captured_failure_chunk, std::strlen(captured_failure_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_pop(state, 3) == 1);
    const char* expired_effect_chunk = "~ 42\n1 / 0";
    CHECK(on1x_eval(
        state, expired_effect_chunk, std::strlen(expired_effect_chunk), "test") == ON1X_ERR);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    const char* nested_effect_chunk = "~ 1\n~ 2\n~";
    CHECK(on1x_eval(
        state, nested_effect_chunk, std::strlen(nested_effect_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_success(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 2);
    CHECK(on1x_pop(state, 5) == 1);
    const char* captured_field_failure_chunk = "~ %{ :answer => 42 }.missing\n~";
    CHECK(on1x_eval(
        state,
        captured_field_failure_chunk,
        std::strlen(captured_field_failure_chunk),
        "test") == ON1X_OK);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    const char* captured_match_failure_chunk = "~ match 1 { 2 => 0 }\n~";
    CHECK(on1x_eval(
        state,
        captured_match_failure_chunk,
        std::strlen(captured_match_failure_chunk),
        "test") == ON1X_OK);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "\"x\" + 1", 7, "test") == ON1X_ERR);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "?42", 3, "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 3) == 1);
    CHECK(on1x_eval(state, "?", 1, "test") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    const char* list_index_chunk = "[10, 20][1]";
    CHECK(on1x_eval(state, list_index_chunk, std::strlen(list_index_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 20);
    CHECK(on1x_pop(state, 3) == 1);
    CHECK(on1x_eval(state, "[10][2]", 7, "test") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    const char* table_index_chunk = "%{ :answer => 42 }[:answer]";
    CHECK(on1x_eval(state, table_index_chunk, std::strlen(table_index_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 3) == 1);
    const char* field_chunk = "%{ :answer => 42 }.answer";
    CHECK(on1x_eval(state, field_chunk, std::strlen(field_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "\"A\"[0]", 6, "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 65);
    CHECK(on1x_pop(state, 3) == 1);
    const char* missing_field_chunk = "%{ :answer => 42 }.missing";
    CHECK(on1x_eval(state, missing_field_chunk, std::strlen(missing_field_chunk), "test") == ON1X_ERR);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    const char* list_assignment_chunk =
        "let values = [1, 2]\n"
        "values[1] = 42\n"
        "values[1]";
    CHECK(on1x_eval(state, list_assignment_chunk, std::strlen(list_assignment_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 3) == 1);
    const char* field_assignment_chunk =
        "let object = %{}\n"
        "object.answer = 42\n"
        "object.answer";
    CHECK(on1x_eval(state, field_assignment_chunk, std::strlen(field_assignment_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 1) == 1);
    const char* out_of_range_assignment_chunk = "let values = [1]\nvalues[2] = 3";
    CHECK(on1x_eval(
        state, out_of_range_assignment_chunk, std::strlen(out_of_range_assignment_chunk), "test") == ON1X_ERR);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    const char* invalid_key_assignment_chunk = "let object = %{}\nobject[[1]] = 2";
    CHECK(on1x_eval(
        state, invalid_key_assignment_chunk, std::strlen(invalid_key_assignment_chunk), "test") == ON1X_ERR);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    const char* match_literals_chunk =
        "match :Some[42] { :None => 0\n"
        "                  :Some[value] => value\n"
        "                  _ => 1 }";
    CHECK(on1x_eval(state, match_literals_chunk, std::strlen(match_literals_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 1) == 1);
    const char* match_tail_chunk =
        "match [1, 2, 3] { [head, ..tail] => head + 40\n"
        "                   _ => 0 }";
    CHECK(on1x_eval(state, match_tail_chunk, std::strlen(match_tail_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 41);
    CHECK(on1x_pop(state, 1) == 1);
    const char* match_tail_value_chunk = "match [1, 2, 3] { [head, ..tail] => tail }";
    CHECK(on1x_eval(state, match_tail_value_chunk, std::strlen(match_tail_value_chunk), "test") == ON1X_OK);
    CHECK(on1x_type(state, -1) == ON1X_LIST && on1x_len(state, -1) == 2);
    CHECK(on1x_pop(state, 1) == 1);
    const char* match_order_chunk = "match 1 { value => 40 + 2\n_ => 0 }";
    CHECK(on1x_eval(state, match_order_chunk, std::strlen(match_order_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 1) == 1);
    const char* match_failure_chunk = "match 1 { 2 => 0 }";
    CHECK(on1x_eval(state, match_failure_chunk, std::strlen(match_failure_chunk), "test") == ON1X_ERR);
    CHECK(on1x_is_error(state, -1));
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

    CHECK(on1x_eval(state, "TypeOf(42) == :Int", 18, "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "TagOf(:Point[1])", 16, "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_pop(state, 3) == 1);
    CHECK(on1x_eval(state, "PayloadOf(:Point[1, 2])", 23, "test") == ON1X_OK);
    CHECK(on1x_type(state, -1) == ON1X_LIST && on1x_len(state, -1) == 2);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "Len(\"abc\")", 10, "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 3);
    CHECK(on1x_pop(state, 1) == 1);
    const char* prelude_mutation_chunk =
        "let values = [10]\n"
        "Push(values, 20)\n"
        "Set(values, 0, 42)\n"
        "Pop(values)";
    CHECK(on1x_eval(
        state, prelude_mutation_chunk, std::strlen(prelude_mutation_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 20);
    CHECK(on1x_pop(state, 3) == 1);
    CHECK(on1x_eval(state, "Pop([])", 7, "test") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "Get([10], 0)", 12, "test") == ON1X_OK);
    CHECK(on1x_is_some(state, -1));
    CHECK(on1x_payload_of(state, -1) == 1 && on1x_list_get(state, -1, 0) == 1);
    CHECK(on1x_as_int(state, -1) == 10);
    CHECK(on1x_pop(state, 3) == 1);
    CHECK(on1x_eval(state, "Get([10], 1)", 12, "test") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    const char* table_prelude_chunk =
        "let table = %{ :one => 1, :two => 2 }\n"
        "Set(table, :three, 3)\n"
        "Len(Keys(table)) + Len(Values(table))";
    CHECK(on1x_eval(
        state, table_prelude_chunk, std::strlen(table_prelude_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 6);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "Iota", 4, "test") == ON1X_OK);
    CHECK(on1x_type(state, -1) == ON1X_IOTA);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "Iota(4)", 7, "test") == ON1X_OK);
    CHECK(on1x_type(state, -1) == ON1X_LIST && on1x_len(state, -1) == 4);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "Iota(0)", 7, "test") == ON1X_OK);
    CHECK(on1x_type(state, -1) == ON1X_LIST && on1x_len(state, -1) == 0);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "Iota(6, 2, -1)", 14, "test") == ON1X_OK);
    CHECK(on1x_type(state, -1) == ON1X_LIST && on1x_len(state, -1) == 4);
    CHECK(on1x_list_get(state, -1, 0) == 1 && on1x_as_int(state, -1) == 6);
    CHECK(on1x_pop(state, 2) == 1);
    CHECK(on1x_eval(state, "Iota(2.5)", 9, "test") == ON1X_OK);
    CHECK(on1x_is_none(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "IsSome(?42) and IsNone(?)", 25, "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "Unwrap(?42)", 11, "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "UnwrapOr(?, 42)", 15, "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 1) == 1);
    const char* result_prelude_chunk =
        "~ 1\n"
        "IsSuccess(~)";
    CHECK(on1x_eval(
        state, result_prelude_chunk, std::strlen(result_prelude_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);
    const char* error_prelude_chunk =
        "~ 1 / 0\n"
        "IsError(~)";
    CHECK(on1x_eval(
        state, error_prelude_chunk, std::strlen(error_prelude_chunk), "test") == ON1X_OK);
    CHECK(on1x_as_bool(state, -1) == 1);
    CHECK(on1x_pop(state, 1) == 1);
    const char* prelude_failure_chunk =
        "~ Get(%{}, [])\n"
        "~";
    CHECK(on1x_eval(
        state, prelude_failure_chunk, std::strlen(prelude_failure_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    const char* unwrap_failure_chunk =
        "~ Unwrap(?)\n"
        "~";
    CHECK(on1x_eval(
        state, unwrap_failure_chunk, std::strlen(unwrap_failure_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
    const char* arity_failure_chunk =
        "~ Len()\n"
        "~";
    CHECK(on1x_eval(
        state, arity_failure_chunk, std::strlen(arity_failure_chunk), "test") == ON1X_OK);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);

    CHECK(on1x_register(state, "Add", add_ints) == ON1X_OK);
    CHECK(on1x_eval(state, "Add(20, 22)", 11, "test") == ON1X_OK);
    CHECK(on1x_as_int(state, -1) == 42);
    CHECK(on1x_pop(state, 1) == 1);
    CHECK(on1x_eval(state, "Add", 3, "test") == ON1X_OK);
    on1x_push_int(state, 2);
    on1x_push_int(state, 3);
    CHECK(on1x_call(state, -3, 2) == ON1X_OK);
    CHECK(on1x_top(state) == 1 && on1x_as_int(state, -1) == 5);
    CHECK(on1x_pop(state, 1) == 1);

    CHECK(on1x_register(state, "Fail", fail_native) == ON1X_OK);
    const char* captured_native_failure_chunk = "~ Fail()\n~";
    CHECK(on1x_eval(
        state,
        captured_native_failure_chunk,
        std::strlen(captured_native_failure_chunk),
        "test") == ON1X_OK);
    CHECK(on1x_is_error(state, -1));
    CHECK(on1x_pop(state, 1) == 1);
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
