# 2. Getting Started

## Building On1x

On1x requires a C++20 compiler and CMake 3.20 or later. Clone the repository
and build:

```sh
git clone https://github.com/on1x-lang/on1x.git
cd on1x
cmake -S . -B build -DON1X_BUILD_TESTS=ON
cmake --build build --parallel
```

This produces:

- `build/src/libon1x.a` — the static library for embedding.
- `build/cli/on1x` — the command-line interpreter and REPL.

### Build options

| Option | Default | Effect |
|--------|---------|--------|
| `ON1X_BUILD_TESTS` | ON | Build the test suite |
| `ON1X_BUILD_CLI` | ON | Build the CLI and REPL |
| `ON1X_STDLIB_PURE_ONLY` | OFF | Compile out all I/O-capable modules |
| `ON1X_ENABLE_FFI` | OFF | Enable the `Dl` dynamic FFI module |
| `ON1X_ENABLE_JIT` | OFF | Enable the JIT backend |
| `ON1X_ENABLE_SEXP` | OFF | Enable the `Sexp` module |

For a minimal, pure-only embedding:

```sh
cmake -S . -B build-pure -DON1X_STDLIB_PURE_ONLY=ON
```

## The CLI

The command-line tool `on1x` provides three modes:

```sh
# Run a file
on1x -f path/to/script.on1x

# Evaluate a single expression
on1x -e '1 + 2'

# Start the interactive REPL
on1x
```

### REPL

The REPL (Read-Eval-Print Loop) accepts On1x expressions and statements. It
uses line-continuation heuristics: if a line ends inside an open bracket or
after a binary operator, the REPL prompts for more input.

```
on1x> let x = 10
on1x> x * 2
20
on1x> fn greet(name) {
....     "Hello, " + name
.... }
on1x> greet("world")
"Hello, world"
on1x> :quit
```

Meta-commands available in the REPL:

- `:quit` or `:q` — exit.
- `:help` or `:h` — show help.

### Running with capabilities

By default the CLI grants all capabilities (`IO`, `FS`, `ENV`, `TIME`,
`CLOCK`). For a sandboxed session without I/O:

```sh
on1x --pure -e '1 + 2'
```

## Your first script

Create a file `hello.on1x`:

```on1x
let name = "On1x"
let greeting = "Hello, " + name
greeting   // the last expression is the result
```

Run it:

```sh
on1x -f hello.on1x
# prints: "Hello, On1x"
```

## Embedding in C

A minimal C program that embeds On1x:

```c
#include <on1x/on1x.h>
#include <stdio.h>

int main(void) {
    On1x_State* S = on1x_open();
    on1x_open_std(S);
    on1x_grant(S, ON1X_CAP_IO);
    on1x_open_io(S);

    const char* src = "Io.Print(\"Hello from C!\")";
    on1x_eval(S, src, strlen(src), "inline");

    on1x_close(S);
    return 0;
}
```

Compile and link:

```sh
cc -o hello_embed hello_embed.c -lon1x -lgc -lm
```

Chapters 29–32 cover the C API in depth.

Continue to Chapter 3 to learn about On1x's value model.
