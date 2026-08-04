#include "syntax/ast_builder.hpp"

namespace on1x::syntax {

AstNode* AstBuilder::make(AstKind kind, std::size_t offset, std::string_view text) {
    return arena_.make<AstNode>(AstNode{kind, source_map_.position(offset), text});
}

void AstBuilder::append_child(AstNode*& head, AstNode* child) noexcept {
    if (!head) {
        head = child;
        return;
    }
    AstNode* tail = head;
    while (tail->next) tail = tail->next;
    tail->next = child;
}

}  // namespace on1x::syntax
