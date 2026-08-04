# Specifications for On1x Language
* Version: 1.0.0
* Author: Chubak Bidpaa <chubakbidpaa@riseup.net>

## Preface

On1x is an **embedding-oriented** language. It is minimal (and minimalist), lightweight, and interoperable with C and, through C, with any language that can bind to a C ABI. The canonical way to embed On1x is the On1x C library, hereby referred to as `libon1x`.

On1x is built around a small set of ideas that are worth stating up front:

- **Dynamic typing.** Types are inferred from values, à la Scheme: *values* carry their type, *variables* do not. A binding may refer to an integer now and a table later.
- **Tags.** Tags are interned symbols, comparable to symbols in later Scheme dialects (e.g. Racket) and in Ruby. A Tag is written with a leading colon, e.g. `:Red`.
- **Two aggregate structures.** All compound data is built from **Lists** and **Tables**. A Tag may be associated with a List to form a **Tagged List**, which makes sum types a syntactic reality rather than a convention.
- **Enumerations.** A List composed entirely of Tags can be read as an enumeration. A more convenient way to build one is with `Iota`, a relic from APL that Go also adopted. In On1x, `Iota` is a *value*, not a construct; it is used both to auto-number enumerations and to generate ranges. `Iota(n)` where `n` is non-integer or non-positive yields the value other languages call `nil`.
- **No nil.** On1x has no `nil`. Absence is modeled with the `?` hint, which marks a *value* as optional. Passing a value produces `Some`; passing nothing produces `None`. Under the hood these are Tagged Lists whose tag is `:Some` (carrying a Payload) or `:None`. Optionality is attached to **values**, not to **variables**.
- **Effects via `~`.** The `~` construct unravels the result of a side-effecting execution into an Error/Success dichotomy. The statement following `~` is handed this result and may do with it as it wishes. The result is a Tagged List tagged `:Success` or `:Error`, carrying an optional Payload (an `?` value). If nothing was produced, the Payload is `None`.

The sections that follow specify the language, its lexical and grammatical structure, the value model, and the `libon1x` interface.

## 1. Design Goals

1. **Embeddability first.** The language exists to be hosted inside a C program. Startup cost, memory footprint, and API surface are kept small.
2. **One obvious data model.** Lists and Tables cover aggregate data; Tags plus Tagged Lists cover variants and enumerations. There is deliberately nothing else.
3. **No null.** Absence and failure are values (`None`, `:Error[...]`), never a hole in the type system.
4. **Values carry meaning.** Because types travel with values, host code and On1x code agree on what a value *is* without a separate schema.
5. **Predictable interop.** Every On1x value has a stable C representation and a stable set of C constructors and accessors.

## 2. Notation and Conventions

Grammar is given in EBNF (Section 17):

- `"x"` is a literal terminal.
- `A B` is concatenation, `A | B` is alternation.
- `[A]` is optional, `{A}` is zero-or-more, `(A)` groups.
- Nonterminals are `PascalCase`.

Throughout, "the reference implementation" refers to `libon1x` as specified in Section 16. Where behavior is marked *implementation-defined*, a conforming implementation must document its choice.

## 3. Lexical Structure

### 3.1 Source Encoding
Source text is UTF-8. Identifiers are restricted to ASCII (Section 3.4); string and Tag contents may hold any UTF-8.

### 3.2 Comments
- Line comment: `//` to end of line.
- Block comment: `/* ... */`. Block comments do not nest.

### 3.3 Line Structure
A statement is terminated by a newline or an explicit `;`. Consecutive terminators are collapsed. A statement may be continued onto the next line if the line ends inside an open bracket `( [ { %{` or immediately after a binary operator.

### 3.4 Identifiers
```text
Ident = ("_" | Letter) { "_" | Letter | Digit }
Letter = "A".."Z" | "a".."z"
Digit  = "0".."9"
```

Convention (not enforced): value and function bindings are `snake_case` or `camelCase`; Tags are `PascalCase`.

