# 4. Unit and Booleans

## `:Unit` — the empty value

`:Unit` has exactly one inhabitant: the literal `()`. It is used where a
statement or expression produces no meaningful result:

```on1x
let x = ()          // Unit value

fn noop() { }        // returns () implicitly

let y = while true { break }   // while always yields ()

TypeOf(())           // :Unit
```

Unit is On1x's equivalent of `void` in C or `nil` in Go — a signal that
"nothing interesting came back." Unlike `nil`, it is a distinct, real value
with its own type.

**Common sources of Unit:**

- The value of a `while` or `for` loop.
- The value of a `break` or `continue` statement.
- A function whose body ends without a trailing expression.
- Assignments, `Set`, `Push`, and other mutating operations.

## `:Bool` — truth values

`:Bool` has exactly two values: `true` and `false`.

```on1x
let a = true
let b = false
TypeOf(a)    // :Bool
TypeOf(b)    // :Bool
```

### No truthiness

Conditions in `if`, `while`, and `and`/`or` **must be `:Bool`**. There is no
implicit conversion from other types:

```on1x
if 1 { ... }         // ERROR — 1 is :Int, not :Bool
if "hello" { ... }   // ERROR — "hello" is :String, not :Bool
if [] { ... }        // ERROR — [] is :List, not :Bool
```

This eliminates an entire class of bugs. To test for an empty list, write the
check explicitly:

```on1x
if Len(xs) == 0 { ... }
```

To test whether an optional is present:

```on1x
if IsSome(opt) { ... }        // IsSome returns :Bool
// or use match:
match opt {
    :Some[v] => ...
    :None    => ...
}
```

### Boolean operators

The logical operators `and`, `or`, and `not` work with `:Bool`:

```on1x
true and false    // false
true or false     // true
not true          // false
not (x == 0)      // true when x != 0
```

`and` and `or` short-circuit:

```on1x
false and (1/0)    // false — the division is never evaluated
true or (1/0)      // true — the division is never evaluated
```

### Comparison operators produce Bool

```on1x
1 == 2             // false
"a" != "b"         // true
3.14 > 0           // true
```

All comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`) return `:Bool`.

Continue to Chapter 5 to learn about integers.
