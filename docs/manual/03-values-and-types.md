# 3. Values and Types

## Dynamic typing

In On1x, **values carry types, not variables.** A binding is just a name; the
value it refers to determines its type at any moment:

```on1x
let x = 42        // x holds an :Int
x = "hello"       // now x holds a :String
x = [1, 2, 3]     // now x holds a :List
```

This is the same model used in Scheme, Ruby, Python, and Lua. There are no type
declarations, no type annotations on variables, and no compile-time type
checking.

## TypeOf

The built-in function `TypeOf(v)` returns a Tag naming the type of `v`:

```on1x
TypeOf(42)          // :Int
TypeOf(3.14)        // :Float
TypeOf("hi")        // :String
TypeOf(:Red)        // :Tag
TypeOf(true)        // :Bool
TypeOf(())          // :Unit
TypeOf([1, 2])      // :List
TypeOf(%{})         // :Table
TypeOf(fn(x) { x }) // :Fn
TypeOf(Iota)        // :Iota
```

## The complete set of types

On1x has exactly ten type Tags:

| Type | Description | Example |
|------|-------------|---------|
| `:Unit` | A single value representing "no value" | `()` |
| `:Bool` | Boolean truth values | `true`, `false` |
| `:Int` | 64-bit signed integer | `42`, `-7`, `0xFF` |
| `:Float` | IEEE 754 binary64 double | `3.14`, `1e-9` |
| `:String` | Immutable UTF-8 text | `"hello"` |
| `:Tag` | Interned symbol | `:Red`, `:"two words"` |
| `:List` | Ordered, mutable sequence | `[1, "two", :three]` |
| `:Table` | Unordered key-value map | `%{ "x" => 1 }` |
| `:Fn` | First-class function | `fn(x) { x + 1 }` |
| `:Iota` | Auto-numbering / range value | `Iota` |

A **Tagged List** (e.g. `:Point[3, 4]`) is still a `:List` as far as `TypeOf`
is concerned. Use `TagOf` to inspect the constructor tag (Chapter 10).

## No implicit conversions

On1x does not implicitly convert between types. The condition of an `if` or
`while` must be a `:Bool`. Adding a `:String` and an `:Int` raises an error.
The operators `==` and `!=` do compare `:Int` and `:Float` numerically (`1 ==
1.0` is `true`), but that is comparison, not conversion.

```on1x
if 1 { ... }         // ERROR: condition must be :Bool
"x" + 3              // ERROR: cannot add String and Int
```

## No nil

On1x has no `nil` / `null` / `undefined` / `None`-as-a-sentinel-for-all-types.
Absence is modeled with:

- **Optionals** (`:Some[v]` / `:None`) for "this value might be absent."
- **Results** (`:Success[v]` / `:Error[e]`) for "this operation might fail."

Both are ordinary Tagged Lists. Chapter 12 covers optionals; Chapter 13 covers
results.

## Type predicates

There are no built-in `IsInt`, `IsString`, etc. Use `TypeOf` and compare:

```on1x
fn is_string(v) { TypeOf(v) == :String }
is_string("hi")    // true
is_string(42)      // false
```

For checking whether a value is a specific kind of Tagged List (e.g. whether it
is `:Some[...]` or `:Success[...]`), use `TagOf` (Chapter 10) or `match`
(Chapter 20).

Continue to Chapter 4 to learn about `:Unit` and `:Bool`.
