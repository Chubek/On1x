# On1x Road To Completion

Last updated: August 6, 2026 (Session 2 — parser fix).

This document is the active handoff and execution map for completing On1x. It
records what the tree actually supports, what remains against the manifests,
and the dependency gates that determine the next safe unit of work.

## Session History

### 2026-08-06 Session 2 (afternoon)

**Parser bug fixed.** `src/syntax/parser.cpp`: `parse_block` now saves and
restores `nesting_depth_` to 0 during block body parsing. Previously, when a
function literal appeared as a call argument (e.g. `Res.AndThen(~, fn(x) { ~
(x * 2)
~ })`), the elevated `nesting_depth_` from the call's argument list
leaked into the function body block. Inside the block, `parse_expression()` for
`~ (x * 2)` would call `peek_binary_operator()` → `skip_trivia(true)` after
parsing the grouped expression, consuming the `
` statement terminator. This
caused `parse_terminated_statement` to emit "expected statement terminator"
because the cursor landed on `~` (the next statement) instead of a `
`.

The fix ensures block-internal statement boundaries are always parsed with
`nesting_depth_==0`, while nested lists/tables/delimited expressions still
manage their own depth and correctly allow multiline continuations within
expressions.

**Debug output cleaned:** Removed temporary `std::fprintf` diagnostics from
`src/stdlib/res/res.cpp` (AndThen debug) and `src/syntax/parser.cpp`.
Remaining diagnostics in `src/api/api_state.cpp` and `src/vm/interpreter.cpp`
should be cleaned in the next session.

**Result:** 19/19 tests pass (was 18/19).

### 2026-08-06 Session 1 (morning)

Initial stdlib module implementations added: Cmp, Math, Bit, Tag, Str, List,
Table, Opt, Res. Tests for all modules created. 18/19 tests passing; Res
AndThen/OrElse tests failing due to parser bug (now fixed).

Before editing, read in this order:

1. `AGENTS.md`
2. `docs/SPECS.md`
3. `manifests/on1x-sources.yaml`
4. `manifests/on1x-stdlib.yaml`
5. this roadmap

The specification controls behavior. The manifests control required paths,
exports, modules, and tests. A file existing is not evidence that its subsystem
is complete; behavior and tests are the completion criteria.

## Non-Negotiable Rules

- Do not edit `third_party/`. Integrate or wrap dependencies from project code.
- Do not change the NaN-boxed `Value` layout, GC strategy, public C ABI, or
  specification without approval.
- Public headers remain C11-compatible and never include private `src/` files.
- No exception crosses an `on1x_*` entry point or native module boundary.
- Root every GC pointer held across allocation. Never store GC pointers in
  ordinary malloc-backed STL containers.
- Tags compare by interned identity. Lists and Tables are never valid Table
  keys.
- There is no nil sentinel. Expected absence is `:Some[v]` / `:None`.
- Conditions accept only `:Bool`; no truthiness may enter the runtime.
- Pure code performs no implicit I/O or other capability-bearing syscall.
- Do not start JIT/AOT until the interpreter is language-complete and parity
  tested.
- Do not add empty manifest files merely to improve inventory counts. Add a
  manifest file when its behavior can be implemented and tested.

## Settled Decisions

- `Iota(0)` yields an empty List. `Iota(-3)` and non-Int one-argument calls
  yield `:None`.
- Two- and three-argument Iota ranges are half-open as specified in §7.
- Ints are observable signed 64-bit two's-complement values with wrapping
  arithmetic. Values outside the packed-immediate range are boxed.
- Arithmetic division by zero raises; `Math.DivMod` returns `:None` for a zero
  divisor.
- Tables are unordered. Core rendering and tests must not impose iteration
  order.
- Tags may be interned from arbitrary valid UTF-8 strings.
- Core `Get` on Strings is byte-indexed. UTF-8-sensitive access must be named
  explicitly, such as `UGet`.
- `Res` is the only result-helper module.
- The public, feature-gated dyncall FFI is approved.

## Current Verified Baseline

Default build (19/19 passing):

```sh
cmake -S . -B /tmp/on1x-api-build \
  -DON1X_BUILD_TESTS=ON -Dbuild_tests=OFF -Denable_docs=OFF \
  -Dinstall_headers=OFF
cmake --build /tmp/on1x-api-build --parallel 2
ctest --test-dir /tmp/on1x-api-build --output-on-failure
```