### 3.5 Keywords
Reserved and unavailable as identifiers:
```text
let  fn   if    else  while  for  in
match  return  break  continue  enum
and  or   not   true  false
```

### 3.6 Literals
- Integer: `42`, `0xFF`, `0b1010`, `0o17`, `1_000_000`. An Int is a 64-bit two's-complement signed integer.
- Float: `3.14`, `1e-9`, `6.022e23`. A Float is an IEEE-754 binary64 double.
- String: `"..."` with escapes `\n \t \r \\ \" \0 \u{XXXX}`.
- Boolean: `true`, `false`.
- Unit: `()` — the single value of type `:Unit`, used where a statement or effect produces no meaningful result.
- Tag: `:Ident` or `:"arbitrary text"`.

### 3.7 Operators and Punctuation
```text
=  ==  !=  <  <=  >  >=
+  -  *  /  %
and  or  not
?  ~  ..  .
(  )  [  ]  {  }  %{
,  ;  :  =>  ->
```

## 4. The Value Model

### 4.1 Values Carry Types
Every value is self-describing. `TypeOf(v)` returns a Tag naming the value's type. The complete set of type Tags is:
```
:Unit  :Bool  :Int  :Float  :String  :Tag
:List  :Table  :Fn  :Iota
```

A Tagged List reports `:List` from `TypeOf`; use `TagOf` (Section 6.3) to read its constructor tag.

### 4.2 Nil-Free Design
There is no `nil`/`null`/`undefined`. Any operation that might not yield a value yields either an **optional** (`:Some`/`:None`, Section 8) or a **result** (`:Success`/`:Error`, Section 9). Reading a missing Table key, indexing out of bounds, and `Iota` of an invalid argument all produce `:None`, never a fault.

### 4.3 Equality and Identity
- Scalars (`:Unit :Bool :Int :Float :String :Tag`) compare by value. `:Int` and `:Float` are cross-comparable numerically: $1 == 1.0$ is `true`.
- Tags are interned; equality is pointer equality internally and value equality semantically.
- Lists and Tables compare structurally, element by element (Tables by key/value set).
- `:Fn` compares by identity only.

## 5. Scalar Values

