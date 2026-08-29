#include "on1x/core/parser.hpp"

#include "dparse.h"

#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string_view>

extern "C" D_ParserTables parser_tables_gram;
extern "C" D_ParseNode *ambiguity_count_fn(D_Parser *pp, int n, D_ParseNode **v);

namespace on1x {
namespace {

static bool validate_with_dparser(const std::string &source, std::string &error) {
  D_Parser *parser = new_D_Parser(&parser_tables_gram, sizeof(D_ParseNode_User));
  if (!parser) {
    error = "failed to initialize dparser";
    return false;
  }

  parser->save_parse_tree = 1;
  parser->ambiguity_fn = ambiguity_count_fn;
  std::string mutable_source = source;
  D_ParseNode *tree = dparse(parser, mutable_source.data(), static_cast<int>(mutable_source.size()));
  if (!tree || parser->syntax_errors) {
    error = "syntax error";
    free_D_Parser(parser);
    return false;
  }

  free_D_ParseNode(parser, tree);
  free_D_Parser(parser);
  return true;
}

enum class kind {
  eof,
  ident,
  number,
  string_lit,
  lparen,
  rparen,
  lbrace,
  rbrace,
  lbracket,
  rbracket,
  comma,
  colon,
  semicolon,
  dot,
  plus,
  minus,
  star,
  slash,
  percent,
  bang,
  amp,
  pipe,
  caret,
  eq,
  lt,
  gt,
  arrow,
  fat_arrow,
  eqeq,
  ne,
  le,
  ge,
  andand,
  oror,
  shl,
  shr,
  pipe_greater,
  range,
  assign,
  dcolon,
};

struct token {
  kind k;
  std::string text;
  std::size_t pos = 0;
};

class lexer {
 public:
  explicit lexer(std::string_view src) : src_(src) {}