Expected result: **19/19 passing**:

- `on1x_util_tests`
- `on1x_core_tests`
- `on1x_syntax_tests`
- `on1x_sema_tests`
- `on1x_ir_tests`
- `on1x_api_tests`
- `on1x_public_headers_c11`
- `on1x_public_headers_cpp20`
- `on1x_stdlib_stack_diag`
- `on1x_stdlib_math_domain`
- `on1x_stdlib_str_utf8`
- `on1x_stdlib_cmp_agreement`
- `on1x_stdlib_arity_errors`
- `on1x_stdlib_tag`
- `on1x_stdlib_bit`
- `on1x_stdlib_list`
- `on1x_stdlib_table`
- `on1x_stdlib_opt`
- `on1x_stdlib_res`

FFI build:

```sh
cmake -S . -B /tmp/on1x-ffi-build \
  -DON1X_BUILD_TESTS=ON -DON1X_ENABLE_FFI=ON \
  -Dbuild_tests=OFF -Denable_docs=OFF -Dinstall_headers=OFF
cmake --build /tmp/on1x-ffi-build --parallel 2
ctest --test-dir /tmp/on1x-ffi-build --output-on-failure
```

Expected result: **9/9 passing**, adding `on1x_ffi_tests`.

Vendored dparser, dyncall, and dynalo emit CMake compatibility warnings. They
are third-party warnings and must not be fixed by patching vendored code.

The `ON1X_STDLIB_PURE_ONLY`, `ON1X_ENABLE_JIT`, `ON1X_ENABLE_SEXP`, and
`ON1X_STDLIB_DOCS` options are configured and compile successfully in the
pure-only/no-optional-subsystems matrix. The matching test run currently has
the same **8/8** result as the default build.

## Implemented Capability Map

### Stdlib (pure modules)

- `Cmp`: Eq, Neq, Lt, Gt, Lte, Gte, Hash, Cmp (three-way ordering).
- `Math`: Abs, Sqrt, Pow, Log, Sin, Cos, Tan, Floor, Ceil, Round, Min, Max,
  Clamp, DivMod, IsInt, IsFinite, IsNaN, ToInt, ToFloat.
- `Bit`: And, Or, Xor, Not, Shl, Shr, PopCount, Clz, Ctz.
- `Tag`: New, Name, IsTag, AllTags.
- `Str`: Len, ByteLen, Bytes, Find, StartsWith, EndsWith, Upper, Lower, Trim,
  Split, Join, Replace, Slice, At, Repr.
- `List`: Len, Get, First, Last, Rest, Push, Pop, Append, Concat, Map, Filter,
  Fold, Find, Any, All, Sort, Unique, Reverse, Slice, Repr.
- `Table`: Keys, Values, Entries, Get, Has, Set, Remove, Merge, Map, Filter,
  FromEntries, Invert, Repr.
- `Opt`: IsSome, IsNone, Unwrap, UnwrapOr, Map, AndThen, OrElse.
- `Res`: IsSuccess, IsError, Map, MapError, AndThen, OrElse, Payload, ToOpt,
  UnwrapOr, Collect.

### Utilities, GC, and Core

- `src/util/` implements arena allocation, small vectors, hashing, UTF-8,
  string building, platform helpers, bit operations, and assertions.
- `src/gc/` implements Boehm GC lifecycle, allocations, roots, handles,
  shadow-stack infrastructure, and finalizer plumbing.
- `src/core/` implements the NaN-boxed value model, boxed 64-bit Ints, Strings,
  interned Tags, Lists, Tables, Tagged Lists, optionals, results, Iota,
  arithmetic, equality, hashing, reserved Tags, and rendering.
- Core Table and hashing boundaries reject List/Table keys.

### Public C API and FFI

- The manifest-listed C headers exist except generated
  `include/on1x/on1x_config.h`.
- Static and shared `libon1x` targets build; `src/api/on1x.map` exports
  `on1x_*`.
- Stack, scalar, List, Table, Tag, optional, result, call, error, GC-reference,
  version, evaluation, and approved FFI surfaces have initial implementations.
