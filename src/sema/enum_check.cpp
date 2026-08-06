#include "sema/enum_check.hpp"

#include <string_view>
#include <vector>

namespace on1x::sema {

namespace {

bool mark_iota(syntax::AstNode* node, bool callee) {
    if (!node) return true;
    if (node->kind == syntax::AstKind::Identifier && node->text == "Iota" && !callee) {
        node->kind = syntax::AstKind::EnumIota;
        return true;
    }
    if (node->kind == syntax::AstKind::Call) {
        bool valid = true;
        bool first = true;
        for (syntax::AstNode* child = node->first; child; child = child->next) {
            valid = mark_iota(child, first) && valid;
            first = false;
        }
        return valid;
    }
    bool valid = true;
    for (syntax::AstNode* child = node->first; child; child = child->next) {
        valid = mark_iota(child, false) && valid;
    }
    return valid;
}

}  // namespace

bool check_enum(syntax::AstNode* enumeration, syntax::Diagnostics& diagnostics) {
    if (!enumeration || enumeration->kind != syntax::AstKind::Enum) return false;
    std::vector<std::string_view> members;
    bool valid = true;
    for (syntax::AstNode* member = enumeration->first; member; member = member->next) {
        if (member->kind != syntax::AstKind::EnumMember || member->text.empty() || !member->first) {
            diagnostics.add(member ? member->position : enumeration->position, "invalid enum member");
            valid = false;
            continue;
        }
        for (const std::string_view existing : members) {
            if (existing == member->text) {
                diagnostics.add(member->position, "duplicate enum member '" + std::string(member->text) + "'");
                valid = false;
                break;
            }
        }
        members.push_back(member->text);
        valid = mark_iota(member->first, false) && valid;
    }
    return valid;
}

}  // namespace on1x::sema
