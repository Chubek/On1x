# 6. Floats

`:Float` values are **IEEE 754 binary64** double-precision floating-point
numbers — the same as C's `double`.

## Literal forms

```on1x
3.14             // standard
1e-9             // scientific notation (0.000000001)
6.022e23         // Avogadro's number
-2.5             // negative
0.0              // zero
1_000.5          // with underscore separator
```

A decimal point (`.`) or exponent (`e` / `E`) is required. Plain `1` is always
an `:Int`.

## Arithmetic

Floats support the same operators as integers:

```on1x
3.14 + 2.0       // 5.14
5.5 - 1.2        // 4.3
2.5 * 4.0        // 10.0
10.0 / 3.0       // 3.3333333333333335
-7.5             // negation
```

Division by zero with floats does **not** raise — it produces IEEE 754
semantics (`Infinity` or `NaN`):

```on1x
1.0 / 0.0        // Infinity
0.0 / 0.0        // NaN
```

Mixed `:Int`/`:Float` arithmetic promotes the integer to float:

```on1x
1 + 2.0          // 3.0  (:Float)
```

The modulo operator `%` does **not** accept floats — both operands must be
`:Int`.

## Comparison

Floats compare numerically with each other and with integers:

```on1x
1.0 == 1         // true
3.14 > 0         // true
1.0 == 1.0000000000000001   // might be false (floating-point imprecision)
```

As with all floating-point, exact equality comparisons should be used
judiciously. For approximate comparison, write a tolerance check:

```on1x
fn approx(a, b, tol) {
    Math.Abs(a - b) < tol
}
```

## Special values

On1x exposes IEEE 754 special values through the `Math` module:

- `Math.Inf` — positive infinity.
- `Math.NaN` — not-a-number.

There are no dedicated `:Infinity` or `:NaN` types — these are ordinary
`:Float` values with special bit patterns.

## Standard library

The `Math` module (Chapter 29) provides the full set of mathematical functions:

- `Math.Sqrt(x)`, `Math.Cbrt(x)` — square and cube root.
- `Math.Pow(base, exp)` — exponentiation.
- `Math.Exp(x)`, `Math.Log(x)`, `Math.Log2(x)`, `Math.Log10(x)`.
- `Math.Sin`, `Math.Cos`, `Math.Tan` and their inverses.
- `Math.Floor(x)`, `Math.Ceil(x)`, `Math.Round(x)`, `Math.Trunc(x)`.
- `Math.Abs(x)` — works on both `:Int` and `:Float`.

Domain errors (e.g. `Math.Sqrt(-1)`, `Math.Log(0)`) return `:None`, not `NaN`.

## Quick reference

| Value | Literal | Type |
|-------|---------|------|
| 3.14 | `3.14` | `:Float` |
| Scientific | `1.5e-3` | `:Float` |
| Infinity | `1.0 / 0.0` or `Math.Inf` | `:Float` |
| NaN | `0.0 / 0.0` or `Math.NaN` | `:Float` |

Continue to Chapter 7 to learn about strings.