- `on1x_call` enforces the native one-result/error stack protocol.
- FFI supports validated scalar direct calls and dynamic library loading.
- Reverse callbacks and GC-owned native library resources are not complete.
- SWIG binding metadata/scripts exist, but bindings are not part of the regular
  validation matrix.

### Syntax

- Literals cover integer bases/separators, floating point, escapes, and Unicode
  scalar escapes.
- The arena AST, builder, readable printer, source map, diagnostics, keywords,
  precedence table, and terminator helpers exist.
- The active hand-written parser covers:
  - scalar literals and identifiers;
  - Lists, Tables, Tagged Lists, optionals, and effect-result reads;
  - unary and the full §12 binary precedence table;
  - calls, indexing, and field syntax;
  - comments and newline/semicolon statement separation;
  - top-level `let`, reassignment, and effect/follower statements.
  - lexical block expressions and `if` / `else` expressions.
  - named/anonymous functions, `return`, `while`/`for`, loop control, enum
    blocks, and `match` expressions with literal, wildcard, binding, List/tail,
    and Tagged List patterns.
- `src/syntax/grammar/on1x.g` transcribes the complete §17 grammar and is
  validated by build-time `make_dparser` generation.
- The generated dparser table is not yet the active parser because AST
  reductions and parity tests are incomplete.

### Sema

- `src/sema/resolver.*` resolves the implemented top-level binding slice,
  annotates globals, and rejects assignment to undefined bindings.
- `src/sema/effect_scope.*` enforces that effect-result `~` is visible only in
  the immediately following statement and resolves nested reads to the
  innermost active capture.
- Block-local lexical scopes and shadowing resolve correctly. Named and
  anonymous function scopes, direct and forwarded upvalues, closures,
  parameters, trailing `rest..`, and return legality resolve correctly. Loop
  context, enum validation and scoped `Iota`, pattern binding uniqueness,
  List-tail validation, and Identifier/Index/Field assignment targets resolve
  correctly. Constant folding remains unimplemented.

### IR and VM

- `src/ir/` contains register instructions, builder helpers, readable dumps,
  an initial CFG container, AST lowering, and register/terminator verification.
- Lowering covers the currently parsed expression, aggregate, binding,
  assignment, and effect nodes.
- Range syntax lowers as an `Iota` call; optional, index, field, and aggregate
  mutation expressions have explicit IR forms.
- The bytecode emitter now consumes IR rather than AST directly.
- The VM executes constants, global/local/upvalue loads and stores, List,
  Table, Tagged List, `Some`, and `None` construction; all parsed unary/binary
  operators; index/field reads and aggregate mutation; branches, closures,
  calls, stack discard, loops, and return.
- `on1x_eval` uses:

  ```text
  source → parser → sema → IR → verifier → bytecode → interpreter
  ```

- Executable language subset: Unit, Bool, Int, Float, String, Tag, Lists,
  Tables, Tagged Lists, optionals, global/local lookup and reassignment,
  blocks, `if`, named/anonymous functions, closures, `rest..`, calls, returns,
  parsed unary/binary operators, index and required field access.
- `match` executes through decision-tree IR and bytecode pattern opcodes.
  Arms are ordered; literals, Tagged Lists, plain Lists, tail bindings,
  wildcard and value bindings work; unmatched values raise through the normal
  API error path.
- Conditional branch, jump, merge, function, capture, and call nodes lower
  `if` expressions and functions. Logical `and`/`or` lower to branches and
  preserve short-circuiting. Effects lower through `lower_effect.cpp` into
  explicit capture boundaries and follower-result reads.

### Runtime

- `src/runtime/state.*` and `stack.*` exist.
- Global values currently live in the core Table owned by `On1x_State`;
  block-local bindings use VM local slots. Bytecode closures carry GC-managed
  captures and execute in independent local frames; host-native and On1x
  functions share bytecode call dispatch. Runtime-owned environment, closure,
  native-call, and iteration helpers now back those paths.
- Runtime pattern matching uses GC-managed compiled patterns and roots tail
  Lists while constructing them. `operators.cpp` handles numeric arithmetic,
  comparisons, structural equality, String concatenation, and unary forms.
  `field_access.cpp` handles optional index reads, required fields, and List/
  Table mutation. `effect.*` and `raise.*` construct exact
  `:Success[?value]` / `:Error[?message]` results; the bytecode interpreter
  unwinds division, native-call, missing-field, and match failures to the
  nearest capture boundary. Lexical environment objects and sandbox
  enforcement do not yet exist.

