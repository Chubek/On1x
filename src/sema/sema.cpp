#include "on1x/sema/sema.hpp"
#include "on1x/egraph/egraph.hpp"

#include <algorithm>
#include <sstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace on1x::sema {

namespace {

bool matches_saturated_error(const expr_ptr &e, std::string_view marker) {
  if (!e) return false;
  auto binary = std::get_if<binary_t>(&e->node);
  if (!binary) return false;

  egraph::graph graph(*e);
  using namespace equinoxng::dsl_api;
  auto x = var("x");
  std::vector<egraph::rewrite_rule> rules = {
      rule("division-by-zero", call("/", {x, lit(std::int64_t{0})}),
           call("sema_error_div_zero")),
      rule("remainder-by-zero", call("%", {x, lit(std::int64_t{0})}),
           call("sema_error_mod_zero")),
  };
  graph.run(rules);
  std::ostringstream rendered;
  rendered << graph.extract().term;
  return rendered.str().find(marker) != std::string::npos;
}

}  // namespace

// ---------------------------------------------------------------------------
//  Type constructors
// ---------------------------------------------------------------------------

std::shared_ptr<type> make_unit_type() {
  auto t = std::make_shared<type>();
  t->tag = type::kind::unit;
  t->node = std::monostate{};
  return t;
}

std::shared_ptr<type> make_prim_type(type::kind k) {
  auto t = std::make_shared<type>();
  t->tag = k;
  t->node = std::monostate{};
  return t;
}

std::shared_ptr<type> make_ref_type(std::shared_ptr<type> inner, bool is_mut) {
  auto t = std::make_shared<type>();
  t->tag = type::kind::ref;
  t->node = type_ref{std::move(inner), is_mut};
  return t;
}

std::shared_ptr<type> make_list_type(std::shared_ptr<type> element) {
  auto t = std::make_shared<type>();
  t->tag = type::kind::list;
  t->node = type_list{std::move(element)};
  return t;
}

std::shared_ptr<type> make_tuple_type(std::vector<std::shared_ptr<type>> elements) {
  auto t = std::make_shared<type>();
  t->tag = type::kind::tuple;
  t->node = type_tuple{std::move(elements)};
  return t;
}

std::shared_ptr<type> make_record_type(std::vector<std::pair<std::string, std::shared_ptr<type>>> fields) {
  auto t = std::make_shared<type>();
  t->tag = type::kind::record;
  t->node = type_record{std::move(fields)};
  return t;
}

std::shared_ptr<type> make_func_type(std::vector<std::shared_ptr<type>> params, std::shared_ptr<type> ret) {
  auto t = std::make_shared<type>();
  t->tag = type::kind::func;
  t->node = type_func{std::move(params), std::move(ret)};
  return t;
}

std::shared_ptr<type> make_generic_type(std::string name, std::vector<std::shared_ptr<type>> args) {
  auto t = std::make_shared<type>();
  t->tag = type::kind::generic;
  t->node = type_generic{std::move(name), std::move(args)};
  return t;
}

std::shared_ptr<type> make_var_type(std::string name) {
  auto t = std::make_shared<type>();
  t->tag = type::kind::var;
  t->node = std::move(name);
  return t;
}

std::shared_ptr<type> make_error_type() {
  auto t = std::make_shared<type>();
  t->tag = type::kind::error;
  t->node = std::monostate{};
  return t;
}

// ---------------------------------------------------------------------------
//  type_to_string
// ---------------------------------------------------------------------------

static const char *prim_name(type::kind k) {
  switch (k) {
    case type::kind::unit:  return "()";
    case type::kind::i32:   return "i32";
    case type::kind::i64:   return "i64";
    case type::kind::f32:   return "f32";
    case type::kind::f64:   return "f64";
    case type::kind::bool_: return "bool";
    case type::kind::char_: return "char";
    case type::kind::str:   return "str";
    default: return "<?>";
  }
}

