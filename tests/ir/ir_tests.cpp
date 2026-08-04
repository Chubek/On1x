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

}

int main() {
    test_lowering_and_desugaring();
    test_verifier_rejects_undefined_register();
    test_if_lowering_has_control_flow();
    test_function_lowering();
    return failures == 0 ? 0 : 1;
}
