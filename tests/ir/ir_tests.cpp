#include "ir/lower_ast.hpp"
#include "ir/verifier.hpp"
#include "sema/resolver.hpp"
#include "syntax/parser.hpp"
#include "util/arena.hpp"

#include <cstdio>
#include <string>

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

void test_lowering_and_desugaring() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    on1x::syntax::Parser parser(
        "let range = 1 .. 4\nrange.value",
        arena,
        diagnostics);
    auto* program = parser.parse_program();
    CHECK(program != nullptr && diagnostics.empty());
    on1x::sema::Resolver resolver(diagnostics);
    CHECK(resolver.resolve(program));
    on1x::ir::Module module;
    CHECK(on1x::ir::lower_ast(program, module, diagnostics));
    CHECK(on1x::ir::verify(module, diagnostics));
    CHECK(diagnostics.empty());
    const std::string dump = on1x::ir::print(module);
    CHECK(dump.find("call Iota") != std::string::npos);
    CHECK(dump.find("store_global range") != std::string::npos);
    CHECK(dump.find("discard") != std::string::npos);
    CHECK(dump.find("field value") != std::string::npos);
}

void test_verifier_rejects_undefined_register() {
    on1x::ir::Module module;
    on1x::ir::Instruction instruction;
    instruction.opcode = on1x::ir::Opcode::Return;
    instruction.operands.push_back(9);
    module.entry.blocks.front().instructions.push_back(std::move(instruction));
    on1x::syntax::Diagnostics diagnostics;
    CHECK(!on1x::ir::verify(module, diagnostics));
    CHECK(!diagnostics.empty());
}

void test_if_lowering_has_control_flow() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    on1x::syntax::Parser parser("if true { 1 } else { 2 }", arena, diagnostics);
    auto* program = parser.parse_program();
    CHECK(program != nullptr && diagnostics.empty());
    on1x::sema::Resolver resolver(diagnostics);
    CHECK(resolver.resolve(program));
    on1x::ir::Module module;
    CHECK(on1x::ir::lower_ast(program, module, diagnostics));
    CHECK(on1x::ir::verify(module, diagnostics));
    CHECK(diagnostics.empty());
    const std::string dump = on1x::ir::print(module);
    CHECK(dump.find("branch_if_false block") != std::string::npos);
    CHECK(dump.find("jump block") != std::string::npos);
    CHECK(module.entry.blocks.size() == 4);
}

void test_function_lowering() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    on1x::syntax::Parser parser("fn add(a, b) { return a + b }\nadd(2, 3)", arena, diagnostics);
    auto* program = parser.parse_program();
    CHECK(program != nullptr && diagnostics.empty());
    on1x::sema::Resolver resolver(diagnostics);
    CHECK(resolver.resolve(program));
    on1x::ir::Module module;
    CHECK(on1x::ir::lower_ast(program, module, diagnostics));
    CHECK(on1x::ir::verify(module, diagnostics));
    CHECK(diagnostics.empty());
    CHECK(module.functions.size() == 1);
    const std::string dump = on1x::ir::print(module);
    CHECK(dump.find("make_function function 1") != std::string::npos);
    CHECK(dump.find("call") != std::string::npos);
}

void test_loop_lowering() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    on1x::syntax::Parser parser("for item in [1, 2] { if true { continue } else { break } }", arena, diagnostics);
    auto* program = parser.parse_program();
    CHECK(program != nullptr && diagnostics.empty());
    on1x::sema::Resolver resolver(diagnostics);
    CHECK(resolver.resolve(program));
    on1x::ir::Module module;
    CHECK(on1x::ir::lower_ast(program, module, diagnostics));
    CHECK(on1x::ir::verify(module, diagnostics));
    CHECK(diagnostics.empty());
    const std::string dump = on1x::ir::print(module);
    CHECK(dump.find("iter_init") != std::string::npos);
    CHECK(dump.find("iter_next") != std::string::npos);
    CHECK(dump.find("iter_close") != std::string::npos);
}