std::string type_to_string(const std::shared_ptr<type> &t) {
  if (!t) return "<null>";

  switch (t->tag) {
    case type::kind::unit:
    case type::kind::i32:
    case type::kind::i64:
    case type::kind::f32:
    case type::kind::f64:
    case type::kind::bool_:
    case type::kind::char_:
    case type::kind::str:
      return prim_name(t->tag);

    case type::kind::var:
      return std::get<std::string>(t->node);

    case type::kind::error:
      return "<error>";

    case type::kind::ref: {
      auto &r = std::get<type_ref>(t->node);
      std::string out = "&";
      if (r.is_mut) out += "mut ";
      out += type_to_string(r.inner);
      return out;
    }

    case type::kind::list: {
      auto &l = std::get<type_list>(t->node);
      return "[" + type_to_string(l.element) + "]";
    }

    case type::kind::tuple: {
      auto &tu = std::get<type_tuple>(t->node);
      std::string out = "(";
      for (std::size_t i = 0; i < tu.elements.size(); ++i) {
        if (i) out += ", ";
        out += type_to_string(tu.elements[i]);
      }
      out += ")";
      return out;
    }

    case type::kind::record: {
      auto &rec = std::get<type_record>(t->node);
      std::string out = "{ ";
      for (std::size_t i = 0; i < rec.fields.size(); ++i) {
        if (i) out += ", ";
        out += rec.fields[i].first + ": " + type_to_string(rec.fields[i].second);
      }
      out += " }";
      return out;
    }

    case type::kind::func: {
      auto &f = std::get<type_func>(t->node);
      std::string out = "(";
      for (std::size_t i = 0; i < f.params.size(); ++i) {
        if (i) out += ", ";
        out += type_to_string(f.params[i]);
      }
      out += ") -> " + type_to_string(f.ret);
      return out;
    }

    case type::kind::generic: {
      auto &g = std::get<type_generic>(t->node);
      std::string out = g.name;
      if (!g.args.empty()) {
        out += "<";
        for (std::size_t i = 0; i < g.args.size(); ++i) {
          if (i) out += ", ";
          out += type_to_string(g.args[i]);
        }
        out += ">";
      }
      return out;
    }
  }

  return "<unknown>";
}

// ---------------------------------------------------------------------------
//  scope
// ---------------------------------------------------------------------------

scope::scope(std::shared_ptr<scope> parent) : parent(std::move(parent)) {}

std::optional<symbol> scope::lookup(const std::string &name) const {
  auto it = symbols.find(name);
  if (it != symbols.end()) return it->second;
  if (parent) return parent->lookup(name);
  return std::nullopt;
}

std::optional<std::shared_ptr<type>> scope::lookup_type(const std::string &name) const {
  auto it = type_aliases.find(name);
  if (it != type_aliases.end()) return it->second;
  if (parent) return parent->lookup_type(name);
  return std::nullopt;
}

bool scope::define(const std::string &name, std::shared_ptr<type> ty, bool is_mut) {
  if (symbols.count(name)) return false;
  symbols[name] = symbol{name, std::move(ty), is_mut, 0, 0};
  return true;
}

bool scope::define_alias(const std::string &name, std::shared_ptr<type> ty) {
  if (type_aliases.count(name)) return false;
  type_aliases[name] = std::move(ty);
  return true;
}

// ---------------------------------------------------------------------------
//  diagnostics
// ---------------------------------------------------------------------------

bool diagnostics::has_errors() const {
  return std::any_of(messages.begin(), messages.end(),
                     [](const diagnostic &d) { return d.lvl == diagnostic::level::error; });
}

void diagnostics::error(std::size_t line, std::size_t col, std::string msg) {
  messages.push_back({diagnostic::level::error, line, col, std::move(msg)});
}

void diagnostics::warn(std::size_t line, std::size_t col, std::string msg) {
  messages.push_back({diagnostic::level::warning, line, col, std::move(msg)});
}

