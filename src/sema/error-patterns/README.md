# Semantic error patterns

These files use Equinox-NG rewrite syntax. The semantic checker applies the
same patterns through `on1x::egraph::graph` and uses the marker terms as
canonical error categories.

- `(/ x 0) => sema_error_div_zero`
- `(% x 0) => sema_error_mod_zero`
- `common-semantic-errors.rwr` adds broader diagnostic shapes for the semantic checker.
