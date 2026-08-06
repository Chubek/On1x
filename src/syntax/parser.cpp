#include "syntax/parser.hpp"

#include "syntax/keywords.hpp"
#include "syntax/literals.hpp"
#include "syntax/precedence.hpp"

#include <cctype>
#include <string>

namespace on1x::syntax {

namespace {

bool is_identifier_start(char character) noexcept {
    return character == '_' || (character >= 'a' && character <= 'z') ||
        (character >= 'A' && character <= 'Z');
}

bool is_identifier_continue(char character) noexcept {
    return is_identifier_start(character) || (character >= '0' && character <= '9');
}

}

Parser::Parser(std::string_view source, Arena& arena, Diagnostics& diagnostics)
    : source_(source), arena_(arena), diagnostics_(diagnostics), builder_(source, arena) {}

void Parser::error(std::size_t offset, std::string message) {
    if (!failed_) diagnostics_.add(SourceMap(source_).position(offset), std::move(message));
    failed_ = true;
}

void Parser::skip_trivia(bool allow_newlines) {
    while (offset_ < source_.size()) {
        const char character = source_[offset_];
        if (character == ' ' || character == '\t' || character == '\r') {
            ++offset_;
            continue;
        }
        if (character == '\n') {
            if (!allow_newlines) return;
            ++offset_;
            continue;
        }
        if (source_.substr(offset_, 2) == "//") {
            offset_ += 2;
            while (offset_ < source_.size() && source_[offset_] != '\n') ++offset_;
            if (!allow_newlines) return;
            continue;
        }
        if (source_.substr(offset_, 2) == "/*") {
            const std::size_t begin = offset_;
            offset_ += 2;
            bool saw_newline = false;
            while (offset_ + 1 < source_.size() && source_.substr(offset_, 2) != "*/") {
                saw_newline = saw_newline || source_[offset_] == '\n';
                ++offset_;
            }
            if (offset_ + 1 >= source_.size()) {
                error(begin, "unterminated block comment");
                return;
            }
            offset_ += 2;
            if (saw_newline && !allow_newlines) {
                logical_newline_ = true;
                return;
            }
            continue;
        }
        return;
    }
}

bool Parser::consume(char character, bool allow_newlines) {
    skip_trivia(allow_newlines);
    if (offset_ < source_.size() && source_[offset_] == character) {
        ++offset_;
        return true;
    }
    return false;
}

bool Parser::consume(std::string_view text, bool allow_newlines) {
    skip_trivia(allow_newlines);
    if (source_.substr(offset_, text.size()) != text) return false;
    if (!text.empty() && is_identifier_continue(text.back()) &&
        offset_ + text.size() < source_.size() &&
        is_identifier_continue(source_[offset_ + text.size()])) {
        return false;
    }
    offset_ += text.size();
    return true;
}

bool Parser::at_word(std::string_view word) const noexcept {
    if (source_.substr(offset_, word.size()) != word) return false;
    return offset_ + word.size() == source_.size() ||
        !is_identifier_continue(source_[offset_ + word.size()]);
}

bool Parser::at_statement_end() const noexcept {
    return logical_newline_ || offset_ == source_.size() || source_[offset_] == ';' ||
        source_[offset_] == '\n';
}

bool Parser::is_lvalue(const AstNode* node) noexcept {
    return node && (node->kind == AstKind::Identifier || node->kind == AstKind::Index ||
                    node->kind == AstKind::Field);
}

void Parser::consume_terminators() {
    logical_newline_ = false;
    while (offset_ < source_.size()) {
        skip_trivia(false);
        if (logical_newline_) {
            logical_newline_ = false;
            continue;
        }
        if (offset_ < source_.size() && (source_[offset_] == ';' || source_[offset_] == '\n')) {
            ++offset_;
            continue;
        }
        break;
    }
}

std::string_view Parser::scan_identifier() {
    skip_trivia(nesting_depth_ != 0);
    if (offset_ >= source_.size() || !is_identifier_start(source_[offset_])) return {};
    const std::size_t begin = offset_++;
    while (offset_ < source_.size() && is_identifier_continue(source_[offset_])) ++offset_;
    return source_.substr(begin, offset_ - begin);
}

std::string_view Parser::scan_string() {
    skip_trivia(nesting_depth_ != 0);
    if (offset_ >= source_.size() || source_[offset_] != '"') return {};
    const std::size_t begin = offset_++;
    bool escaped = false;
    while (offset_ < source_.size()) {
        const char character = source_[offset_++];
        if (!escaped && character == '"') return source_.substr(begin, offset_ - begin);
        if (!escaped && character == '\n') {
            error(begin, "newline in string literal");
            return {};
        }
        escaped = !escaped && character == '\\';
        if (character != '\\') escaped = false;
    }
    error(begin, "unterminated string literal");
    return {};
}

std::string_view Parser::scan_number() {
    skip_trivia(nesting_depth_ != 0);
    if (offset_ >= source_.size() ||
        !std::isdigit(static_cast<unsigned char>(source_[offset_]))) {
        return {};
    }
    const std::size_t begin = offset_;
    bool seen_dot = false;
    bool seen_exponent = false;
    while (offset_ < source_.size()) {
        const char character = source_[offset_];
        if (std::isalnum(static_cast<unsigned char>(character)) || character == '_') {
            seen_exponent = seen_exponent || character == 'e' || character == 'E';
            ++offset_;
            continue;
        }
        if (character == '.' && !seen_dot && !seen_exponent &&
            source_.substr(offset_, 2) != "..") {
            seen_dot = true;
            ++offset_;
            continue;
        }
        if ((character == '+' || character == '-') && offset_ > begin &&
            (source_[offset_ - 1] == 'e' || source_[offset_ - 1] == 'E')) {
            ++offset_;
            continue;
        }
        break;
    }
    return source_.substr(begin, offset_ - begin);
}

AstNode* Parser::parse_delimited_expression(char closing, std::string_view context) {
    ++nesting_depth_;
    AstNode* expression = parse_expression();
    --nesting_depth_;
    if (!expression) return nullptr;
    if (!consume(closing, true)) {
        error(offset_, "expected '" + std::string(1, closing) + "' after " + std::string(context));
        return nullptr;
    }
    return expression;
}

AstNode* Parser::parse_list(std::size_t begin) {
    AstNode* list = builder_.make(AstKind::List, begin);
    ++nesting_depth_;
    skip_trivia(true);
    if (consume(']', true)) {
        --nesting_depth_;
        return list;
    }
    while (!failed_) {
        AstNode* element = parse_expression();
        if (!element) {
            --nesting_depth_;
            return nullptr;
        }
        AstBuilder::append_child(list->first, element);
        if (consume(']', true)) {
            --nesting_depth_;
            return list;
        }
        if (!consume(',', true)) {
            error(offset_, "expected ',' or ']' in List literal");
            --nesting_depth_;
            return nullptr;
        }
        if (consume(']', true)) {
            --nesting_depth_;
            return list;
        }
    }
    --nesting_depth_;
    return nullptr;
}

AstNode* Parser::parse_table(std::size_t begin) {
    AstNode* table = builder_.make(AstKind::Table, begin);
    ++nesting_depth_;
    skip_trivia(true);
    if (consume('}', true)) {
        --nesting_depth_;
        return table;
    }
    while (!failed_) {
        AstNode* key = parse_expression();
        if (!key) {
            --nesting_depth_;
            return nullptr;
        }
        if (!consume("=>", true)) {
            error(offset_, "expected '=>' after Table key");
            --nesting_depth_;
            return nullptr;
        }
        AstNode* value = parse_expression();
        if (!value) {
            --nesting_depth_;
            return nullptr;
        }
        AstNode* entry = builder_.make(AstKind::TableEntry, key->position.byte_offset);
        entry->first = key;
        key->next = value;
        AstBuilder::append_child(table->first, entry);
        if (consume('}', true)) {
            --nesting_depth_;
            return table;
        }
        if (!consume(',', true)) {
            error(offset_, "expected ',' or '}' in Table literal");
            --nesting_depth_;
            return nullptr;
        }
        if (consume('}', true)) {
            --nesting_depth_;
            return table;
        }
    }
    --nesting_depth_;
    return nullptr;
}

AstNode* Parser::parse_tag(std::size_t begin) {
    std::string_view name;
    if (offset_ < source_.size() && source_[offset_] == '"') {
        name = scan_string();
        std::string decoded;
        if (!failed_ && !decode_string(name, decoded)) {
            error(begin, "invalid quoted Tag");
            return nullptr;
        }
    } else {
        name = scan_identifier();
        if (name.empty() || is_keyword(name)) {
            error(begin, "expected Tag name after ':'");
            return nullptr;
        }
    }
    AstNode* tag = builder_.make(AstKind::Tag, begin, name);
    if (!consume('[', nesting_depth_ != 0)) return tag;
    AstNode* tagged = builder_.make(AstKind::TaggedList, begin, name);
    ++nesting_depth_;
    if (consume(']', true)) {
        --nesting_depth_;
        return tagged;
    }
    while (!failed_) {
        AstNode* payload = parse_expression();
        if (!payload) {
            --nesting_depth_;
            return nullptr;
        }
        AstBuilder::append_child(tagged->first, payload);
        if (consume(']', true)) {
            --nesting_depth_;
            return tagged;
        }
        if (!consume(',', true)) {
            error(offset_, "expected ',' or ']' in Tagged List");
            --nesting_depth_;
            return nullptr;
        }
    }
    --nesting_depth_;
    return nullptr;
}

AstNode* Parser::parse_block(std::size_t begin) {
    AstNode* block = builder_.make(AstKind::Block, begin);
    consume_terminators();
    AstNode** tail = &block->first;
    while (!failed_) {
        skip_trivia(false);
        if (consume('}', false)) {
            return block;
        }
        if (offset_ >= source_.size()) {
            error(begin, "expected '}' to close block");
            return nullptr;
        }
        AstNode* statement = parse_terminated_statement();
        if (!statement) {
            return nullptr;
        }
        *tail = statement;
        tail = &statement->sibling;
    }
    return nullptr;
}

AstNode* Parser::parse_if(std::size_t begin) {
    AstNode* condition = parse_expression();
    if (!condition) return nullptr;
    if (!consume('{')) {
        error(offset_, "expected '{' after if condition");
        return nullptr;
    }
    AstNode* then_block = parse_block(offset_ - 1U);
    if (!then_block) return nullptr;

    AstNode* if_node = builder_.make(AstKind::If, begin, "if");
    if_node->first = condition;
    condition->next = then_block;
    const std::size_t before_else = offset_;
    const bool before_else_logical_newline = logical_newline_;
    if (consume("else", true)) {
        AstNode* else_branch = nullptr;
        const std::size_t else_begin = offset_;
        if (consume("if")) {
            else_branch = parse_if(else_begin);
        } else if (consume('{')) {
            else_branch = parse_block(offset_ - 1U);
        } else {
            error(offset_, "expected 'if' or '{' after 'else'");
            return nullptr;
        }
        then_block->next = else_branch;
    } else {
        offset_ = before_else;
        logical_newline_ = before_else_logical_newline;
    }
    return if_node;
}

AstNode* Parser::parse_function(std::size_t begin) {
    const std::string_view name = scan_identifier();
    if (!name.empty() && is_keyword(name)) {
        error(begin, "keyword cannot be a function name");
        return nullptr;
    }
    if (!consume('(')) {
        error(offset_, "expected '(' after 'fn'");
        return nullptr;
    }
    AstNode* function = builder_.make(AstKind::Function, begin, name);
    AstNode** tail = &function->first;
    ++nesting_depth_;
    if (!consume(')', true)) {
        while (!failed_) {
            const std::string_view parameter_name = scan_identifier();
            if (parameter_name.empty() || is_keyword(parameter_name)) {
                error(offset_, "expected parameter name");
                --nesting_depth_;
                return nullptr;
            }
            AstNode* parameter = builder_.make(AstKind::Parameter, offset_ - parameter_name.size(), parameter_name);
            if (consume("..", true)) parameter->variadic = true;
            *tail = parameter;
            tail = &parameter->next;
            if (parameter->variadic && !consume(')', true)) {
                error(offset_, "rest parameter must be last");
                --nesting_depth_;
                return nullptr;
            }
            if (parameter->variadic || consume(')', true)) break;
            if (!consume(',', true)) {
                error(offset_, "expected ',' or ')' in parameter list");
                --nesting_depth_;
                return nullptr;
            }
        }
    }
    --nesting_depth_;
    if (!consume('{')) {
        error(offset_, "expected '{' before function body");
        return nullptr;
    }
    AstNode* body = parse_block(offset_ - 1U);
    if (!body) return nullptr;
    *tail = body;
    return function;
}

AstNode* Parser::parse_return(std::size_t begin) {
    AstNode* result = builder_.make(AstKind::Return, begin, "return");
    skip_trivia(nesting_depth_ != 0);
    if (!at_statement_end() && source_[offset_] != '}') {
        result->first = parse_expression();
    }
    return result;
}

AstNode* Parser::parse_while(std::size_t begin) {
    AstNode* condition = parse_expression();
    if (!condition) return nullptr;
    if (!consume('{')) {
        error(offset_, "expected '{' after while condition");
        return nullptr;
    }
    AstNode* loop = builder_.make(AstKind::While, begin, "while");
    loop->first = condition;
    condition->next = parse_block(offset_ - 1U);
    return condition->next ? loop : nullptr;
}

AstNode* Parser::parse_for(std::size_t begin) {
    const std::string_view name = scan_identifier();
    if (name.empty() || is_keyword(name)) {
        error(offset_, "expected loop binding after 'for'");
        return nullptr;
    }
    if (!consume("in")) {
        error(offset_, "expected 'in' after loop binding");
        return nullptr;
    }
    AstNode* iterable = parse_expression();
    if (!iterable) return nullptr;
    if (!consume('{')) {
        error(offset_, "expected '{' after for iterable");
        return nullptr;
    }
    AstNode* loop = builder_.make(AstKind::For, begin, name);
    loop->first = iterable;
    iterable->next = parse_block(offset_ - 1U);
    return iterable->next ? loop : nullptr;
}

AstNode* Parser::parse_enum(std::size_t begin) {
    if (!consume('{')) {
        error(offset_, "expected '{' after 'enum'");
        return nullptr;
    }
    AstNode* enumeration = builder_.make(AstKind::Enum, begin, "enum");
    AstNode** tail = &enumeration->first;
    consume_terminators();
    while (!failed_) {
        skip_trivia(false);
        if (consume('}', false)) return enumeration;
        const std::size_t member_offset = offset_;
        const std::string_view name = scan_identifier();
        if (name.empty() || is_keyword(name)) {
            error(offset_, "expected enum member name");
            return nullptr;
        }
        if (!consume('=')) {
            error(offset_, "expected '=' after enum member name");
            return nullptr;
        }
        AstNode* value = parse_expression();
        if (!value) return nullptr;
        AstNode* member = builder_.make(AstKind::EnumMember, member_offset, name);
        member->first = value;
        *tail = member;
        tail = &member->next;

        if (consume(',', false)) {
            consume_terminators();
            continue;
        }
        skip_trivia(false);
        if (consume('}', false)) return enumeration;
        if (at_statement_end()) {
            consume_terminators();
            continue;
        }
        error(offset_, "expected ',' or '}' after enum member");
        return nullptr;
    }
    return nullptr;
}

AstNode* Parser::parse_pattern_list(std::size_t begin) {
    AstNode* pattern = builder_.make(AstKind::PatternList, begin);
    ++nesting_depth_;
    if (consume(']', true)) {
        --nesting_depth_;
        return pattern;
    }
    while (!failed_) {
        if (consume("..", true)) {
            const std::string_view tail = scan_identifier();
            if (tail.empty() || tail == "_" || is_keyword(tail)) {
                error(offset_, "expected tail binding after '..' in List pattern");
                --nesting_depth_;
                return nullptr;
            }
            AstNode* binding = builder_.make(
                AstKind::PatternBinding, offset_ - tail.size(), tail);
            binding->variadic = true;
            AstBuilder::append_child(pattern->first, binding);
            if (!consume(']', true)) {
                error(offset_, "tail binding must be last in List pattern");
                --nesting_depth_;
                return nullptr;
            }
            --nesting_depth_;
            return pattern;
        }
        AstNode* element = parse_pattern();
        if (!element) {
            --nesting_depth_;
            return nullptr;
        }
        AstBuilder::append_child(pattern->first, element);
        if (consume(']', true)) {
            --nesting_depth_;
            return pattern;
        }
        if (!consume(',', true)) {
            error(offset_, "expected ',' or ']' in List pattern");
            --nesting_depth_;
            return nullptr;
        }
    }
    --nesting_depth_;
    return nullptr;
}

AstNode* Parser::parse_pattern_tag(std::size_t begin) {
    std::string_view name;
    if (offset_ < source_.size() && source_[offset_] == '"') {
        name = scan_string();
        std::string decoded;
        if (!failed_ && !decode_string(name, decoded)) {
            error(begin, "invalid quoted Tag pattern");
            return nullptr;
        }
    } else {
        name = scan_identifier();
        if (name.empty() || is_keyword(name)) {
            error(begin, "expected Tag name after ':' in pattern");
            return nullptr;
        }
    }
    AstNode* pattern = builder_.make(AstKind::PatternTaggedList, begin, name);
    if (!consume('[', nesting_depth_ != 0)) return pattern;
    ++nesting_depth_;
    if (consume(']', true)) {
        --nesting_depth_;
        return pattern;
    }
    while (!failed_) {
        AstNode* element = parse_pattern();
        if (!element) {
            --nesting_depth_;
            return nullptr;
        }
        AstBuilder::append_child(pattern->first, element);
        if (consume(']', true)) {
            --nesting_depth_;
            return pattern;
        }
        if (!consume(',', true)) {
            error(offset_, "expected ',' or ']' in Tagged List pattern");
            --nesting_depth_;
            return nullptr;
        }
    }
    --nesting_depth_;
    return nullptr;
}

AstNode* Parser::parse_pattern() {
    skip_trivia(true);
    if (failed_ || offset_ >= source_.size()) {
        if (!failed_) error(offset_, "expected pattern");
        return nullptr;
    }
    const std::size_t begin = offset_;
    if (consume('[')) return parse_pattern_list(begin);
    if (consume(':')) return parse_pattern_tag(begin);
    if (consume('(')) {
        if (consume(')', true)) return builder_.make(AstKind::Unit, begin, "()");
        error(begin, "only '()' is valid as a pattern");
        return nullptr;
    }
    if (source_[offset_] == '"') {
        const std::string_view text = scan_string();
        std::string decoded;
        if (!failed_ && !decode_string(text, decoded)) {
            error(begin, "invalid string pattern");
            return nullptr;
        }
        return failed_ ? nullptr : builder_.make(AstKind::String, begin, text);
    }
    const std::string_view number = scan_number();
    if (!number.empty()) {
        std::int64_t integer = 0;
        if (decode_integer(number, integer)) return builder_.make(AstKind::Int, begin, number);
        double floating = 0.0;
        if (decode_float(number, floating)) return builder_.make(AstKind::Float, begin, number);
        error(begin, "invalid numeric pattern");
        return nullptr;
    }
    const std::string_view identifier = scan_identifier();
    if (identifier == "true" || identifier == "false") {
        return builder_.make(AstKind::Bool, begin, identifier);
    }
    if (identifier == "_") return builder_.make(AstKind::PatternWildcard, begin, identifier);
    if (!identifier.empty() && !is_keyword(identifier)) {
        return builder_.make(AstKind::PatternBinding, begin, identifier);
    }
    error(begin, "expected pattern");
    return nullptr;
}

AstNode* Parser::parse_match(std::size_t begin) {
    AstNode* subject = parse_expression();
    if (!subject) return nullptr;
    if (!consume('{')) {
        error(offset_, "expected '{' after match value");
        return nullptr;
    }
    AstNode* match = builder_.make(AstKind::Match, begin, "match");
    match->first = subject;
    AstNode** tail = &subject->next;
    consume_terminators();
    while (!failed_) {
        skip_trivia(false);
        if (consume('}', false)) return match;
        AstNode* pattern = parse_pattern();
        if (!pattern) return nullptr;
        if (!consume("=>", true)) {
            error(offset_, "expected '=>' after match pattern");
            return nullptr;
        }
        AstNode* body = nullptr;
        if (consume('{')) {
            body = parse_block(offset_ - 1U);
        } else {
            body = parse_expression();
        }
        if (!body) return nullptr;
        AstNode* arm = builder_.make(AstKind::MatchArm, pattern->position.byte_offset);
        arm->first = pattern;
        pattern->next = body;
        *tail = arm;
        tail = &arm->next;
        skip_trivia(false);
        if (consume(',', false)) {
            consume_terminators();
            continue;
        }
        if (consume('}', false)) return match;
        if (at_statement_end()) {
            consume_terminators();
            continue;
        }
        error(offset_, "expected match arm terminator");
        return nullptr;
    }
    return nullptr;
}

AstNode* Parser::parse_primary() {
    skip_trivia(nesting_depth_ != 0);
    if (failed_ || offset_ >= source_.size()) {
        if (!failed_) error(offset_, "expected expression");
        return nullptr;
    }
    const std::size_t begin = offset_;
    if (consume("if")) return parse_if(begin);
    if (consume("match")) return parse_match(begin);
    if (consume('{')) return parse_block(begin);
    if (consume("fn")) return parse_function(begin);
    if (consume("enum")) return parse_enum(begin);
    if (consume('(')) {
        if (consume(')', true)) return builder_.make(AstKind::Unit, begin, "()");
        return parse_delimited_expression(')', "grouped expression");
    }
    if (consume('[')) return parse_list(begin);
    if (source_.substr(offset_, 2) == "%{") {
        offset_ += 2;
        return parse_table(begin);
    }
    if (consume(':')) return parse_tag(begin);
    if (source_[offset_] == '"') {
        const std::string_view text = scan_string();
        std::string decoded;
        if (!failed_ && !decode_string(text, decoded)) {
            error(begin, "invalid string literal");
            return nullptr;
        }
        return failed_ ? nullptr : builder_.make(AstKind::String, begin, text);
    }
    const std::string_view number = scan_number();
    if (!number.empty()) {
        std::int64_t integer = 0;
        if (decode_integer(number, integer)) return builder_.make(AstKind::Int, begin, number);
        double floating = 0.0;
        if (decode_float(number, floating)) return builder_.make(AstKind::Float, begin, number);
        error(begin, "invalid numeric literal");
        return nullptr;
    }
    const std::string_view identifier = scan_identifier();
    if (!identifier.empty()) {
        if (identifier == "true" || identifier == "false") {
            return builder_.make(AstKind::Bool, begin, identifier);
        }
        if (is_keyword(identifier)) {
            error(begin, "keyword cannot appear as an expression");
            return nullptr;
        }
        return builder_.make(AstKind::Identifier, begin, identifier);
    }
    error(begin, "expected expression");
    return nullptr;
}

AstNode* Parser::parse_postfix() {
    AstNode* expression = parse_primary();
    if (!expression) return nullptr;
    while (!failed_) {
        skip_trivia(nesting_depth_ != 0);
        const std::size_t begin = offset_;
        if (consume('(')) {
            AstNode* call = builder_.make(AstKind::Call, begin);
            call->first = expression;
            ++nesting_depth_;
            if (!consume(')', true)) {
                while (!failed_) {
                    AstNode* argument = parse_expression();
                    if (!argument) {
                        --nesting_depth_;
                        return nullptr;
                    }
                    AstBuilder::append_child(expression->next, argument);
                    if (consume(')', true)) break;
                    if (!consume(',', true)) {
                        error(offset_, "expected ',' or ')' in call");
                        --nesting_depth_;
                        return nullptr;
                    }
                }
            }
            --nesting_depth_;
            expression = call;
            continue;
        }
        if (consume('[')) {
            AstNode* index = builder_.make(AstKind::Index, begin);
            index->first = expression;
            AstNode* subscript = parse_delimited_expression(']', "index expression");
            if (!subscript) return nullptr;
            expression->next = subscript;
            expression = index;
            continue;
        }
        if (source_.substr(offset_, 2) != ".." && consume('.')) {
            const std::string_view field = scan_identifier();
            if (field.empty() || is_keyword(field)) {
                error(offset_, "expected field name after '.'");
                return nullptr;
            }
            AstNode* access = builder_.make(AstKind::Field, begin, field);
            access->first = expression;
            expression = access;
            continue;
        }
        break;
    }
    return expression;
}

AstNode* Parser::parse_unary() {
    skip_trivia(nesting_depth_ != 0);
    const std::size_t begin = offset_;
    if (consume("not") || consume('-')) {
        const std::string_view operation = source_.substr(begin, offset_ - begin);
        AstNode* operand = parse_unary();
        if (!operand) return nullptr;
        AstNode* unary = builder_.make(AstKind::Unary, begin, operation);
        unary->first = operand;
        return unary;
    }
    if (consume('~')) {
        skip_trivia(nesting_depth_ != 0);
        if (at_statement_end() || source_[offset_] == ',' || source_[offset_] == ')' ||
            source_[offset_] == ']' || source_[offset_] == '}') {
            return builder_.make(AstKind::EffectResult, begin, "~");
        }
        AstNode* operand = parse_unary();
        if (!operand) return nullptr;
        AstNode* unary = builder_.make(AstKind::Unary, begin, "~");
        unary->first = operand;
        return unary;
    }
    if (consume('?')) {
        AstNode* optional = builder_.make(AstKind::Optional, begin, "?");
        skip_trivia(nesting_depth_ != 0);
        if (!at_statement_end() && source_[offset_] != ',' && source_[offset_] != ')' &&
            source_[offset_] != ']' && source_[offset_] != '}') {
            optional->first = parse_unary();
            if (!optional->first) return nullptr;
        }
        return optional;
    }
    return parse_postfix();
}

std::string_view Parser::peek_binary_operator() {
    skip_trivia(nesting_depth_ != 0);
    constexpr std::string_view operators[] = {
        "==", "!=", "<=", ">=", "..", "+", "-", "*", "/", "%", "<", ">",
    };
    for (const std::string_view operation : operators) {
        if (source_.substr(offset_, operation.size()) == operation) return operation;
    }
    if (at_word("and")) return "and";
    if (at_word("or")) return "or";
    return {};
}

AstNode* Parser::parse_expression(int minimum_precedence) {
    AstNode* left = parse_unary();
    if (!left) return nullptr;
    bool consumed_non_associative = false;
    while (!failed_) {
        const std::size_t operator_offset = offset_;
        const std::string_view operation = peek_binary_operator();
        const OperatorPrecedence* precedence = find_binary_operator(operation);
        if (!precedence || precedence->level < minimum_precedence) return left;
        if (precedence->associativity == Associativity::None && consumed_non_associative) {
            error(operator_offset, "range operator '..' is non-associative");
            return nullptr;
        }
        offset_ += operation.size();
        skip_trivia(true);
        AstNode* right = parse_expression(precedence->level + 1);
        if (!right) return nullptr;
        AstNode* binary = builder_.make(AstKind::Binary, operator_offset, operation);
        binary->first = left;
        left->next = right;
        left = binary;
        consumed_non_associative = precedence->associativity == Associativity::None;
    }
    return nullptr;
}

AstNode* Parser::parse_statement() {
    skip_trivia(false);
    const std::size_t begin = offset_;
    if (consume("return")) return parse_return(begin);
    if (consume("while")) return parse_while(begin);
    if (consume("for")) return parse_for(begin);
    if (consume("break")) return builder_.make(AstKind::Break, begin, "break");
    if (consume("continue")) return builder_.make(AstKind::Continue, begin, "continue");
    if (consume("let")) {
        const std::string_view name = scan_identifier();
        if (name.empty() || is_keyword(name)) {
            error(offset_, "expected binding name after 'let'");
            return nullptr;
        }
        skip_trivia(nesting_depth_ != 0);
        if (source_.substr(offset_, 2) == "==" || !consume('=')) {
            error(offset_, "expected '=' after binding name");
            return nullptr;
        }
        AstNode* initializer = parse_expression();
        if (!initializer) return nullptr;
        AstNode* binding = builder_.make(AstKind::Let, begin, name);
        binding->first = initializer;
        return binding;
    }

    // spec §9: a statement-leading '~' captures the complete following
    // expression, including its binary operators, before its follower runs.
    if (offset_ < source_.size() && source_[offset_] == '~') {
        std::size_t lookahead = offset_ + 1U;
        while (lookahead < source_.size() &&
               (source_[lookahead] == ' ' || source_[lookahead] == '\t')) {
            ++lookahead;
        }
        if (lookahead < source_.size() && source_[lookahead] != '\n' &&
            source_[lookahead] != '\r' && source_[lookahead] != ';' &&
            source_[lookahead] != '}') {
            ++offset_;
            AstNode* operand = parse_expression();
            if (!operand) return nullptr;
            AstNode* effect = builder_.make(AstKind::Unary, begin, "~");
            effect->first = operand;
            return effect;
        }
    }

    AstNode* expression = parse_expression();
    if (!expression) return nullptr;
    skip_trivia(false);
    if (!logical_newline_ && source_.substr(offset_, 2) != "==" &&
        offset_ < source_.size() && source_[offset_] == '=') {
        if (!is_lvalue(expression)) {
            error(expression->position.byte_offset, "left side of assignment is not assignable");
            return nullptr;
        }
        ++offset_;
        AstNode* value = parse_expression();
        if (!value) return nullptr;
        AstNode* assignment = builder_.make(AstKind::Assign, begin);
        assignment->first = expression;
        expression->next = value;
        return assignment;
    }
    return expression;
}

AstNode* Parser::parse_terminated_statement() {
    AstNode* statement = parse_statement();
    if (!statement) return nullptr;
    skip_trivia(false);
    if (!at_statement_end() && source_[offset_] != '}') {
        error(offset_, "expected statement terminator");
        return nullptr;
    }
    consume_terminators();
    if (statement->kind != AstKind::Unary || statement->text != "~") return statement;
    if (offset_ >= source_.size()) {
        error(statement->position.byte_offset, "effect capture requires a following statement");
        return nullptr;
    }
    AstNode* follower = parse_terminated_statement();
    if (!follower) return nullptr;
    AstNode* effect = builder_.make(AstKind::Effect, statement->position.byte_offset, "~");
    effect->first = statement->first;
    statement->first->next = follower;
    return effect;
}

AstNode* Parser::parse_program() {
    AstNode* program = builder_.make(AstKind::Program, 0);
    AstNode** tail = &program->first;
    consume_terminators();
    while (!failed_ && offset_ < source_.size()) {
        AstNode* statement = parse_terminated_statement();
        if (!statement) return nullptr;
        *tail = statement;
        tail = &statement->sibling;
    }
    return failed_ ? nullptr : program;
}

}  // namespace on1x::syntax
