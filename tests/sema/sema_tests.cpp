#include "sema/resolver.hpp"
#include "syntax/parser.hpp"
#include "util/arena.hpp"

#include <cstdio>

namespace {

int failures = 0;

#define CHECK(expression)                                                     \
    do {                                                                      \
        if (!(expression)) {                                                  \
            std::fprintf(stderr, "CHECK failed at %s:%d: %s\n",              \
                         __FILE__, __LINE__, #expression);                    \
            ++failures;                                                       \
        }                                                                     \
    } while (false)

on1x::syntax::AstNode* parse(
    std::string_view source,
    on1x::Arena& arena,
    on1x::syntax::Diagnostics& diagnostics) {
    on1x::syntax::Parser parser(source, arena, diagnostics);
    return parser.parse_program();
}

void test_global_resolution_and_assignment() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    auto* program = parse("let x = 1\nx = x + 2\nx", arena, diagnostics);
    CHECK(program != nullptr && diagnostics.empty());
    on1x::sema::Resolver resolver(diagnostics);
    CHECK(resolver.resolve(program));
    CHECK(diagnostics.empty());
    CHECK(program->first->resolution == on1x::syntax::ResolutionKind::Global);
    auto* assignment = program->first->sibling;
    CHECK(assignment->first->resolution == on1x::syntax::ResolutionKind::Global);
    CHECK(assignment->first->binding_index == program->first->binding_index);
}

void test_undefined_assignment() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    auto* program = parse("missing = 1", arena, diagnostics);
    CHECK(program != nullptr && diagnostics.empty());
    on1x::sema::Resolver resolver(diagnostics);
    CHECK(!resolver.resolve(program));
    CHECK(!diagnostics.empty());
    CHECK(diagnostics.entries().front().message ==
          "assignment to undefined binding 'missing'");
}

void test_effect_scope() {
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        auto* program = parse("~ work(); consume(~)", arena, diagnostics);
        CHECK(program != nullptr && diagnostics.empty());
        on1x::sema::Resolver resolver(diagnostics);
        CHECK(resolver.resolve(program));
        CHECK(diagnostics.empty());
        auto* effect_result = program->first->first->next->first->next;
        CHECK(effect_result->kind == on1x::syntax::AstKind::EffectResult);
        CHECK(effect_result->lexical_depth == 0);
    }
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        auto* program = parse("consume(~)", arena, diagnostics);
        CHECK(program != nullptr && diagnostics.empty());
        on1x::sema::Resolver resolver(diagnostics);
        CHECK(!resolver.resolve(program));
        CHECK(!diagnostics.empty());
    }
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        auto* program = parse("~ 1\n~ 2\n~", arena, diagnostics);
        CHECK(program != nullptr && diagnostics.empty());
        on1x::sema::Resolver resolver(diagnostics);
        CHECK(resolver.resolve(program));
        CHECK(diagnostics.empty());
        auto* inner_effect = program->first->first->next;
        auto* inner_result = inner_effect->first->next;
        CHECK(inner_result->kind == on1x::syntax::AstKind::EffectResult);
        CHECK(inner_result->lexical_depth == 0);
    }
}

void test_block_shadowing() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    auto* program = parse(
        "let value = 1\n"
        "{ let value = 2\n"
        "  value }\n"
        "value",
        arena,
        diagnostics);
    CHECK(program != nullptr && diagnostics.empty());
    on1x::sema::Resolver resolver(diagnostics);
    CHECK(resolver.resolve(program));
    CHECK(diagnostics.empty());
    auto* block = program->first->sibling;
    CHECK(block->kind == on1x::syntax::AstKind::Block);
    CHECK(block->first->resolution == on1x::syntax::ResolutionKind::Local);
    CHECK(block->first->sibling->resolution == on1x::syntax::ResolutionKind::Local);
    CHECK(program->first->sibling->sibling->resolution == on1x::syntax::ResolutionKind::Global);
}

