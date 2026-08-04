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

void test_expression_precedence() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    on1x::syntax::Parser parser("1 + 2 * 3 == 7 and not false", arena, diagnostics);
    const auto* program = parser.parse_program();
    CHECK(program != nullptr && diagnostics.empty());
    CHECK(on1x::syntax::print_ast(program) ==
          "(Program (Binary and (Binary == (Binary + (Int 1) (Binary * (Int 2) (Int 3))) "
          "(Int 7)) (Unary not (Bool false))))");
}

void test_compound_and_postfix_expressions() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    on1x::syntax::Parser parser(
        "[:Point[1, 2], %{ :name => \"on1x\" }]; service.lookup(1, ?)[0].value",
        arena,
        diagnostics);
    const auto* program = parser.parse_program();
    CHECK(program != nullptr && diagnostics.empty());
    CHECK(program->first != nullptr && program->first->kind == on1x::syntax::AstKind::List);
    CHECK(program->first->first != nullptr &&
          program->first->first->kind == on1x::syntax::AstKind::TaggedList);
    CHECK(program->first->first->next != nullptr &&
          program->first->first->next->kind == on1x::syntax::AstKind::Table);
    CHECK(program->first->sibling != nullptr &&
          program->first->sibling->kind == on1x::syntax::AstKind::Field);
}

void test_comments_newlines_and_diagnostics() {
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        on1x::syntax::Parser parser(
            "1 + // continued after binary operator\n"
            "2\n"
            "/* separator\ncomment */ 3",
            arena,
            diagnostics);
        const auto* program = parser.parse_program();
        CHECK(program != nullptr && diagnostics.empty());
        CHECK(program->first != nullptr && program->first->sibling != nullptr);
        CHECK(program->first->sibling->sibling == nullptr);
    }
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        on1x::syntax::Parser parser("[1, 2", arena, diagnostics);
        CHECK(parser.parse_program() == nullptr);
        CHECK(!diagnostics.empty());
        CHECK(diagnostics.entries().front().position.line == 1);
        CHECK(diagnostics.entries().front().message == "expected ',' or ']' in List literal");
    }
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        on1x::syntax::Parser parser("1 .. 2 .. 3", arena, diagnostics);
        CHECK(parser.parse_program() == nullptr);
        CHECK(!diagnostics.empty());
        CHECK(diagnostics.entries().front().message == "range operator '..' is non-associative");
    }
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        on1x::syntax::Parser parser("~ risky_call(); use(~)", arena, diagnostics);
        const auto* program = parser.parse_program();
        CHECK(program != nullptr && diagnostics.empty());
        CHECK(program->first != nullptr &&
              program->first->kind == on1x::syntax::AstKind::Effect);
        CHECK(program->first->first != nullptr &&
              program->first->first->next != nullptr &&
              program->first->first->next->kind == on1x::syntax::AstKind::Call);
        CHECK(program->first->sibling == nullptr);
    }
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        on1x::syntax::Parser parser("let answer = 40 + 2\nanswer = answer + 1", arena, diagnostics);
        const auto* program = parser.parse_program();
        CHECK(program != nullptr && diagnostics.empty());
        CHECK(program->first != nullptr && program->first->kind == on1x::syntax::AstKind::Let);
        CHECK(program->first->sibling != nullptr &&
              program->first->sibling->kind == on1x::syntax::AstKind::Assign);
    }
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        on1x::syntax::Parser parser("1 2", arena, diagnostics);
        CHECK(parser.parse_program() == nullptr);
        CHECK(!diagnostics.empty());
        CHECK(diagnostics.entries().front().message == "expected statement terminator");
    }
}

void test_blocks_and_if_expressions() {
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        on1x::syntax::Parser parser(
            "if true {\n"
            "  let answer = 40\n"
            "  answer + 2\n"
            "} else {\n"
            "  0\n"
            "}",
            arena,
            diagnostics);
        const auto* program = parser.parse_program();
        CHECK(program != nullptr && diagnostics.empty());
        CHECK(program->first != nullptr && program->first->kind == on1x::syntax::AstKind::If);
        CHECK(program->first->first->next != nullptr &&
              program->first->first->next->kind == on1x::syntax::AstKind::Block);
        CHECK(program->first->first->next->first != nullptr &&
              program->first->first->next->first->sibling != nullptr);
        CHECK(on1x::syntax::print_ast(program).find("(If if") != std::string::npos);
    }
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        on1x::syntax::Parser parser("if true { 1", arena, diagnostics);
        CHECK(parser.parse_program() == nullptr);
        CHECK(!diagnostics.empty());
        CHECK(diagnostics.entries().front().message == "expected '}' to close block");
    }
}

void test_function_syntax() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    on1x::syntax::Parser parser(
        "fn add(a, b) { return a + b }\n"
        "let collect = fn(first, rest..) { rest }",
        arena,
        diagnostics);
    const auto* program = parser.parse_program();
    CHECK(program != nullptr && diagnostics.empty());
    CHECK(program->first != nullptr && program->first->kind == on1x::syntax::AstKind::Function);
    CHECK(program->first->first != nullptr &&
          program->first->first->kind == on1x::syntax::AstKind::Parameter);
    CHECK(program->first->first->next->next != nullptr &&
          program->first->first->next->next->kind == on1x::syntax::AstKind::Block);
    CHECK(program->first->sibling != nullptr &&
          program->first->sibling->kind == on1x::syntax::AstKind::Let);
    CHECK(program->first->sibling->first->first->next->variadic);
}

}  // namespace

int main() {
    test_integers();
    test_floats_and_strings();
    test_diagnostics();
    test_parser_surface();
    test_expression_precedence();
    test_compound_and_postfix_expressions();
    test_comments_newlines_and_diagnostics();
    test_blocks_and_if_expressions();
    test_function_syntax();
    return failures == 0 ? 0 : 1;
}
