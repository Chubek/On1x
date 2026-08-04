# On1x dparser Grammar

`on1x.g` is the scannerless grammar table source for spec §17. CMake runs the
vendored `make_dparser` executable and writes `on1x.g.d_parser.c` into the build
tree; the generated file is never edited or checked in.

The grammar spells precedence through layered productions rather than dparser
priority annotations. This keeps the non-associative range production explicit
and matches the table in spec §12.

Newlines remain grammar tokens because they terminate statements. Spaces,
carriage returns, comments, and horizontal tabs are handled by the reserved
`whitespace` production. The active C++ parser is retained until dparser
reduction actions build every AST production and parser-parity tests pass.