### Prelude

- `src/prelude/` is complete and linked into both static and shared
  `libon1x` targets. Every fresh state installs `TypeOf`, `TagOf`,
  `PayloadOf`, `Len`, `Get`, `Set`, `Push`, `Pop`, `Keys`, `Values`,
  `Iota`, optional helpers, and result predicates.
- Prelude natives obey the one-result/error API frame protocol. `Get` and
  `Pop` return optionals for expected absence; unhashable Table keys, invalid
  argument types, invalid mutation indices, and `Unwrap(:None)` raise.
- `Iota` is installed as the required `:Iota` callable value. Dedicated
  lowering removes its callee register before emitting the range opcode, so
  direct calls and `a .. b` both consume only range arguments.
- `tests/api/api_tests.cpp` covers prelude availability, mutation, table
  enumeration, Iota forms, optional/result predicates, and captured native
  error stack shapes.

## Immediate Critical Path

The next work must deepen the interpreter in dependency order. Do not skip
ahead to stdlib, C++ facade, or JIT.

### Stage 1 — Complete Statement and Block AST

Implement the remaining syntax/AST productions in coherent slices:

1. lexical `Block` and block-valued expressions;
2. `if` / `else`, `while`, `for`, `break`, and `continue`;
3. named and anonymous functions, parameters, `rest..`, and `return`;
4. enum blocks;
5. match arms and every pattern form.

For every slice:

- update both the active parser and AST printer;
- add positive, negative, and source-position tests;
- keep dparser grammar parity visible;
- do not activate generated dparser tables until reductions produce identical
  ASTs for the shared corpus.

**Exit gate:** every §17 production has an AST representation and parser tests.

### Stage 2 — Complete Sema

Build sema immediately behind each syntax slice:

1. introduce lexical scope stacks and binding records;
2. resolve locals, globals, shadowing, and reassignment;
3. resolve function parameters and capture upvalues;
4. reject `return` outside functions and loop controls outside loops;
5. add `assign_check.cpp` for Identifier/Index/Field assignment rules;
6. add `enum_check.cpp` for member validation and scoped Iota;
7. add `pattern_check.*` for unique bindings and legal tail patterns;
8. add `const_fold.cpp` only for behavior-preserving literal folds;
9. keep `effect_scope.cpp` and runtime capture semantics locked together.

**Exit gate:** all invalid scope/control/pattern programs fail before IR, with
specific diagnostics; closure capture metadata is complete.

### Stage 3 — Make IR Control-Flow Complete

Extend IR from a single linear block to the language execution model:

1. branches, conditional branches, block parameters, and loop back-edges;
2. functions, parameters, locals, upvalues, calls, and returns;
3. aggregate construction/mutation and general operators;
4. effect capture/unwind and follower binding in `lower_effect.cpp`;
5. enum construction and Iota scoping;
6. match decision trees in `lower_match.cpp`;
7. iteration protocol for List, Table keys, and Iota/ranges;
8. real CFG predecessor/successor construction;
9. verifier rules for dominance, terminators, operand arity, and register types
   where statically known;
10. only then add DCE, copy propagation, and conservative inlining.

**Exit gate:** every valid AST lowers to verified IR; no interpreter semantic is
implemented only as an AST special case.

### Stage 4 — Complete Runtime and Bytecode Interpreter

Implement manifest runtime files in this order:

1. `env.*` for lexical environments and cross-chunk globals;
2. `closure.*` and `call.*` for On1x/native functions and `rest..`;
3. `raise.*` and `effect.*` for catchable errors and exact `~` lifetime;
4. `operators.cpp`, including no-truthiness and String `+`;
5. `field_access.cpp` with required-key raise semantics;
6. `iterate.cpp` and `enum_eval.cpp`;
7. `pattern_match.cpp`;
8. `sandbox.cpp`;
9. complete bytecode opcodes, jump patching, frames, register windows, debug
   locations, and `dispatch_table.inc`.

Add interpreter tests at each step. Division by zero, wrong operand types,
missing required fields, and non-exhaustive match must raise through the same
error path.

