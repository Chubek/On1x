#include "syntax/diagnostics.hpp"
#include "syntax/keywords.hpp"
#include "syntax/literals.hpp"
#include "syntax/parser.hpp"
#include "syntax/terminators.hpp"
#include "util/arena.hpp"

#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>

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

void test_integers() {
    std::int64_t value = 0;
    CHECK(on1x::syntax::decode_integer("1_000_000", value) && value == 1000000);
    CHECK(on1x::syntax::decode_integer("0xF_F", value) && value == 255);
    CHECK(on1x::syntax::decode_integer("0b1010", value) && value == 10);
    CHECK(on1x::syntax::decode_integer("0o17", value) && value == 15);
    CHECK(on1x::syntax::decode_integer("-9223372036854775808", value));
    CHECK(value == std::numeric_limits<std::int64_t>::min());
    CHECK(!on1x::syntax::decode_integer("_1", value));
    CHECK(!on1x::syntax::decode_integer("1_", value));
    CHECK(!on1x::syntax::decode_integer("0b102", value));
}

void test_floats_and_strings() {
    double floating = 0.0;
    CHECK(on1x::syntax::decode_float("6.022e2", floating) && floating == 602.2);
    CHECK(!on1x::syntax::decode_float("1__0.0", floating));
    std::string text;
    CHECK(on1x::syntax::decode_string("\"a\\n\\u{1F600}\"", text));
    CHECK(text == "a\n\xF0\x9F\x98\x80");
    CHECK(!on1x::syntax::decode_string("\"\\u{D800}\"", text));
    CHECK(!on1x::syntax::decode_string("\"\\u{110000}\"", text));
    CHECK(!on1x::syntax::decode_string("\"\\q\"", text));
}

void test_diagnostics() {
    on1x::syntax::Diagnostics diagnostics;
    diagnostics.add({3, 2, 4}, "expected expression");
    CHECK(!diagnostics.empty());
    CHECK(diagnostics.entries().front().position.line == 2);
    CHECK(diagnostics.entries().front().message == "expected expression");
}

void test_parser_surface() {
    CHECK(on1x::syntax::is_identifier("valid_name"));
    CHECK(!on1x::syntax::is_identifier("let"));
    CHECK(!on1x::syntax::is_identifier("9bad"));
    CHECK(on1x::syntax::has_valid_terminators("let x = (1 + 2)\n"));
    CHECK(!on1x::syntax::has_valid_terminators("\"unterminated"));

    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    on1x::syntax::Parser parser("1 + 2; :Tag; \"ok\"", arena, diagnostics);
    const auto* program = parser.parse_program();
    CHECK(program != nullptr && diagnostics.empty());
    CHECK(program->first != nullptr && program->first->kind == on1x::syntax::AstKind::Binary);
    CHECK(program->first->sibling != nullptr && program->first->sibling->kind == on1x::syntax::AstKind::Tag);
    CHECK(program->first->sibling->sibling != nullptr);
}

}  // namespace

int main() {
    test_integers();
    test_floats_and_strings();
    test_diagnostics();
    test_parser_surface();
    return failures == 0 ? 0 : 1;
}