void test_enum_lowering() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    on1x::syntax::Parser parser("enum { Red = Iota, Green = Iota, Blue = 7 }", arena, diagnostics);
    auto* program = parser.parse_program();
    CHECK(program != nullptr && diagnostics.empty());
    on1x::sema::Resolver resolver(diagnostics);
    CHECK(resolver.resolve(program));
    on1x::ir::Module module;
    CHECK(on1x::ir::lower_ast(program, module, diagnostics));
    CHECK(on1x::ir::verify(module, diagnostics));
    CHECK(diagnostics.empty());
    const std::string dump = on1x::ir::print(module);
    CHECK(dump.find("make_table") != std::string::npos);
    CHECK(dump.find("tag Red") != std::string::npos);
}

void test_match_lowering() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    on1x::syntax::Parser parser(
        "match :Some[42] { :None => 0\n"
        "                  :Some[value] => value\n"
        "                  _ => 1 }",
        arena,
        diagnostics);
    auto* program = parser.parse_program();
    CHECK(program != nullptr && diagnostics.empty());
    on1x::sema::Resolver resolver(diagnostics);
    CHECK(resolver.resolve(program));
    on1x::ir::Module module;
    CHECK(on1x::ir::lower_ast(program, module, diagnostics));
    CHECK(on1x::ir::verify(module, diagnostics));
    CHECK(diagnostics.empty());
    const std::string dump = on1x::ir::print(module);
    CHECK(dump.find("match_pattern") != std::string::npos);
    CHECK(dump.find("match_failure") != std::string::npos);
}

void test_logical_lowering() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    on1x::syntax::Parser parser("true and false or true", arena, diagnostics);
    auto* program = parser.parse_program();
    CHECK(program != nullptr && diagnostics.empty());
    on1x::sema::Resolver resolver(diagnostics);
    CHECK(resolver.resolve(program));
    on1x::ir::Module module;
    CHECK(on1x::ir::lower_ast(program, module, diagnostics));
    CHECK(on1x::ir::verify(module, diagnostics));
    CHECK(diagnostics.empty());
    const std::string dump = on1x::ir::print(module);
    CHECK(dump.find("require_bool") != std::string::npos);
    CHECK(dump.find("branch_if_false block") != std::string::npos);
}

void test_aggregate_assignment_lowering() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    on1x::syntax::Parser parser(
        "let xs = [1]\n"
        "xs[0] = 2\n"
        "let table = %{}\n"
        "table.answer = 42",
        arena,
        diagnostics);
    auto* program = parser.parse_program();
    CHECK(program != nullptr && diagnostics.empty());
    on1x::sema::Resolver resolver(diagnostics);
    CHECK(resolver.resolve(program));
    on1x::ir::Module module;
    CHECK(on1x::ir::lower_ast(program, module, diagnostics));
    CHECK(on1x::ir::verify(module, diagnostics));
    CHECK(diagnostics.empty());
    const std::string dump = on1x::ir::print(module);
    CHECK(dump.find("set_index") != std::string::npos);
    CHECK(dump.find("set_field answer") != std::string::npos);
}

void test_effect_lowering() {
    on1x::Arena arena;
    on1x::syntax::Diagnostics diagnostics;
    on1x::syntax::Parser parser("~ 1 / 0\n~", arena, diagnostics);
    auto* program = parser.parse_program();
    CHECK(program != nullptr && diagnostics.empty());
    on1x::sema::Resolver resolver(diagnostics);
    CHECK(resolver.resolve(program));
    on1x::ir::Module module;
    CHECK(on1x::ir::lower_ast(program, module, diagnostics));
    CHECK(on1x::ir::verify(module, diagnostics));
    CHECK(diagnostics.empty());
    const std::string dump = on1x::ir::print(module);
    CHECK(dump.find("begin_capture") != std::string::npos);
    CHECK(dump.find("end_capture") != std::string::npos);
    CHECK(dump.find("effect_result") != std::string::npos);
    CHECK(dump.find("end_effect_scope") != std::string::npos);
}

}

int main() {
    test_lowering_and_desugaring();
    test_verifier_rejects_undefined_register();
    test_if_lowering_has_control_flow();
    test_function_lowering();
    test_loop_lowering();
    test_enum_lowering();
    test_match_lowering();
    test_logical_lowering();
    test_aggregate_assignment_lowering();
    test_effect_lowering();
    return failures == 0 ? 0 : 1;
}