  std::vector<token> lex(std::string &error) {
    std::vector<token> out;
    while (true) {
      skip_ws();
      if (eof()) {
        out.push_back({kind::eof, "", pos_});
        return out;
      }
      const std::size_t start = pos_;
      char c = peek();
      if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        while (!eof() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) advance();
        out.push_back({kind::ident, std::string(src_.substr(start, pos_ - start)), start});
        continue;
      }
      if (std::isdigit(static_cast<unsigned char>(c))) {
        while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
        if (!eof() && peek() == '.') {
          advance();
          while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
        }
        out.push_back({kind::number, std::string(src_.substr(start, pos_ - start)), start});
        continue;
      }
      if (c == '"') {
        advance();
        std::string text;
        while (!eof() && peek() != '"') {
          if (peek() == '\\') {
            advance();
            if (eof()) break;
            char esc = advance();
            switch (esc) {
              case 'n': text.push_back('\n'); break;
              case 'r': text.push_back('\r'); break;
              case 't': text.push_back('\t'); break;
              case '\\': text.push_back('\\'); break;
              case '"': text.push_back('"'); break;
              default: text.push_back(esc); break;
            }
          } else {
            text.push_back(advance());
          }
        }
        if (eof()) {
          error = "unterminated string literal";
          return {};
        }
        advance();
        out.push_back({kind::string_lit, std::move(text), start});
        continue;
      }
      auto two = [this]() -> std::string {
        if (pos_ + 1 >= src_.size()) return {};
        return std::string(src_.substr(pos_, 2));
      };
      auto three = [this]() -> std::string {
        if (pos_ + 2 >= src_.size()) return {};
        return std::string(src_.substr(pos_, 3));
      };
      if (three() == ">>=" || three() == "<<=") {
        out.push_back({kind::assign, three(), start});
        pos_ += 3;
        continue;
      }
      if (two() == "->") { out.push_back({kind::arrow, "->", start}); pos_ += 2; continue; }
      if (two() == "=>") { out.push_back({kind::fat_arrow, "=>", start}); pos_ += 2; continue; }
      if (two() == "==") { out.push_back({kind::eqeq, "==", start}); pos_ += 2; continue; }
      if (two() == "!=") { out.push_back({kind::ne, "!=", start}); pos_ += 2; continue; }
      if (two() == "<=") { out.push_back({kind::le, "<=", start}); pos_ += 2; continue; }
      if (two() == ">=") { out.push_back({kind::ge, ">=", start}); pos_ += 2; continue; }
      if (two() == "&&") { out.push_back({kind::andand, "&&", start}); pos_ += 2; continue; }
      if (two() == "||") { out.push_back({kind::oror, "||", start}); pos_ += 2; continue; }
      if (two() == "<<") { out.push_back({kind::shl, "<<", start}); pos_ += 2; continue; }
      if (two() == ">>") { out.push_back({kind::shr, ">>", start}); pos_ += 2; continue; }
      if (two() == "|>") { out.push_back({kind::pipe_greater, "|>", start}); pos_ += 2; continue; }
      if (two() == "::") { out.push_back({kind::dcolon, "::", start}); pos_ += 2; continue; }
      if (two() == "..") { out.push_back({kind::range, "..", start}); pos_ += 2; continue; }
      switch (advance()) {
        case '(': out.push_back({kind::lparen, "(", start}); break;
        case ')': out.push_back({kind::rparen, ")", start}); break;
        case '{': out.push_back({kind::lbrace, "{", start}); break;
        case '}': out.push_back({kind::rbrace, "}", start}); break;
        case '[': out.push_back({kind::lbracket, "[", start}); break;
        case ']': out.push_back({kind::rbracket, "]", start}); break;
        case ',': out.push_back({kind::comma, ",", start}); break;
        case ':': out.push_back({kind::colon, ":", start}); break;
        case ';': out.push_back({kind::semicolon, ";", start}); break;
        case '.': out.push_back({kind::dot, ".", start}); break;
        case '+': out.push_back({kind::plus, "+", start}); break;
        case '-': out.push_back({kind::minus, "-", start}); break;
        case '*': out.push_back({kind::star, "*", start}); break;
        case '/': out.push_back({kind::slash, "/", start}); break;
        case '%': out.push_back({kind::percent, "%", start}); break;
        case '!': out.push_back({kind::bang, "!", start}); break;
        case '&': out.push_back({kind::amp, "&", start}); break;
        case '|': out.push_back({kind::pipe, "|", start}); break;
        case '^': out.push_back({kind::caret, "^", start}); break;
        case '=': out.push_back({kind::assign, "=", start}); break;
        case '<': out.push_back({kind::lt, "<", start}); break;
        case '>': out.push_back({kind::gt, ">", start}); break;
        case '#': break;
        default:
          error = "unexpected character at offset " + std::to_string(start);
          return {};
      }
    }
  }

 private:
  void skip_ws() {
    while (!eof()) {
      if (std::isspace(static_cast<unsigned char>(peek()))) {
        ++pos_;
        continue;
      }
      if (peek() == '-' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '-') {
        pos_ += 2;
        while (!eof() && peek() != '\n') ++pos_;
        continue;
      }
      break;
    }
  }

  bool eof() const { return pos_ >= src_.size(); }
  char peek() const { return src_[pos_]; }
  char advance() { return src_[pos_++]; }

  std::string_view src_;
  std::size_t pos_ = 0;
};

class parser {
 public:
  explicit parser(std::vector<token> tokens) : tokens_(std::move(tokens)) {}

  program parse(std::string &error) {
    program out;
    while (!check(kind::eof)) {
      auto form = parse_toplevel(error);
      if (!error.empty()) return {};
      if (form) out.forms.push_back(std::move(form));
      else synchronize();
    }
    return out;
  }

