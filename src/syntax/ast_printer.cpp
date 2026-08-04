#include "syntax/ast.hpp"

#include <string>

namespace on1x::syntax {

namespace {

std::string_view kind_name(AstKind kind) noexcept {
    switch (kind) {
    case AstKind::Unit: return "Unit";
    case AstKind::Bool: return "Bool";
    case AstKind::Int: return "Int";
    case AstKind::Float: return "Float";
    case AstKind::String: return "String";
    case AstKind::Tag: return "Tag";
    case AstKind::Identifier: return "Identifier";
    case AstKind::List: return "List";
    case AstKind::Table: return "Table";
    case AstKind::TableEntry: return "TableEntry";
    case AstKind::TaggedList: return "TaggedList";
    case AstKind::Unary: return "Unary";
    case AstKind::Binary: return "Binary";
    case AstKind::Call: return "Call";
    case AstKind::Index: return "Index";
    case AstKind::Field: return "Field";
    case AstKind::Optional: return "Optional";
    case AstKind::EffectResult: return "EffectResult";
    case AstKind::Let: return "Let";
    case AstKind::Assign: return "Assign";
    case AstKind::Effect: return "Effect";
    case AstKind::Block: return "Block";
    case AstKind::If: return "If";
    case AstKind::Function: return "Function";
    case AstKind::Parameter: return "Parameter";
    case AstKind::Return: return "Return";
    case AstKind::Program: return "Program";
    }
    return "Unknown";
}

void append_node(std::string& output, const AstNode* node) {
    if (!node) {
        output += "()";
        return;
    }
    output.push_back('(');
    output += kind_name(node->kind);
    if (!node->text.empty()) {
        output.push_back(' ');
        output.append(node->text);
    }
    if (node->kind == AstKind::Block) {
        for (const AstNode* statement = node->first; statement; statement = statement->sibling) {
            output.push_back(' ');
            append_node(output, statement);
        }
    } else {
        for (const AstNode* child = node->first; child; child = child->next) {
            output.push_back(' ');
            append_node(output, child);
        }
    }
    output.push_back(')');
}

}

std::string print_ast(const AstNode* node) {
    std::string output;
    if (node && node->kind == AstKind::Program) {
        output = "(Program";
        for (const AstNode* statement = node->first; statement; statement = statement->sibling) {
            output.push_back(' ');
            append_node(output, statement);
        }
        output.push_back(')');
        return output;
    }
    append_node(output, node);
    return output;
}

}  // namespace on1x::syntax