**Exit gate:** all core language constructs run correctly on the interpreter,
including closures, effects, matching, enum, loops, and no-truthiness.

### Stage 5 — Prelude and C API Conformance

Completed: the manifest-listed `src/prelude/` subsystem is installed into
every fresh state and its behavior is covered through the parser, VM, native
call bridge, and C API stack inspection.

### Standard Library

- `src/stdlib/` infrastructure installs pure modules through `on1x_open_std`.
- `Cmp` is implemented with `Compare`, `Eq`, `DeepEq`, `Hash`, `MinOf`, and
  `MaxOf`; API coverage exercises equality, ordering, hash rejection, and
  empty-list absence.
- Native implementations now exist for `Math`, `Bit`, `Tag`, and `Str`,
  including numeric domains, shifts/bit indexing, tagged-list attach/detach,
  and UTF-8-aware string primitives. All five completed pure modules
  (`Cmp`, `Math`, `Bit`, `Tag`, `Str`) install through `on1x_open_std`.
  Persistent roots now retain state globals, the API stack, reserved tags,
  the tag-interning head, module tables, and native functions. This fixes the
  GC corruption exposed by larger module descriptors.

Remaining C API conformance work:

1. expand coverage to every public C entry point;
2. generate and validate `on1x_config.h`;
3. finish FFI callbacks/resource lifetime after native `:Fn` calls.

**Exit gate:** spec §18.2 and §16 are fully covered, with no ABI exception
escape and exact stack shapes.

## Later Paths

### C++ Facade

All manifest-listed `include/on1x/*.hpp` files are absent. Implement them only
after the C ABI and rooting rules are stable:

- move-only `State`;
- rooted `Value`;
- List/Table/Tag views;
- optional/result bridges;
- native callable adapters;
- evaluation helpers.

The facade adds safety and ergonomics, never semantics.

### Standard Library

Do not start stdlib member implementation until functions, native calls,
prelude, effects, and aggregate execution are green.

Then follow this dependency order:

1. infrastructure: registry, module builder, capability storage, args, docs,
   embedded-source loading, `on1x_open_std`;
2. `Cmp`;
3. `Math`, `Bit`, `Tag`, `Str`;
4. `List`, `Table`, `Opt`, `Res`, `Fn`, `Iter`, `Rand`;
5. embedded On1x sources and parity tests;
6. `Io`;
7. `Fs`/Path, `Os`, `Time`;
8. optional `Sexp`, `Dl`, and `Asm`.

The stdlib infrastructure requires these public headers:

- `include/on1x/on1x_stdlib.h`
- `include/on1x/on1x_capability.h`
- `include/on1x/on1x_module.h`
- `include/on1x/stdlib.hpp`

Capability modules must be absent when ungranted. Pure modules must pass a
syscall-interposed purity test.

### CLI and Tooling

After the interpreter and prelude:

- implement `src/tools/on1x_main.cpp`, REPL, dump tool, and host prelude;
- reconcile Print through `Io`, leaving no duplicate implementation;
- add conformance scripts under `tests/conformance/`;
- add AST/IR/bytecode dumps through `src/sexp` when enabled.

### SEXP

Implement the sfsexp wrapper only behind `ON1X_ENABLE_SEXP`. Keep all sfsexp
headers inside `src/sexp`. Use its writers for snapshots rather than ad-hoc
rendering.

### JIT and AOT

This remains gated until interpreter completion and parity:

- add `ON1X_ENABLE_JIT` and `ON1X_ENABLE_ASMTK`, default off;
- confine asmjit/asmtk includes to `src/jit`;
- implement shadow-stack safepoints and deoptimization;
- prove JIT/interpreter agreement on a shared corpus before exposing selection
  to hosts.

## Decision Gates Before Affected Work

The stdlib manifest contains assumptions that must not silently become API
behavior.

Resolved by the spec or settled decisions:

- Int width/overflow: signed 64-bit wrapping.
- Operator division by zero: raises; `Math.DivMod` zero divisor returns
  `:None`.
- Iota two-/three-argument convention: half-open.
- runtime-created Tags from valid UTF-8: permitted.

Still requires approval or a specification clarification:

1. **Deterministic Table `Repr`:** the stdlib manifest assumes sorting Table
   keys, but the settled rule says Tables are unordered and core rendering must
   not sort. Resolve before `Io.Repr` or `Sexp.Write`.