 private:
  expr_ptr parse_toplevel(std::string &error) {
    if (match_ident("module") || match_ident("import")) {
      skip_until_toplevel();
      return make_expr(nil_t{});
    }
    if (match_ident("type")) {
      skip_until_toplevel();
      return make_expr(nil_t{});
    }
    if (match_ident("fn")) {
      auto name = consume_ident(error, "expected function name");
      if (!error.empty()) return {};
      if (match(kind::lbracket) || match(kind::lt)) {
        skip_balanced(error, previous().k == kind::lbracket ? kind::rbracket : kind::gt);
        if (!error.empty()) return {};
      }
      consume(kind::lparen, error, "expected '(' after function name");
      if (!error.empty()) return {};
      std::vector<std::string> params;
      if (!check(kind::rparen)) {
        do {
          params.push_back(consume_ident(error, "expected parameter name"));
          if (!error.empty()) return {};
          if (match(kind::colon)) {
            parse_type(error);
            if (!error.empty()) return {};
          }
        } while (match(kind::comma));
      }
      consume(kind::rparen, error, "expected ')' after parameters");
      if (!error.empty()) return {};
      if (match(kind::arrow)) {
        parse_type(error);
        if (!error.empty()) return {};
      }
      auto body = parse_function_body(error);
      if (!error.empty()) return {};
      auto definition = make_expr(binary_t{
          "=",
          make_expr(ident_t{name}),
          make_expr(lambda_t{std::move(params), body}),
      });
      match(kind::semicolon);
      return definition;
    }
    auto expr = parse_sequence(error);
    if (!error.empty()) return {};
    match(kind::semicolon);
    return expr;
  }

  void parse_type(std::string &error) {
    int depth = 0;
    while (!check(kind::eof)) {
      if (depth == 0) {
        if (check(kind::comma) || check(kind::rparen) || check(kind::rbracket) ||
            check(kind::semicolon) || check(kind::arrow) || check(kind::fat_arrow) ||
            check(kind::assign) || check(kind::rbrace) || check(kind::eof) ||
            (check(kind::ident) &&
             (peek().text == "in" || peek().text == "else" || peek().text == "where" ||
              peek().text == "do" || peek().text == "with"))) {
          return;
        }
      }
      if (match(kind::lparen) || match(kind::lbracket) || match(kind::lbrace) || match(kind::lt)) {
        ++depth;
        continue;
      }
      if (match(kind::rparen) || match(kind::rbracket) || match(kind::rbrace) || match(kind::gt)) {
        if (depth > 0) --depth;
        else return;
        continue;
      }
      ++pos_;
    }
    if (depth != 0) error = "unterminated type annotation";
  }

  expr_ptr parse_block_or_expr(std::string &error) {
    if (match(kind::assign)) return parse_expression(error);
    if (match(kind::lbrace)) {
      std::vector<expr_ptr> stmts;
      while (!check(kind::rbrace) && !check(kind::eof)) {
        if (match(kind::semicolon)) continue;
        auto e = parse_expression(error);
        if (!error.empty()) return {};
        stmts.push_back(e);
        match(kind::semicolon);
      }
      consume(kind::rbrace, error, "expected '}'");
      if (!error.empty()) return {};
      if (stmts.empty()) return make_expr(nil_t{});
      auto value = stmts.back();
      stmts.pop_back();
      return make_expr(block_t{std::move(stmts), value});
    }
    return parse_sequence(error);
  }

  expr_ptr parse_function_body(std::string &error) {
    if (match(kind::assign)) return parse_expression(error);
    return parse_block_or_expr(error);
  }

  expr_ptr parse_sequence(std::string &error) {
    std::vector<expr_ptr> exprs;
    exprs.push_back(parse_expression(error));
    if (!error.empty()) return {};
    while (match(kind::semicolon)) {
      if (check(kind::eof) || check(kind::rbrace) || check(kind::rparen) || check(kind::rbracket)) {
        break;
      }
      exprs.push_back(parse_expression(error));
      if (!error.empty()) return {};
    }
    if (exprs.size() == 1) return exprs.front();
    auto value = exprs.back();
    exprs.pop_back();
    return make_expr(block_t{std::move(exprs), value});
  }

  expr_ptr parse_expression(std::string &error) { return parse_assignment(error); }

