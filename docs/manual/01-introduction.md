# 1. Introduction

On1x is an **embedding-oriented**, dynamically-typed programming language
designed to live inside C and C++ programs. It is minimal, predictable, and
free of surprises — a language you can carry in your head.

## Why On1x?

- **Embeddability first.** On1x is not a standalone platform. It ships as
  `libon1x`, a single C library that you link into your application. Startup is
  cheap, the memory footprint is small, and the API surface is compact.
- **One obvious data model.** All compound data is built from just **Lists**
  and **Tables**. Tags plus Tagged Lists cover sum types, enumerations, and
  variants. There is nothing else to learn.
- **No nil.** Absence is a value (`:None`), not a hole in the type system.
  Every operation that can fail to produce a value returns an **optional**
  (`:Some[v]` or `:None`). Every operation that can fail for external reasons
  returns a **result** (`:Success[v]` or `:Error[e]`), catchable with `~`.
- **Values carry types.** Types travel with values, not with variables. A
  binding can hold an integer now and a table later. Host code and On1x code
  agree on what a value *is* without a separate schema.
- **No implicit I/O.** The core language has no file, network, or clock access.
  Capabilities must be explicitly granted by the host. Pure code stays pure.

## Design philosophy

On1x is deliberately small. It does not have classes, interfaces, generics,
macros, async/await, or a package manager. Every feature that *is* present was
chosen because it composes with the rest:

| Concept | Mechanism |
|---------|-----------|
| Variants / sum types | Tagged Lists (`:Circle[0, 0, 5]`) |
| Enums with numbers | `enum { Red = Iota, Green = Iota, Blue = Iota }` |
| Absence | `:Some[v]` / `:None` (optional values) |
| Errors | `:Success[v]` / `:Error[e]` + `~` effect capture |
| Namespaces | Tables of functions (`%{ :fn1 => ..., :fn2 => ... }`) |
| Iteration | `Iota` ranges, `for` loops, `Iter` module |
| Interop | `libon1x` C API, `Dl` FFI module |

## What this manual covers

- **Chapters 2–3:** Getting started, the value model.
- **Chapters 4–11:** Every scalar and compound type.
- **Chapters 12–16:** Optionals, results, effects, bindings, functions.
- **Chapters 17–21:** Expressions, operators, control flow, pattern matching,
  `Iota`, and enumerations.
- **Chapters 22–23:** Lexical structure and modules.
- **Chapters 24–25:** Embedding On1x in C and the C++ facade.
- **Chapters 26–28:** The standard library: pure modules, capability-gated
  modules, and optional tiers.
- **Chapters 29–30:** Errors, debugging, and formal grammar reference.

Each chapter assumes you have read the preceding ones. Code examples are
complete and runnable.

## A first taste

```on1x
fn greet(name) {
    match name {
        :Some[n] => "Hello, " + n
        :None    => "Hello, stranger"
    }
}

greet(?"Ada")   // "Hello, Ada"
greet(?)        // "Hello, stranger"
```

This snippet uses **optionals** (`?`), **pattern matching** (`match`), **Tagged
List constructors** (`:Some`, `:None`), **string concatenation** (`+`), and
**functions**. These five ideas — plus lists, tables, and `~` — are the whole
of On1x.

Continue to Chapter 2 to set up On1x and write your first program.