2. **Sandbox root API:** the manifest assumes the filesystem sandbox root is a
   public C API concept. Resolve before implementing `Fs` or adding ABI.

Do not implement the affected behavior until these gates are answered.

## Missing Manifest Inventory by Subsystem

This list is directional; use the manifests as the exact checklist.

- **Public facade:** headers exist and compile as C++20; behavioral conformance
  (rooting, views, adapters, and evaluation helpers) remains incomplete.
- **Sema:** constant-fold file.
- **IR:** CFG and conservative optimization passes are present; verifier and
  optimization coverage still need expansion.
- **VM:** generated dispatch table and remaining full opcode/debug behavior.
- **Runtime:** enum evaluation, sandbox, and remaining manifest files outside
  state/stack/env/closure/call/effects/raising/iteration/operators/field
  access/pattern matching.
- **Prelude:** complete.
- **SEXP:** entire subsystem.
- **Tools:** entire subsystem.
- **JIT:** entire subsystem, intentionally gated.
- **Stdlib:** remaining pure/capability modules, headers, embedded sources,
  required tests, and CMake glue; `Cmp` is the current completed module.

Some FFI manifest paths exist but remain behaviorally incomplete; do not count
callbacks or resource lifetime complete from file presence alone.

## Test Debt and Required Matrices

Core tests still required:

- every literal and diagnostic from §3;
- statement continuation/termination edge cases;
- nested lexical scope, shadowing, closure, and upvalue tests;
- no-truthiness conditions;
- Iota callable forms and enum numbering;
- effect visibility exactly one follower later and nested innermost-wins;
- exhaustive/non-exhaustive match and pattern binding uniqueness;
- interpreter/operator error paths;
- every C API function's success and failure stack shape.

Stdlib tests required by the manifest:

- `test_capability.cpp`
- `test_purity.cpp`
- `test_no_nil.cpp`
- `test_math_domain.cpp`
- `test_str_utf8.cpp`
- `test_table_keys.cpp`
- `test_iter_laziness.cpp`
- `test_cmp_agreement.cpp`
- `test_res_effects.cpp`
- `test_embedded_parity.cpp`
- `test_arity_errors.cpp`

Build options/matrices to add and keep green:

```sh
# Default
cmake -S . -B build -DON1X_BUILD_TESTS=ON

# Pure stdlib only
cmake -S . -B build-pure -DON1X_STDLIB_PURE_ONLY=ON

# Core and pure stdlib without optional subsystems
cmake -S . -B build-core \
  -DON1X_ENABLE_JIT=OFF -DON1X_ENABLE_FFI=OFF -DON1X_ENABLE_SEXP=OFF

# Stdlib documentation compiled out
cmake -S . -B build-nodocs -DON1X_STDLIB_DOCS=OFF
```

Add the options before claiming those configurations were validated.

## Recommended Next Unit

Continue implementing remaining pure modules: `Fn`, `Iter`, `Rand`. Then create
`lib/on1x/` directory with embedded `.on1x` sources (Opt, Res, Iter parity
modules), implement `tools/embed_sources.py`, and add parity tests. After pure
modules, proceed to capability modules: `Io`, `Fs`, `Os`, `Time`.

Required tests still to add: `test_purity.cpp`, `test_capability.cpp`,
`test_no_nil.cpp`, `test_iter_laziness.cpp`, `test_res_effects.cpp`,
`test_embedded_parity.cpp`.

**Immediate cleanup:** Remove remaining debug fprintf calls from
`src/api/api_state.cpp` and `src/vm/interpreter.cpp`.

## Resume Checklist

1. Run default tests before editing.
2. Pick one dependency-complete unit from the critical path.
3. Add behavior and tests together.
4. Run focused tests, then default and relevant feature builds.
5. Run `git diff --check`.
6. Update this roadmap's baseline, completed capability, remaining manifest
   inventory, and recommended next unit.
7. Report what remains; never call On1x complete while a manifest entry or
   required test is absent.

## Current Diff

Modified files in this session:
- `src/syntax/parser.cpp` — parse_block fix + debug cleanup
- `src/stdlib/res/res.cpp` — debug cleanup

Uncommitted. Build verified with `default` configuration (19/19 pass).