  expr_ptr parse_assignment(std::string &error) {
    auto lhs = parse_pipeline(error);
    if (!error.empty()) return {};
    if (match(kind::assign)) {
      auto rhs = parse_assignment(error);
      if (!error.empty()) return {};
      return make_expr(binary_t{"=", lhs, rhs});
    }
    return lhs;
  }

  expr_ptr parse_pipeline(std::string &error) {
    auto expr = parse_logical_or(error);
    while (match(kind::pipe_greater)) {
      auto rhs = parse_logical_or(error);
      if (!error.empty()) return {};
      expr = make_expr(binary_t{"|>", expr, rhs});
    }
    return expr;
  }

  expr_ptr parse_logical_or(std::string &error) {
    auto expr = parse_logical_and(error);
    while (match(kind::oror)) {
      auto rhs = parse_logical_and(error);
      if (!error.empty()) return {};
      expr = make_expr(binary_t{"||", expr, rhs});
    }
    return expr;
  }

  expr_ptr parse_logical_and(std::string &error) {
    auto expr = parse_equality(error);
    while (match(kind::andand)) {
      auto rhs = parse_equality(error);
      if (!error.empty()) return {};
      expr = make_expr(binary_t{"&&", expr, rhs});
    }
    return expr;
  }

  expr_ptr parse_equality(std::string &error) {
    auto expr = parse_comparison(error);
    while (true) {
      if (match(kind::eqeq)) {
        auto rhs = parse_comparison(error);
        if (!error.empty()) return {};
        expr = make_expr(binary_t{"==", expr, rhs});
      } else if (match(kind::ne)) {
        auto rhs = parse_comparison(error);
        if (!error.empty()) return {};
        expr = make_expr(binary_t{"!=", expr, rhs});
      } else {
        break;
      }
    }
    return expr;
  }

  expr_ptr parse_comparison(std::string &error) {
    auto expr = parse_term(error);
    while (true) {
      if (match(kind::lt)) expr = binary(expr, "<", parse_term(error), error);
      else if (match(kind::le)) expr = binary(expr, "<=", parse_term(error), error);
      else if (match(kind::gt)) expr = binary(expr, ">", parse_term(error), error);
      else if (match(kind::ge)) expr = binary(expr, ">=", parse_term(error), error);
      else break;
      if (!error.empty()) return {};
    }
    return expr;
  }

  expr_ptr parse_term(std::string &error) {
    auto expr = parse_factor(error);
    while (true) {
      if (match(kind::plus)) expr = binary(expr, "+", parse_factor(error), error);
      else if (match(kind::minus)) expr = binary(expr, "-", parse_factor(error), error);
      else if (match(kind::pipe)) expr = binary(expr, "|", parse_factor(error), error);
      else if (match(kind::caret)) expr = binary(expr, "^", parse_factor(error), error);
      else break;
      if (!error.empty()) return {};
    }
    return expr;
  }

  expr_ptr parse_factor(std::string &error) {
    auto expr = parse_unary(error);
    while (true) {
      if (match(kind::star)) expr = binary(expr, "*", parse_unary(error), error);
      else if (match(kind::slash)) expr = binary(expr, "/", parse_unary(error), error);
      else if (match(kind::percent)) expr = binary(expr, "%", parse_unary(error), error);
      else if (match(kind::shl)) expr = binary(expr, "<<", parse_unary(error), error);
      else if (match(kind::shr)) expr = binary(expr, ">>", parse_unary(error), error);
      else break;
      if (!error.empty()) return {};
    }
    return expr;
  }

  expr_ptr parse_unary(std::string &error) {
    if (match(kind::minus)) return make_expr(unary_t{"-", parse_unary(error)});
    if (match(kind::bang)) return make_expr(unary_t{"!", parse_unary(error)});
    if (match(kind::amp)) return make_expr(unary_t{"&", parse_unary(error)});
    return parse_call(error);
  }

