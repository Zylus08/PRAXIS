# Implementation report

M1 implements a native tree-walking interpreter in C++20. The source manager, token scanner, minimal preprocessor, recursive-descent/precedence parser, typed AST, semantic scopes and checker, value/conversion layer, object/frame runtime, native bindings and CLI are separate modules. No existing parser or interpreter was embedded.

The implementation preserves lvalue identity, const access, reference aliases, value parameters, recursion, lexical lookup, loop control and deterministic scope cleanup. Static verification is independent of evaluation. Native output and string support are explicitly documented host bridges.

Verification found and repaired literal/operator token confusion, NUL-terminated literal behavior, include visibility ordering, global constant initialization ordering, keyword reservation, for-body redeclaration checks, malformed directive operands, floating signed-zero negation and direct long-long-to-float rounding. Regression cases remain in the fixture corpus. Runtime checks guard arithmetic overflow, conversion ranges, array bounds, initialization state, call/evaluation depth and execution budget.

Build issues were environmental: default Ninja concurrency exceeded compiler heap availability; MSVC's shared PDB manager reported C1902. The reproducible build uses one compiler process and embedded /Z7 information, including CMake compiler probes. No persistent system/toolchain configuration was changed.

The external verification harness is the only source-compilation path. Release differential verification can reuse reference results when the fixture source SHA-256 matches an earlier actual MSVC compilation; it still checks, executes and re-executes the Release interpreter. The C++ executable has only Windows/MSVC runtime dependencies. A separate smoke test executes it with compiler tools removed from PATH.

Language exclusions and the REPL's nontransactional runtime-error behavior are documented in docs/LANGUAGE_SUPPORT.md, docs/SEMANTICS.md and docs/LIMITATIONS.md. There are no fake class, template, overload or destructor implementations. Future extension points are described without claiming they work.