void diagnostics::note(std::size_t line, std::size_t col, std::string msg) {
  messages.push_back({diagnostic::level::note, line, col, std::move(msg)});
}

std::string diagnostics::report() const {
  std::ostringstream out;
  for (const auto &d : messages) {
    const char *tag = "error";
    if (d.lvl == diagnostic::level::warning) tag = "warning";
    else if (d.lvl == diagnostic::level::note) tag = "note";
    out << tag << "[" << d.line << ":" << d.col << "]: " << d.message << "\n";
  }
  return out.str();
}

// ---------------------------------------------------------------------------
//  type_checker
// ---------------------------------------------------------------------------

type_checker::type_checker() : global(std::make_shared<scope>()) {
  // register built-in primitive types
  global->define_alias("i32",  make_prim_type(type::kind::i32));
  global->define_alias("i64",  make_prim_type(type::kind::i64));
  global->define_alias("f32",  make_prim_type(type::kind::f32));
  global->define_alias("f64",  make_prim_type(type::kind::f64));
  global->define_alias("bool", make_prim_type(type::kind::bool_));
  global->define_alias("char", make_prim_type(type::kind::char_));
  global->define_alias("str",  make_prim_type(type::kind::str));
  global->define_alias("unit", make_unit_type());
}

// ---------------------------------------------------------------------------
//  is_subtype / is_assignable / common_type / unify
// ---------------------------------------------------------------------------

bool type_checker::is_subtype(const std::shared_ptr<type> &sub, const std::shared_ptr<type> &super) {
  if (!sub || !super) return false;
  if (sub->tag == type::kind::error || super->tag == type::kind::error) return true;
  if (sub->tag == type::kind::var || super->tag == type::kind::var) return true;
  if (sub->tag == super->tag) {
    switch (sub->tag) {
      case type::kind::unit:
      case type::kind::i32:
      case type::kind::i64:
      case type::kind::f32:
      case type::kind::f64:
      case type::kind::bool_:
      case type::kind::char_:
      case type::kind::str:
        return true;

      case type::kind::ref: {
        auto &sr = std::get<type_ref>(sub->node);
        auto &tr = std::get<type_ref>(super->node);
        if (tr.is_mut && !sr.is_mut) return false;
        return is_subtype(sr.inner, tr.inner);
      }

      case type::kind::list: {
        auto &sl = std::get<type_list>(sub->node);
        auto &tl = std::get<type_list>(super->node);
        return is_subtype(sl.element, tl.element);
      }

      case type::kind::tuple: {
        auto &st = std::get<type_tuple>(sub->node);
        auto &tt = std::get<type_tuple>(super->node);
        if (st.elements.size() != tt.elements.size()) return false;
        for (std::size_t i = 0; i < st.elements.size(); ++i)
          if (!is_subtype(st.elements[i], tt.elements[i])) return false;
        return true;
      }

      case type::kind::record: {
        auto &sr = std::get<type_record>(sub->node);
        auto &tr = std::get<type_record>(super->node);
        if (sr.fields.size() != tr.fields.size()) return false;
        for (std::size_t i = 0; i < sr.fields.size(); ++i) {
          if (sr.fields[i].first != tr.fields[i].first) return false;
          if (!is_subtype(sr.fields[i].second, tr.fields[i].second)) return false;
        }
        return true;
      }

      case type::kind::func: {
        auto &sf = std::get<type_func>(sub->node);
        auto &tf = std::get<type_func>(super->node);
        if (sf.params.size() != tf.params.size()) return false;
        for (std::size_t i = 0; i < sf.params.size(); ++i)
          if (!is_subtype(tf.params[i], sf.params[i])) return false;
        return is_subtype(sf.ret, tf.ret);
      }

      default:
        return false;
    }
  }

  // numeric promotion: i32 → i64 → f32 → f64
  if (super->tag == type::kind::f64 &&
      (sub->tag == type::kind::i32 || sub->tag == type::kind::i64 || sub->tag == type::kind::f32))
    return true;
  if (super->tag == type::kind::f32 &&
      (sub->tag == type::kind::i32 || sub->tag == type::kind::i64))
    return true;
  if (super->tag == type::kind::i64 && sub->tag == type::kind::i32)
    return true;

  return false;
}

