#ifndef ON1X_SEMA_SEMA_HPP
#define ON1X_SEMA_SEMA_HPP

 #include "on1x/core/ast.hpp"
 #include "on1x/core/parser.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace on1x::sema {

// ---------------------------------------------------------------------------
//  Type representation — concrete types must be defined before `type`
// ---------------------------------------------------------------------------

struct type;

struct type_ref {
  std::shared_ptr<type> inner;
  bool is_mut = false;
};

struct type_list {
  std::shared_ptr<type> element;
};

struct type_tuple {
  std::vector<std::shared_ptr<type>> elements;
};

struct type_record {
  std::vector<std::pair<std::string, std::shared_ptr<type>>> fields;
};

struct type_func {
  std::vector<std::shared_ptr<type>> params;
  std::shared_ptr<type> ret;
};

struct type_generic {
  std::string name;
  std::vector<std::shared_ptr<type>> args;
};

struct type {
  enum class kind {
    unit,
    i32,
    i64,
    f32,
    f64,
    bool_,
    char_,
    str,
    var,
    ref,
    list,
    tuple,
    record,
    func,
    generic,
    error,
  };

  using node_t = std::variant<
    std::monostate,
    type_ref,
    type_list,
    type_tuple,
    type_record,
    type_func,
    type_generic,
    std::string
  >;

  kind tag;
  node_t node;
};

std::shared_ptr<type> make_unit_type();
std::shared_ptr<type> make_prim_type(type::kind k);
std::shared_ptr<type> make_ref_type(std::shared_ptr<type> inner, bool is_mut);
std::shared_ptr<type> make_list_type(std::shared_ptr<type> element);
std::shared_ptr<type> make_tuple_type(std::vector<std::shared_ptr<type>> elements);
std::shared_ptr<type> make_record_type(std::vector<std::pair<std::string, std::shared_ptr<type>>> fields);
std::shared_ptr<type> make_func_type(std::vector<std::shared_ptr<type>> params, std::shared_ptr<type> ret);
std::shared_ptr<type> make_generic_type(std::string name, std::vector<std::shared_ptr<type>> args);
std::shared_ptr<type> make_var_type(std::string name);
std::shared_ptr<type> make_error_type();

std::string type_to_string(const std::shared_ptr<type> &t);

// ---------------------------------------------------------------------------
//  Typed AST
// ---------------------------------------------------------------------------

struct typed_expr;
using typed_expr_ptr = std::shared_ptr<typed_expr>;

struct typed_expr {
  expr_ptr source;
  std::shared_ptr<type> inferred;
  std::vector<typed_expr_ptr> children;
};

struct typed_program {
  std::vector<typed_expr_ptr> forms;
};

// ---------------------------------------------------------------------------
//  Scope / Symbol table
// ---------------------------------------------------------------------------

struct symbol {
  std::string name;
  std::shared_ptr<type> ty;
  bool is_mutable = false;
  std::size_t def_line = 0;
  std::size_t def_col = 0;
};

struct scope {
  std::shared_ptr<scope> parent;
  std::map<std::string, symbol> symbols;
  std::map<std::string, std::shared_ptr<type>> type_aliases;

  explicit scope(std::shared_ptr<scope> parent = {});

  std::optional<symbol> lookup(const std::string &name) const;
  std::optional<std::shared_ptr<type>> lookup_type(const std::string &name) const;
  bool define(const std::string &name, std::shared_ptr<type> ty, bool is_mut = false);
  bool define_alias(const std::string &name, std::shared_ptr<type> ty);
};

// ---------------------------------------------------------------------------
//  Diagnostics
// ---------------------------------------------------------------------------

struct diagnostic {
  enum class level { error, warning, note };
  level lvl = level::error;
  std::size_t line = 0;
  std::size_t col = 0;
  std::string message;
};

struct diagnostics {
  std::vector<diagnostic> messages;

  bool has_errors() const;
  void error(std::size_t line, std::size_t col, std::string msg);
  void warn(std::size_t line, std::size_t col, std::string msg);
  void note(std::size_t line, std::size_t col, std::string msg);
  std::string report() const;
};

// ---------------------------------------------------------------------------
//  Type checker
// ---------------------------------------------------------------------------

struct type_checker {
  std::shared_ptr<scope> global;
  diagnostics diags;

  explicit type_checker();

   typed_program check(const on1x::program &prog);

private:
  typed_expr_ptr check_expr(const expr_ptr &e, std::shared_ptr<scope> sc);
  std::shared_ptr<type> infer(const expr_ptr &e, std::shared_ptr<scope> sc);

  bool is_assignable(const std::shared_ptr<type> &from, const std::shared_ptr<type> &to);
  bool is_subtype(const std::shared_ptr<type> &sub, const std::shared_ptr<type> &super);
  std::shared_ptr<type> unify(const std::shared_ptr<type> &a, const std::shared_ptr<type> &b);
  std::shared_ptr<type> common_type(const std::shared_ptr<type> &a, const std::shared_ptr<type> &b);
  std::shared_ptr<type> resolve_alias(const std::shared_ptr<type> &t, const std::shared_ptr<scope> &sc);

  std::shared_ptr<type> type_of_literal(const expr_ptr &e);
  std::shared_ptr<type> type_of_unary(const std::string &op, const std::shared_ptr<type> &rhs);
  std::shared_ptr<type> type_of_binary(const std::string &op,
                                        const std::shared_ptr<type> &lhs,
                                        const std::shared_ptr<type> &rhs);
};

}  // namespace on1x::sema

#endif