| Type | Literal | Notes |
|------|---------|-------|
| `:Unit` | `()` | Sole inhabitant. Result of effectful statements with no value. |
| `:Bool` | `true`, `false` | Only Booleans are valid conditions; there is no truthiness. |
| `:Int` | `42` | 64-bit signed. Overflow wraps (two's complement). |
| `:Float` | `3.14` | binary64. |
| `:String` | `"hi"` | Immutable, UTF-8, indexable by byte offset via `Get`. |
| `:Tag` | `:Red` | Interned symbol. |

There is no implicit conversion between `:Int` and `:Float` in arithmetic other than promotion: an expression mixing the two promotes to `:Float`.

## 6. Compound Values

### 6.1 Lists
An ordered, heterogeneous, mutable sequence.
```on1x
let xs = [1, "two", :three, [4]]
Len(xs)        // 4
Get(xs, 0)     // :Some[1]
Get(xs, 99)    // :None
Push(xs, 5)    // xs is now [1, "two", :three, [4], 5]
```

Indexing is zero-based. `Get` returns an optional; there is no out-of-bounds fault.

### 6.2 Tables
An unordered map from any hashable value (scalars and Tags) to any value.
```on1x
let t = %{ "name" => "on1x", :version => [0, 1, 0], 1 => true }
Get(t, "name")    // :Some["on1x"]
Get(t, :missing)  // :None
Set(t, :year, 2026)
Keys(t)           // list of keys (unspecified order)
```

Lists and Tables are not valid keys (they are mutable and compare structurally). Attempting to use one as a key is a runtime error surfaced through `~`.

### 6.3 Tagged Lists (Sum Types)
Associating a Tag with a List produces a Tagged List. The Tag is the *constructor*; the List is the *payload*.
```on1x
:Point[3, 4]
:Circle[:Point[0, 0], 5]
:Leaf[]                 // empty payload
```

A Tagged List is still a `:List` for `TypeOf`. Two extra accessors apply:
```on1x
TagOf(:Point[3, 4])       // :Some[:Point]
TagOf([1, 2, 3])          // :None   (plain, untagged list)
PayloadOf(:Point[3, 4])   // [3, 4]
```

Sum types are just the set of constructors a program agrees to use, deconstructed with `match` (Section 14):
```on1x
fn area(shape) {
    match shape {
        :Circle[_, r]        => 3.14159 * r * r
        :Rect[w, h]          => w * h
        _                    => :Error[:UnknownShape]
    }
}
```

### 6.4 Enumerations
A List composed entirely of Tags reads as an enumeration:
```on1x
let direction = [:North, :East, :South, :West]
```

For numbered enumerations, use the `enum` block, where the bare value `Iota` auto-numbers members starting at $0$ (Section 7). An `enum` block evaluates to a Table mapping member Tags to their numbers:
```on1x
let Color = enum {
    Red   = Iota,   // 0
    Green = Iota,   // 1
    Blue  = Iota,   // 2
}
Get(Color, :Blue)   // :Some[2]
```

Members are implicitly Tags; `Red` in an `enum` block denotes the key `:Red`.

## 7. Iota

`Iota` is a built-in *value* of type `:Iota`. It has two roles.

**As an auto-numbering value (the Go/APL relic).** Inside an `enum` block, each textual occurrence of `Iota` evaluates to a counter that starts at $0$ and increments by $1$ per member. The counter resets at the start of each block.

**As a range generator (callable).** Applied to arguments it produces a List:

- `Iota(n)` → the List $[0, 1, \dots, n-1]$ for a positive integer `n`.
- `Iota(a, b)` → the List of integers $[a, a+1, \dots, b-1]$ (half-open).
- `Iota(a, b, s)` → values $a + k\cdot s$ for $k = 0, 1, 2, \dots$ while they remain within the half-open interval toward `b`. `s` may be negative for a descending range; $s = 0$ is invalid.

**The nil case.** `Iota(n)` where `n` is non-integer or non-positive (and no further arguments make it a valid range) evaluates to `:None`. Since On1x has no `nil`, this is how `Iota` expresses "no range":
```on1x
Iota(5)        // [0, 1, 2, 3, 4]
Iota(2, 6)     // [2, 3, 4, 5]
Iota(6, 2, -1) // [6, 5, 4, 3]
Iota(0)        // []
Iota(-3)       // :None
Iota(2.5)      // :None
```

## 8. Optionality

Optionality is a property of a value, expressed with `?`.

- `?expr` constructs an optional carrying a value: `:Some[expr]`.
- `?` with no operand is the empty optional: `:None`.
```on1x
let a = ?42     // :Some[42]
let b = ?       // :None
```

These are ordinary Tagged Lists, so they work with `match` and with the prelude helpers:
```on1x
IsSome(a)     // true
IsNone(b)     // true
Unwrap(a)     // 42     (runtime error via ~ if applied to :None)
UnwrapOr(b, 0)// 0
```

Because optionality rides on the value, a single binding can hold `:Some[...]` or `:None` at different times, and functions can accept "a value that might be absent" without any variable-level annotation:
```on1x
fn greet(name) {
    match name {
        :Some[n] => Print("Hello, " + n)
        :None    => Print("Hello, stranger")
    }
}
greet(?"Ada")   // Hello, Ada
greet(?)        // Hello, stranger
```

## 9. The `~` Construct (Effect Handling)

`~ expr` evaluates `expr` under **effect capture** and produces a result value:

- `:Success[payload]` if evaluation completed normally.
- `:Error[payload]` if evaluation raised (a runtime error, a host-signaled error, or an explicit raise).

The payload is an optional (`?` value): the produced value wrapped in `:Some[...]`, or `:None` if nothing was produced (e.g. an effect returning `:Unit`).

The result is bound to the reserved name `~` within the **single statement that follows**. That statement decides what to do with it.
```on1x
read_file("config.on1x") ~ match {
    :Success[?text] => load_config(text)
    :Success[?]     => Print("empty file")
    :Error[?err]    => Print("failed: " + err)
    :Error[?]       => Print("failed (no detail)")
}
```

Notes:

- `~` captures only the effect of its immediate operand. Errors raised inside the *following* statement are not captured by that same `~`.
- `~` may be nested; the innermost `~` binds the name `~` for its follower.
- Outside the follower statement, referencing `~` is a compile-time error.
- A function may signal failure to a caller's `~` with `return :Error[?reason]` by convention, or by raising through a host function.

## 10. Variables and Bindings

`let` introduces a binding in the current scope; plain `=` reassigns an existing binding.
```on1x
let x = 10
x = x + 1        // reassignment
let x = "shadow" // new binding, shadows the outer x in this scope
```

Scoping is lexical. A block `{ ... }` (Section 13) introduces a scope. Functions close over their defining environment.

There is no uninitialized state: `let x` without an initializer is a syntax error. Use `let x = ?` for a deliberately-absent value.

## 11. Functions

Functions are first-class values of type `:Fn`.
```on1x
fn add(a, b) { a + b }             // named

let mul = fn(a, b) { a * b }       // anonymous, bound

fn clamp(x, lo, hi) {
    if x < lo { return lo }
    if x > hi { return hi }
    x                               // last expression is the value
}
```

- The value of a block is its last expression; `return` exits early with a value.
- A function with no meaningful result yields `()`.
- Parameters are positional. A trailing parameter written `rest..` collects remaining arguments into a List:
```on1x
fn sum(rest..) {
    let total = 0
    for x in rest { total = total + x }
    total
}
sum(1, 2, 3, 4)   // 10
```

- On1x has no separate optionality on parameters; pass `?value`/`?` to express presence/absence per Section 8.

## 12. Expressions and Operators

Precedence, highest to lowest:

| Level | Operators | Assoc |
|------:|-----------|-------|
| 1 | `f(...)`  `x[i]`  `x.field`  (call, index, field) | left |
| 2 | `?` (prefix) `~` (prefix) `not` `-` (unary) | right |
| 3 | `*`  `/`  `%` | left |
| 4 | `+`  `-` | left |
| 5 | `..` (range shorthand) | none |
| 6 | `<`  `<=`  `>`  `>=` | left |
| 7 | `==`  `!=` | left |
| 8 | `and` | left |
| 9 | `or` | left |

- `and`/`or` short-circuit and require `:Bool` operands.
- `+` on `:String` concatenates; on numbers it adds; it is not defined across mixed String/number operands (raises, catchable by `~`).
- `a .. b` is sugar for `Iota(a, b)`.
- `x.field` is sugar for `Get(x, :field)` on a Table and requires the key to be present, otherwise it raises (use `Get` for the optional-returning form).

## 13. Statements and Control Flow
```text
Block      = "{" { Statement } "}"

if C { ... } else { ... }
while C { ... }
for item in iterable { ... }   // iterable: List, Table (yields keys), or range
match value { arms }           // Section 14
break
continue
return [Expr]
```

- Conditions must be `:Bool`. There is no truthiness and no implicit optional-to-bool coercion.
- `for x in table` iterates keys; use `Get(table, x)` for values, or `for k in Keys(t)`.
- `if`/`match` are expressions and yield a value; `while`/`for` yield `()`.

## 14. Pattern Matching

`match` deconstructs values by shape. Arms are tried top to bottom; the first match wins.
```text
Match = "match" Expr "{" { Pattern "=>" (Expr | Block) } "}"
```

Patterns:

- Literal: `1`, `"x"`, `true`, `:Red`, `()`
- Wildcard: `_`
- Binding: `name` (binds the matched value)
- List: `[p1, p2, ..rest]` (`..rest` binds the tail List)
- Tagged List: `:Tag[p1, p2, ...]`, or `:Tag` for empty payload
- Optional and result patterns are just Tagged-List patterns: `:Some[p]`, `:None`, `:Success[p]`, `:Error[p]`
```on1x
fn describe(v) {
    match v {
        :None            => "nothing"
        :Some[0]         => "zero"
        :Some[n]         => "value " + n
        [x, ..rest]      => "list starting with " + x
        _                => "something else"
    }
}
```

A `match` that no arm covers raises (catchable by `~`). Add a trailing `_` arm to make matching total.

## 15. Modules and the Embedding Model

On1x is embedded, not standalone-first. A "program" is a sequence of top-level statements evaluated in an environment provided by the host.

- **Environment.** The host creates a state (`On1xCState`) that owns a global environment. Top-level `let` bindings live there.
- **Native functions.** The host registers C functions as `:Fn` values under names in the global environment. From On1x they are indistinguishable from On1x functions.
- **Loading.** A host may evaluate multiple source chunks against the same state; later chunks see earlier bindings.
- **No implicit I/O.** The core language has no file, network, or clock access. Such capabilities exist only if the host registers them. This keeps embedded instances sandboxed by default.

There is no separate module keyword in 0.1.0. Namespacing is done with Tables of functions:
```on1x
let math = %{
    :square => fn(x) { x * x },
    :cube   => fn(x) { x * x * x },
}
Get(math, :square)   // :Some[<fn>]
```

## 16. `libon1x` — C Library Interface

`libon1x` exposes a stack-based, reentrant C API. A `On1xCState` is a single logical interpreter instance; it is not thread-safe, but distinct states are independent and may run on separate threads.

Memory is managed by a tracing garbage collector inside the state. Values handed to C are protected from collection while referenced through the value stack or through explicit *rooted handles*.

### 16.1 Core Types

```c
typedef struct On1xState On1xState;

/* Every On1x value maps to exactly one of these tags. */
typedef enum {
    ON1X_UNIT, ON1X_BOOL, ON1X_INT, ON1X_FLOAT,
    ON1X_STRING, ON1X_TAG, ON1X_LIST, ON1X_TABLE,
    ON1X_FN, ON1X_IOTA
} On1xType;

/* Opaque, GC-managed value handle bound to a state's stack slot. */
typedef uintptr_t On1xRef;   /* index into the value stack; negative = from top */

/* Native function signature. Arguments occupy the stack; the callee
   pushes its single result (or an error) before returning. */
typedef On1xStatus (*On1xCFn)(On1xState *S, int argc);

/* Result of any operation that can fail at the C boundary. */
typedef enum { ON1X_OK, ON1X_ERR } On1xStatus;
```

### 16.2 Lifecycle

```c
On1xCState *On1xCOpen(void);
void        On1xCClose(On1xCState *S);

/* Compile and evaluate a chunk in S's global environment.
   On failure, an :Error value is left on the stack and ON1X_ERR returned. */
On1xCStatus On1xCEval(On1xCState *S, const char *src, size_t len,
                      const char *chunkname);
```

### 16.3 Stack and Value Construction

The stack grows as you push. Indices are 1-based from the bottom, or negative from the top (`-1` is the topmost).

```c
int  On1xCTop(On1xCState *S);              /* number of stack slots */
void On1xCPop(On1xCState *S, int n);
void On1xCDup(On1xCState *S, int idx);

On1xCType On1xCType(On1xCState *S, int idx);

/* Push scalars */
void On1xCPush_unit (On1xCState *S);
void On1xCPush_bool (On1xCState *S, int b);
void On1xCPush_int  (On1xCState *S, int64_t n);
void On1xCPush_float(On1xCState *S, double d);
void On1xCPush_str  (On1xCState *S, const char *s, size_t len);
void On1xCPush_tag  (On1xCState *S, const char *name, size_t len);

/* Aggregates: push an empty one, then fill it */
void On1xCNew_list (On1xCState *S);
void On1xCList_push(On1xCState *S, int listidx);      /* moves top into list */

void On1xCNew_table(On1xCState *S);
void On1xCTable_set(On1xCState *S, int tblidx);       /* consumes key,value on top */

/* Tagged List: build a list, then tag it */
void On1xCTag_list (On1xCState *S, const char *tag, size_t len, int listidx);
```

### 16.4 Reading Values

```c
int64_t On1xCAs_int  (On1xCState *S, int idx);
double  On1xCAs_float(On1xCState *S, int idx);
int     On1xCAs_bool (On1xCState *S, int idx);
const char *On1xCAs_str(On1xCState *S, int idx, size_t *len);

int     On1xCLen     (On1xCState *S, int idx);        /* List/Table/String */
int     On1xCList_get(On1xCState *S, int idx, int i); /* pushes elem; 0 if OOB */
int     On1xCTable_get(On1xCState *S, int idx);       /* key on top -> value or :None */

/* Tagged List introspection */
int     On1xCTag_of  (On1xCState *S, int idx, char *buf, size_t bufsz); /* -1 if untagged */
void    On1xCPayload_of(On1xCState *S, int idx);      /* pushes the payload list */
```

### 16.5 Registering Native Functions and Calling

```c
void On1xCRegister(On1xCState *S, const char *name, On1xCCFn fn);

/* Call the value at function-idx with argc args pushed above it.
   Semantics mirror `~`: on success ON1X_OK and the result is on top;
   on failure ON1X_ERR and an :Error[...] value is on top. */
On1xCStatus On1xCCall(On1xCState *S, int fnidx, int argc);
```

### 16.6 Error Model

The C API mirrors the language's `~`. A native function reports failure by pushing an `:Error[?payload]` value and returning `ON1X_ERR`; it reports success by pushing exactly one result and returning `ON1X_OK`. When On1x code wraps such a call in `~`, it receives the same `:Success`/`:Error` Tagged List described in Section 9.

```c
/* Convenience: build :Error[:Some[<string>]] and signal failure. */
On1xCStatus On1xCError(On1xCState *S, const char *msg);
```

### 16.7 Minimal Embedding Example

```c
#include "on1x.h"

static On1xCStatus l_hypot(On1xCState *S, int argc) {
    if (argc != 2) return On1xCError(S, "hypot expects 2 args");
    double a = On1xCAs_float(S, 1);
    double b = On1xCAs_float(S, 2);
    On1xCPush_float(S, sqrt(a*a + b*b));   /* result on top */
    return ON1X_OK;
}

int main(void) {
    On1xCState *S = On1xCOpen();
    On1xCRegister(S, "Hypot", l_hypot);

    const char *src =
        "~ Hypot(3, 4)\n"
        "match ~ {\n"
        "  :Success[?d] => Print(\"len = \" + d)\n"
        "  :Error[?e]   => Print(\"oops: \" + e)\n"
        "}\n";

    On1xCEval(S, src, strlen(src), "main");
    On1xCClose(S);
    return 0;
}
```

## 17. Grammar (EBNF)
```ebnf
Program     = { Statement } .

Statement   = Let
            | Assign
            | ExprStmt
            | Effect
            | If | While | For | Match
            | "return" [ Expr ]
            | "break" | "continue"
            | Block
            , Terminator .

Terminator  = ";" | Newline | EOF .

Let         = "let" Ident "=" Expr .
Assign      = LValue "=" Expr .
LValue      = Ident | Index | Field .
ExprStmt    = Expr .

Effect      = "~" Expr Terminator Statement .   (* '~' visible in Statement *)

Block       = "{" { Statement } "}" .

If          = "if" Expr Block [ "else" ( If | Block ) ] .
While       = "while" Expr Block .
For         = "for" Ident "in" Expr Block .
Match       = "match" Expr "{" { Arm } "}" .
Arm         = Pattern "=>" ( Expr | Block ) [ Terminator ] .

FnDecl      = "fn" Ident "(" [ Params ] ")" Block .
FnLit       = "fn" "(" [ Params ] ")" Block .
Params      = Ident { "," Ident } [ "," Ident ".." ] .

Enum        = "enum" "{" { Ident "=" Expr [ "," ] } "}" .

Expr        = OrExpr .
OrExpr      = AndExpr { "or" AndExpr } .
AndExpr     = EqExpr { "and" EqExpr } .
EqExpr      = RelExpr { ("==" | "!=") RelExpr } .
RelExpr     = RangeExpr { ("<" | "<=" | ">" | ">=") RangeExpr } .
RangeExpr   = AddExpr [ ".." AddExpr ] .
AddExpr     = MulExpr { ("+" | "-") MulExpr } .
MulExpr     = Unary { ("*" | "/" | "%") Unary } .
Unary       = ("not" | "-") Unary
            | "?" [ Postfix ]                 (* '?' alone = None *)
            | "~" Unary                       (* effect operand in expr position *)
            | Postfix .
Postfix     = Primary { Call | Index | Field } .
Call        = "(" [ Args ] ")" .
Index       = "[" Expr "]" .
Field       = "." Ident .
Args        = Expr { "," Expr } .

Primary     = Literal
            | Ident
            | "~"                              (* reads current effect result *)
            | "(" [ Expr ] ")"                 (* group; "()" = Unit *)
            | ListLit
            | TableLit
            | TaggedList
            | FnLit
            | FnDecl
            | Enum .

ListLit     = "[" [ Expr { "," Expr } ] "]" .
TableLit    = "%{" [ Entry { "," Entry } ] "}" .
Entry       = Expr "=>" Expr .
TaggedList  = Tag "[" [ Expr { "," Expr } ] "]" .

Literal     = Int | Float | String | Bool | Unit | Tag .
Bool        = "true" | "false" .
Unit        = "(" ")" .
Tag         = ":" Ident | ":" String .

Pattern     = "_"
            | Literal
            | Ident
            | "[" [ Pattern { "," Pattern } ] [ ".." Ident ] "]"
            | Tag [ "[" [ Pattern { "," Pattern } ] "]" ] .
```

## 18. Appendix

### 18.1 Reserved Tags
The following Tags carry defined meaning:
```text
Types:    :Unit :Bool :Int :Float :String :Tag :List :Table :Fn :Iota
Optional: :Some :None
Result:   :Success :Error
```

Programs may define any other Tags freely.

### 18.2 Prelude
Always available (host may extend or restrict):
```on1x
TypeOf(v)          TagOf(v)          PayloadOf(v)
Len(x)             Get(x, k)         Set(x, k, v)
Push(list, v)      Pop(list)         Keys(t)          Values(t)
Iota                                  (see Section 7)
IsSome(o)          IsNone(o)         Unwrap(o)        UnwrapOr(o, d)
IsSuccess(r)       IsError(r)
Print(x)                              (present only if the host registers it)
```

### 18.3 Worked Example: A Small Interpreter State
```on1x
// A tiny sum type and its use, end to end.
let Shape = %{}   // namespace of constructors, by convention

fn circle(r)   { :Circle[r] }
fn rect(w, h)  { :Rect[w, h] }

fn area(s) {
    match s {
        :Circle[r] => 3.14159 * r * r
        :Rect[w, h] => w * h
        _ => :Error[?"not a shape"]
    }
}

let shapes = [circle(2), rect(3, 4), :Triangle[3, 4, 5]]

for i in Iota(Len(shapes)) {
    ~ area(Get(shapes, i))   // area may raise via the fallthrough error
    match ~ {
        :Success[?a] => Print("area = " + a)
        :Error[?e]   => Print("skip: " + e)
        :Error[?]    => Print("skip (unknown)")
    }
}
```

This exercises Tagged Lists as sum types, optionals as payloads, `Iota` as a range generator, and `~` as the effect boundary, which together are the whole of On1x's data and control story.