  expr_ptr parse_call(std::string &error) {
    auto expr = parse_primary(error);
    while (match(kind::lparen)) {
      std::vector<expr_ptr> args;
      if (!check(kind::rparen)) {
        do {
          args.push_back(parse_expression(error));
          if (!error.empty()) return {};
        } while (match(kind::comma));
      }
      consume(kind::rparen, error, "expected ')'");
      if (!error.empty()) return {};
      expr = make_expr(call_t{expr, std::move(args)});
    }
    return expr;
  }

  expr_ptr parse_primary(std::string &error) {
    if (match(kind::number)) {
      const std::string text = previous().text;
      if (text.find('.') != std::string::npos) return make_expr(float_t{std::stod(text)});
      return make_expr(int_t{std::stoll(text)});
    }
    if (match(kind::string_lit)) return make_expr(string_t{previous().text});
    if (match_ident("true")) return make_expr(bool_t{true});
    if (match_ident("false")) return make_expr(bool_t{false});
    if (match_ident("let")) {
      auto name = consume_ident(error, "expected binding name");
      if (!error.empty()) return {};
      consume(kind::assign, error, "expected '=' after binding name");
      if (!error.empty()) return {};
      auto value = parse_expression(error);
      if (!error.empty()) return {};
      if (!match_ident("in")) {
        error = "expected 'in'";
        return {};
      }
      if (!error.empty()) return {};
      auto body = parse_expression(error);
      if (!error.empty()) return {};
      return make_expr(let_t{name, value, body});
    }
    if (match_ident("if")) {
      auto cond = parse_expression(error);
      if (!error.empty()) return {};
      if (match_ident("then")) {
        auto then_expr = parse_expression(error);
        if (!error.empty()) return {};
        if (!match_ident("else")) {
          error = "expected 'else'";
          return {};
        }
        auto else_expr = parse_expression(error);
        if (!error.empty()) return {};
        return make_expr(if_t{cond, then_expr, else_expr});
      }
      auto then_expr = parse_block_or_expr(error);
      if (!error.empty()) return {};
      expr_ptr else_expr = make_expr(nil_t{});
      if (match_ident("else")) {
        else_expr = parse_block_or_expr(error);
        if (!error.empty()) return {};
      }
      return make_expr(if_t{cond, then_expr, else_expr});
    }
    if (match_ident("fn")) {
      consume(kind::lparen, error, "expected '(' after fn");
      if (!error.empty()) return {};
      std::vector<std::string> params;
      if (!check(kind::rparen)) {
        do {
          params.push_back(consume_ident(error, "expected parameter name"));
          if (!error.empty()) return {};
          if (match(kind::colon)) {
            parse_type(error);
            if (!error.empty()) return {};
          }
        } while (match(kind::comma));
      }
      consume(kind::rparen, error, "expected ')' after parameters");
      if (!error.empty()) return {};
      if (match(kind::arrow)) {
        parse_type(error);
        if (!error.empty()) return {};
      }
      auto body = parse_block_or_expr(error);
      if (!error.empty()) return {};
      return make_expr(lambda_t{std::move(params), body});
    }
    if (match(kind::lparen)) {
      if (match(kind::rparen)) return make_expr(nil_t{});
      auto first = parse_expression(error);
      if (!error.empty()) return {};
      if (match(kind::comma)) {
        std::vector<expr_ptr> items{first};
        do {
          items.push_back(parse_expression(error));
          if (!error.empty()) return {};
        } while (match(kind::comma));
        consume(kind::rparen, error, "expected ')'");
        if (!error.empty()) return {};
        return make_expr(tuple_t{std::move(items)});
      }
      consume(kind::rparen, error, "expected ')'");
      if (!error.empty()) return {};
      return first;
    }
    if (match(kind::lbracket)) {
      std::vector<expr_ptr> items;
      if (!check(kind::rbracket)) {
        do {
          items.push_back(parse_expression(error));
          if (!error.empty()) return {};
        } while (match(kind::comma));
      }
      consume(kind::rbracket, error, "expected ']'");
      if (!error.empty()) return {};
      return make_expr(list_t{std::move(items)});
    }
    if (match(kind::lbrace)) {
      std::vector<std::pair<std::string, expr_ptr>> fields;
      if (!check(kind::rbrace)) {
        do {
          auto name = consume_ident(error, "expected field name");
          if (!error.empty()) return {};
          expr_ptr value = make_expr(ident_t{name});
          if (match(kind::colon)) {
            value = parse_expression(error);
            if (!error.empty()) return {};
          }
          fields.emplace_back(name, value);
        } while (match(kind::comma));
      }
      consume(kind::rbrace, error, "expected '}'");
      if (!error.empty()) return {};
      return make_expr(record_t{std::move(fields)});
    }
    if (match(kind::ident)) return make_expr(ident_t{previous().text});
    error = "unexpected token at offset " + std::to_string(peek().pos);
    return {};
  }