bool type_checker::is_assignable(const std::shared_ptr<type> &from, const std::shared_ptr<type> &to) {
  return is_subtype(from, to);
}

std::shared_ptr<type> type_checker::common_type(const std::shared_ptr<type> &a, const std::shared_ptr<type> &b) {
  if (!a || !b) return make_error_type();
  if (a->tag == type::kind::error) return b;
  if (b->tag == type::kind::error) return a;
  if (is_subtype(a, b)) return b;
  if (is_subtype(b, a)) return a;

  // try numeric promotion
  static const std::vector<type::kind> promo = {
    type::kind::i32, type::kind::i64, type::kind::f32, type::kind::f64
  };
  auto idx = [](type::kind k) -> int {
    for (int i = 0; i < static_cast<int>(promo.size()); ++i)
      if (promo[i] == k) return i;
    return -1;
  };
  int ia = idx(a->tag);
  int ib = idx(b->tag);
  if (ia >= 0 && ib >= 0) return make_prim_type(promo[std::max(ia, ib)]);

  return make_error_type();
}

std::shared_ptr<type> type_checker::unify(const std::shared_ptr<type> &a, const std::shared_ptr<type> &b) {
  return common_type(a, b);
}

std::shared_ptr<type> type_checker::resolve_alias(const std::shared_ptr<type> &t,
                                                   const std::shared_ptr<scope> &sc) {
  if (!t) return make_error_type();
  if (t->tag == type::kind::generic) {
    auto &g = std::get<type_generic>(t->node);
    auto resolved = sc->lookup_type(g.name);
    if (resolved) return *resolved;
    // resolve args recursively
    for (auto &arg : g.args) arg = resolve_alias(arg, sc);
  }
  return t;
}

// ---------------------------------------------------------------------------
//  type_of_literal / type_of_unary / type_of_binary
// ---------------------------------------------------------------------------

std::shared_ptr<type> type_checker::type_of_literal(const expr_ptr &e) {
  return std::visit([&](auto &&val) -> std::shared_ptr<type> {
    using T = std::decay_t<decltype(val)>;
    if constexpr (std::is_same_v<T, bool_t>)
      return make_prim_type(type::kind::bool_);
    else if constexpr (std::is_same_v<T, int_t>)
      return make_prim_type(type::kind::i64);
    else if constexpr (std::is_same_v<T, float_t>)
      return make_prim_type(type::kind::f64);
    else if constexpr (std::is_same_v<T, string_t>)
      return make_prim_type(type::kind::str);
    else if constexpr (std::is_same_v<T, nil_t>)
      return make_unit_type();
    else
      return make_error_type();
  }, e->node);
}

std::shared_ptr<type> type_checker::type_of_unary(const std::string &op,
                                                   const std::shared_ptr<type> &rhs) {
  if (!rhs || rhs->tag == type::kind::error) return make_error_type();

  if (op == "!") {
    if (rhs->tag == type::kind::bool_) return make_prim_type(type::kind::bool_);
    return make_error_type();
  }
  if (op == "-") {
    if (rhs->tag == type::kind::i32 || rhs->tag == type::kind::i64 ||
        rhs->tag == type::kind::f32 || rhs->tag == type::kind::f64)
      return rhs;
    return make_error_type();
  }
  if (op == "~") {
    if (rhs->tag == type::kind::i32 || rhs->tag == type::kind::i64)
      return rhs;
    return make_error_type();
  }
  if (op == "*") {
    // dereference
    if (rhs->tag == type::kind::ref)
      return std::get<type_ref>(rhs->node).inner;
    return make_error_type();
  }
  if (op == "&") {
    // reference
    return make_ref_type(rhs, false);
  }

  return make_error_type();
}

