# 5. Integers

`:Int` values are **64-bit signed two's-complement integers**. They can
represent values from $-2^{63}$ to $2^{63}-1$.

## Literal forms

On1x supports decimal, hexadecimal, binary, and octal integer literals, with
optional underscore separators for readability:

```on1x
42               // decimal
0xFF             // hexadecimal (255)
0b1010           // binary (10)
0o17             // octal (15)
1_000_000        // decimal with separators (1000000)
0xFF_FF          // hex with separators (65535)
-42              // negative
```

## Arithmetic

The standard arithmetic operators are available:

```on1x
1 + 2            // 3
5 - 3            // 2
4 * 7            // 28
10 / 3           // 3   (integer division truncates toward zero)
10 % 3           // 1   (remainder)
-5               // unary negation
```

Division by zero **raises** an error (catchable with `~`, see Chapter 14):

```on1x
1 / 0            // raises
```

The standard library's `Math.DivMod(a, b)` returns `:None` for a zero divisor
instead of raising (Chapter 29).

## Wrapping overflow

Integer arithmetic wraps on overflow (two's complement):

```on1x
let max_int = 9223372036854775807   // 2^63 - 1
max_int + 1                         // -9223372036854775808  (wraps)
```

This is deliberate: it matches the behavior of C's signed 64-bit integers and
avoids the overhead of checked arithmetic on every operation.

## Integer/Float promotion

When an `:Int` and a `:Float` appear as operands of an arithmetic operator
other than `%`, the `:Int` is promoted to `:Float` and the result is `:Float`:

```on1x
1 + 2.0          // 3.0   (:Float)
3.5 * 2          // 7.0   (:Float)
10 / 3.0         // 3.333...  (:Float)
```

The modulo operator `%` requires both operands to be `:Int`; mixed operands
raise.

## Comparison

Integers compare numerically with each other and with floats:

```on1x
1 == 1           // true
1 == 1.0         // true  (:Int vs :Float — numerical equality)
2 < 5            // true
-3 > -10         // true
```

## Standard library

The `Math` module (Chapter 29) provides:

- `Math.Abs(n)` — absolute value.
- `Math.Min(a, b)`, `Math.Max(a, b)` — minimum and maximum.
- `Math.Sign(n)` — -1, 0, or 1.
- `Math.DivMod(a, b)` — returns `:Some[[quotient, remainder]]` or `:None` on
  division by zero.

The `Bit` module (Chapter 29) provides bitwise operations:

- `Bit.And(a, b)`, `Bit.Or(a, b)`, `Bit.Xor(a, b)`, `Bit.Not(a)`.
- `Bit.Shl(n, bits)`, `Bit.Sar(n, bits)`, `Bit.Shr(n, bits)` — shift
  operations. Shift counts at or above 64 return `:None`.

## Quick reference

| Operation | Expression | Result |
|-----------|-----------|--------|
| Addition | `a + b` | `:Int` or `:Float` |
| Subtraction | `a - b` | `:Int` or `:Float` |
| Multiplication | `a * b` | `:Int` or `:Float` |
| Division | `a / b` | `:Int` or `:Float`; raises on /0 |
| Modulo | `a % b` | `:Int`; raises on /0 |
| Negation | `-a` | `:Int` or `:Float` |

Continue to Chapter 6 to learn about floats.