void test_function_resolution_and_return() {
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        auto* program = parse("fn identity(value) { return value }", arena, diagnostics);
        CHECK(program != nullptr && diagnostics.empty());
        on1x::sema::Resolver resolver(diagnostics);
        CHECK(resolver.resolve(program));
        CHECK(diagnostics.empty());
        auto* parameter = program->first->first;
        auto* result = parameter->next->first->first;
        CHECK(parameter->resolution == on1x::syntax::ResolutionKind::Local);
        CHECK(result->resolution == on1x::syntax::ResolutionKind::Local);
    }
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        auto* program = parse("return 1", arena, diagnostics);
        CHECK(program != nullptr && diagnostics.empty());
        on1x::sema::Resolver resolver(diagnostics);
        CHECK(!resolver.resolve(program));
        CHECK(!diagnostics.empty());
        CHECK(diagnostics.entries().front().message == "return is only valid inside a function");
    }
}

void test_loop_resolution() {
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        auto* program = parse("for item in [1] { item }", arena, diagnostics);
        CHECK(program != nullptr && diagnostics.empty());
        on1x::sema::Resolver resolver(diagnostics);
        CHECK(resolver.resolve(program));
        CHECK(diagnostics.empty());
        auto* body_identifier = program->first->first->next->first;
        CHECK(body_identifier->resolution == on1x::syntax::ResolutionKind::Local);
    }
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        auto* program = parse("break\ncontinue", arena, diagnostics);
        CHECK(program != nullptr && diagnostics.empty());
        on1x::sema::Resolver resolver(diagnostics);
        CHECK(!resolver.resolve(program));
        CHECK(!diagnostics.empty());
        CHECK(diagnostics.entries().front().message == "break is only valid inside a loop");
    }
}

void test_enum_resolution() {
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        auto* program = parse("enum { Red = Iota, Green = Iota + 1 }", arena, diagnostics);
        CHECK(program != nullptr && diagnostics.empty());
        on1x::sema::Resolver resolver(diagnostics);
        CHECK(resolver.resolve(program));
        CHECK(diagnostics.empty());
        CHECK(program->first->first->first->kind == on1x::syntax::AstKind::EnumIota);
        CHECK(program->first->first->next->first->first->kind == on1x::syntax::AstKind::EnumIota);
    }
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        auto* program = parse("enum { Red = 0, Red = 1 }", arena, diagnostics);
        CHECK(program != nullptr && diagnostics.empty());
        on1x::sema::Resolver resolver(diagnostics);
        CHECK(!resolver.resolve(program));
        CHECK(!diagnostics.empty());
        CHECK(diagnostics.entries().front().message == "duplicate enum member 'Red'");
    }
}

void test_match_resolution() {
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        auto* program = parse("match :Pair[1, 2] { :Pair[left, right] => left + right }", arena, diagnostics);
        CHECK(program != nullptr && diagnostics.empty());
        on1x::sema::Resolver resolver(diagnostics);
        CHECK(resolver.resolve(program));
        CHECK(diagnostics.empty());
        auto* arm = program->first->first->next;
        auto* left = arm->first->first;
        auto* body_left = arm->first->next->first;
        CHECK(left->resolution == on1x::syntax::ResolutionKind::Local);
        CHECK(body_left->resolution == on1x::syntax::ResolutionKind::Local);
        CHECK(left->binding_index == body_left->binding_index);
    }
    {
        on1x::Arena arena;
        on1x::syntax::Diagnostics diagnostics;
        auto* program = parse("match [1, 2] { [value, value] => value }", arena, diagnostics);
        CHECK(program != nullptr && diagnostics.empty());
        on1x::sema::Resolver resolver(diagnostics);
        CHECK(!resolver.resolve(program));
        CHECK(!diagnostics.empty());
        CHECK(diagnostics.entries().front().message == "pattern binds 'value' more than once");
    }
}

}

int main() {
    test_global_resolution_and_assignment();
    test_undefined_assignment();
    test_effect_scope();
    test_block_shadowing();
    test_function_resolution_and_return();
    test_loop_resolution();
    test_enum_resolution();
    test_match_resolution();
    return failures == 0 ? 0 : 1;
}