std::shared_ptr<type> type_checker::type_of_binary(const std::string &op,
                                                    const std::shared_ptr<type> &lhs,
                                                    const std::shared_ptr<type> &rhs) {
  if (!lhs || !rhs || lhs->tag == type::kind::error || rhs->tag == type::kind::error)
    return make_error_type();

  // arithmetic
  if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%" || op == "**") {
    auto ct = common_type(lhs, rhs);
    if (op == "%" && (ct->tag == type::kind::f32 || ct->tag == type::kind::f64))
      return make_error_type();
    if (ct->tag == type::kind::i32 || ct->tag == type::kind::i64 ||
        ct->tag == type::kind::f32 || ct->tag == type::kind::f64)
      return ct;
    return make_error_type();
  }

  // comparison
  if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
    if (!is_subtype(lhs, rhs) && !is_subtype(rhs, lhs)) return make_error_type();
    return make_prim_type(type::kind::bool_);
  }

  // logical
  if (op == "&&" || op == "||") {
    if (lhs->tag == type::kind::bool_ && rhs->tag == type::kind::bool_)
      return make_prim_type(type::kind::bool_);
    return make_error_type();
  }

  // bitwise
  if (op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>") {
    if ((lhs->tag == type::kind::i32 || lhs->tag == type::kind::i64) &&
        (rhs->tag == type::kind::i32 || rhs->tag == type::kind::i64))
      return lhs;
    return make_error_type();
  }

  // string concatenation
  if (op == "++") {
    if (lhs->tag == type::kind::str && rhs->tag == type::kind::str)
      return make_prim_type(type::kind::str);
    return make_error_type();
  }

  // assignment
  if (op == "=") {
    if (is_assignable(rhs, lhs)) return lhs;
    return make_error_type();
  }
  if (op == "+=" || op == "-=" || op == "*=" || op == "/=" || op == "%=") {
    if (lhs->tag == type::kind::i32 || lhs->tag == type::kind::i64 ||
        lhs->tag == type::kind::f32 || lhs->tag == type::kind::f64) {
      if (is_assignable(rhs, lhs)) return lhs;
    }
    return make_error_type();
  }
  if (op == "&=" || op == "|=" || op == "^=" || op == "<<=" || op == ">>=") {
    if ((lhs->tag == type::kind::i32 || lhs->tag == type::kind::i64) &&
        (rhs->tag == type::kind::i32 || rhs->tag == type::kind::i64))
      return lhs;
    return make_error_type();
  }

  return make_error_type();
}

// ---------------------------------------------------------------------------
//  infer
// ---------------------------------------------------------------------------

