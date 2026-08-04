#pragma once

#include "syntax/ast.hpp"
#include "syntax/diagnostics.hpp"

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace on1x::sema {

class Resolver {
public:
    explicit Resolver(syntax::Diagnostics& diagnostics) : diagnostics_(diagnostics) {}

    [[nodiscard]] bool resolve(syntax::AstNode* program);

private:
    struct Binding {
        std::string_view name;
        std::uint32_t index;
        std::uint32_t scope_depth;
        std::uint32_t function_index;
    };

    [[nodiscard]] bool resolve_statement(syntax::AstNode* node);
    [[nodiscard]] bool resolve_expression(syntax::AstNode* node);
    [[nodiscard]] bool resolve_block(syntax::AstNode* block);
    [[nodiscard]] bool resolve_function(syntax::AstNode* node);
    [[nodiscard]] const Binding* find_binding(std::string_view name) const noexcept;
    void annotate_reference(syntax::AstNode* node, bool assignment);
    [[nodiscard]] std::uint32_t capture(
        syntax::AstNode* function,
        const Binding& binding);

    syntax::Diagnostics& diagnostics_;
    std::vector<Binding> bindings_;
    std::uint32_t next_binding_ = 0;
    std::uint32_t scope_depth_ = 0;
    std::uint32_t current_function_ = 0;
    std::uint32_t next_function_ = 1;
    std::vector<syntax::AstNode*> function_stack_;
    std::vector<std::unique_ptr<syntax::AstNode>> captures_;
};

}  // namespace on1x::sema