  expr_ptr binary(const expr_ptr &lhs, const char *op, const expr_ptr &rhs, std::string &error) {
    if (error.empty() && !rhs) error = "expected expression after operator";
    return make_expr(binary_t{op, lhs, rhs});
  }

  bool match(kind k) {
    if (check(k)) {
      ++pos_;
      return true;
    }
    return false;
  }

  bool match_ident(std::string_view text) {
    if (check(kind::ident) && peek().text == text) {
      ++pos_;
      return true;
    }
    return false;
  }

  std::string consume_ident(std::string &error, const char *message) {
    if (!match(kind::ident)) {
      error = message;
      return {};
    }
    return previous().text;
  }

  void consume(kind k, std::string &error, const char *message) {
    if (!match(k) && error.empty()) error = message;
  }

  bool check(kind k) const { return peek().k == k; }
  const token &peek() const { return tokens_[pos_]; }
  const token &previous() const { return tokens_[pos_ - 1]; }
  void advance() { if (!check(kind::eof)) ++pos_; }
  void skip_statement() {
    while (!check(kind::eof) && !check(kind::semicolon)) ++pos_;
    match(kind::semicolon);
  }
  void skip_until_toplevel() {
    int depth = 0;
    bool seen = false;
    while (!check(kind::eof)) {
      if (seen && depth == 0 && (check(kind::ident) &&
          (peek().text == "module" || peek().text == "import" || peek().text == "type" || peek().text == "fn"))) {
        return;
      }
      seen = true;
      if (match(kind::lbrace) || match(kind::lparen) || match(kind::lbracket)) {
        ++depth;
        continue;
      }
      if (match(kind::rbrace) || match(kind::rparen) || match(kind::rbracket)) {
        if (depth > 0) --depth;
        continue;
      }
      ++pos_;
    }
  }
  void skip_balanced(std::string &error, kind closing) {
    int depth = 1;
    while (!check(kind::eof) && depth > 0) {
      if (match(kind::lparen) || match(kind::lbracket) || match(kind::lbrace) || match(kind::lt)) {
        ++depth;
        continue;
      }
      if (match(closing)) {
        --depth;
        continue;
      }
      if ((closing == kind::rparen && match(kind::rparen)) ||
          (closing == kind::rbracket && match(kind::rbracket)) ||
          (closing == kind::rbrace && match(kind::rbrace)) ||
          (closing == kind::gt && match(kind::gt))) {
        --depth;
        continue;
      }
      ++pos_;
    }
    if (depth != 0) error = "unterminated generic parameter list";
  }
  void synchronize() {
    while (!check(kind::eof)) {
      if (match(kind::semicolon)) return;
      ++pos_;
    }
  }

  std::vector<token> tokens_;
  std::size_t pos_ = 0;
};

}  // namespace

program parse_program(const std::string &source, std::string &error) {
  if (!validate_with_dparser(source, error)) return {};
  error.clear();
  lexer lex(source);
  auto tokens = lex.lex(error);
  if (!error.empty()) return {};
  parser p(std::move(tokens));
  return p.parse(error);
}

}  // namespace on1x