std::shared_ptr<type> type_checker::infer(const expr_ptr &e, std::shared_ptr<scope> sc) {
  if (!e) return make_unit_type();

  return std::visit([&](auto &&val) -> std::shared_ptr<type> {
    using T = std::decay_t<decltype(val)>;

    // literals
    if constexpr (std::is_same_v<T, nil_t> ||
                  std::is_same_v<T, bool_t> ||
                  std::is_same_v<T, int_t> ||
                  std::is_same_v<T, float_t> ||
                  std::is_same_v<T, string_t>) {
      return type_of_literal(e);
    }

    // identifier
    if constexpr (std::is_same_v<T, ident_t>) {
      auto sym = sc->lookup(val.value);
      if (!sym) {
        diags.error(0, 0, "undefined variable: " + val.value);
        return make_error_type();
      }
      return sym->ty;
    }

    // unary
    if constexpr (std::is_same_v<T, unary_t>) {
      auto rhs_ty = infer(val.rhs, sc);
      if (diags.has_errors()) return make_error_type();
      auto result = type_of_unary(val.op, rhs_ty);
      if (result->tag == type::kind::error) {
        diags.error(0, 0, "invalid unary operator '" + val.op +
                    "' on type " + type_to_string(rhs_ty));
        return make_error_type();
      }
      return result;
    }

    // binary
    if constexpr (std::is_same_v<T, binary_t>) {
      auto lhs_ty = infer(val.lhs, sc);
      auto rhs_ty = infer(val.rhs, sc);
      if (diags.has_errors()) return make_error_type();

      if (matches_saturated_error(e, "sema_error_div_zero")) {
        diags.error(0, 0, "division by zero");
        return make_error_type();
      }
      if (matches_saturated_error(e, "sema_error_mod_zero")) {
        diags.error(0, 0, "remainder by zero");
        return make_error_type();
      }

      // assignment: lookup the left-hand identifier
      if (val.op == "=" || val.op == "+=" || val.op == "-=" || val.op == "*=" ||
          val.op == "/=" || val.op == "%=" || val.op == "&=" || val.op == "|=" ||
          val.op == "^=" || val.op == "<<=" || val.op == ">>=") {
        auto id = std::get_if<ident_t>(&val.lhs->node);
        if (!id) {
          diags.error(0, 0, "left side of assignment must be an identifier");
          return make_error_type();
        }
        auto sym = sc->lookup(id->value);
        if (!sym) {
          diags.error(0, 0, "undefined variable: " + id->value);
          return make_error_type();
        }
        if (!sym->is_mutable) {
          diags.error(0, 0, "cannot assign to immutable variable: " + id->value);
          return make_error_type();
        }
        if (!is_assignable(rhs_ty, sym->ty)) {
          diags.error(0, 0, "cannot assign value of type " + type_to_string(rhs_ty) +
                      " to variable of type " + type_to_string(sym->ty));
          return make_error_type();
        }
        return sym->ty;
      }

      auto result = type_of_binary(val.op, lhs_ty, rhs_ty);
      if (result->tag == type::kind::error) {
        diags.error(0, 0, "invalid binary operator '" + val.op +
                    "' between types " + type_to_string(lhs_ty) +
                    " and " + type_to_string(rhs_ty));
        return make_error_type();
      }
      return result;
    }

    // if
    if constexpr (std::is_same_v<T, if_t>) {
      auto cond_ty = infer(val.cond, sc);
      if (diags.has_errors()) return make_error_type();
      if (cond_ty->tag != type::kind::bool_ && cond_ty->tag != type::kind::error) {
        diags.error(0, 0, "if condition must be bool, got " + type_to_string(cond_ty));
        return make_error_type();
      }
      auto then_ty = infer(val.then_branch, sc);
      auto else_ty = infer(val.else_branch, sc);
      if (diags.has_errors()) return make_error_type();
      auto result = common_type(then_ty, else_ty);
      if (result->tag == type::kind::error) {
        diags.error(0, 0, "if branches have incompatible types: " +
                    type_to_string(then_ty) + " and " + type_to_string(else_ty));
        return make_error_type();
      }
      return result;
    }

    // let
    if constexpr (std::is_same_v<T, let_t>) {
      auto val_ty = infer(val.value, sc);
      if (diags.has_errors()) return make_error_type();
      auto inner = std::make_shared<scope>(sc);
      if (!inner->define(val.name, val_ty)) {
        diags.error(0, 0, "duplicate definition: " + val.name);
        return make_error_type();
      }
      return infer(val.body, inner);
    }

    // block
    if constexpr (std::is_same_v<T, block_t>) {
      auto inner = std::make_shared<scope>(sc);
      std::shared_ptr<type> last_ty = make_unit_type();
      for (const auto &stmt : val.statements) {
        last_ty = infer(stmt, inner);
        if (diags.has_errors()) return make_error_type();
      }
      if (val.value) {
        last_ty = infer(val.value, inner);
        if (diags.has_errors()) return make_error_type();
      }
      return last_ty;
    }

    // list
    if constexpr (std::is_same_v<T, list_t>) {
      if (val.elements.empty()) return make_list_type(make_var_type("a"));
      auto first_ty = infer(val.elements[0], sc);
      if (diags.has_errors()) return make_error_type();
      for (std::size_t i = 1; i < val.elements.size(); ++i) {
        auto el_ty = infer(val.elements[i], sc);
        if (diags.has_errors()) return make_error_type();
        if (!is_subtype(el_ty, first_ty)) {
          diags.error(0, 0, "list element type mismatch: expected " +
                      type_to_string(first_ty) + ", got " + type_to_string(el_ty));
          return make_error_type();
        }
      }
      return make_list_type(first_ty);
    }

    // tuple
    if constexpr (std::is_same_v<T, tuple_t>) {
      std::vector<std::shared_ptr<type>> types;
      for (const auto &el : val.elements) {
        types.push_back(infer(el, sc));
        if (diags.has_errors()) return make_error_type();
      }
      return make_tuple_type(std::move(types));
    }

    // record
    if constexpr (std::is_same_v<T, record_t>) {
      std::vector<std::pair<std::string, std::shared_ptr<type>>> fields;
      std::set<std::string> names;
      for (const auto &[name, ex] : val.fields) {
        if (!names.insert(name).second) {
          diags.error(0, 0, "duplicate record field: " + name);
          return make_error_type();
        }
        fields.emplace_back(name, infer(ex, sc));
        if (diags.has_errors()) return make_error_type();
      }
      return make_record_type(std::move(fields));
    }

    // lambda
    if constexpr (std::is_same_v<T, lambda_t>) {
      auto inner = std::make_shared<scope>(sc);
      std::vector<std::shared_ptr<type>> param_types;
      for (const auto &p : val.params) {
        // untyped parameters are inferred later
        auto pty = make_var_type(p);
        if (!inner->define(p, pty)) {
          diags.error(0, 0, "duplicate lambda parameter: " + p);
          return make_error_type();
        }
        param_types.push_back(pty);
      }
      auto body_ty = infer(val.body, inner);
      if (diags.has_errors()) return make_error_type();
      return make_func_type(std::move(param_types), body_ty);
    }

    // call
    if constexpr (std::is_same_v<T, call_t>) {
      auto callee_ty = infer(val.callee, sc);
      if (diags.has_errors()) return make_error_type();
      if (callee_ty->tag != type::kind::func) {
        diags.error(0, 0, "value of type " + type_to_string(callee_ty) + " is not callable");
        return make_error_type();
      }
      auto &func = std::get<type_func>(callee_ty->node);
      if (func.params.size() != val.args.size()) {
        diags.error(0, 0, "function expects " + std::to_string(func.params.size()) +
                    " arguments, got " + std::to_string(val.args.size()));
        return make_error_type();
      }
      for (std::size_t i = 0; i < val.args.size(); ++i) {
        auto arg_ty = infer(val.args[i], sc);
        if (diags.has_errors()) return make_error_type();
        if (!is_assignable(arg_ty, func.params[i])) {
          diags.error(0, 0, "argument " + std::to_string(i + 1) + " type mismatch: expected " +
                      type_to_string(func.params[i]) + ", got " + type_to_string(arg_ty));
          return make_error_type();
        }
      }
      return func.ret;
    }

    return make_error_type();
  }, e->node);
}

// ---------------------------------------------------------------------------
//  check_expr / check
// ---------------------------------------------------------------------------

typed_expr_ptr type_checker::check_expr(const expr_ptr &e, std::shared_ptr<scope> sc) {
  auto t = std::make_shared<typed_expr>();
  t->source = e;
  t->inferred = infer(e, sc);
  return t;
}

 typed_program type_checker::check(const on1x::program &prog) {
  typed_program out;
  for (const auto &form : prog.forms) {
    auto checked = check_expr(form, global);
    out.forms.push_back(checked);
    if (diags.has_errors()) break;
  }
  return out;
}

}  // namespace on1x::sema
