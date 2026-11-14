# Project PRAXIS M1 — PASS for the documented subset

## Repository and commits

- Repository: `E:\PRAXIS`; branch: `main`.
- Starting commit: none — the directory was empty and was not a Git repository.
- Verified implementation commit: `f33b72a9b58f36396fcb638a0f49f8a1003799f5`.
- Final delivery commit: the evidence commit containing this report. Resolve it with `git log -1 --format=%H -- audit/m1/M1_FINAL_REPORT.md`; its exact hash is also supplied in the delivery response. A commit cannot contain its own hash.
- Existing Git identity preserved. No remote push performed.

## Implemented language and architecture

Independent source/diagnostics, lexer, token preprocessor, recursive-descent parser with precedence climbing, typed AST, source-order static symbols/scopes, conversion/lvalue checks, and native AST tree-walking execution. Runtime has explicit values, object identity, initialization/lifetime flags, lexical scopes, call frames, reference parameter binding and cleanup.

Working features: the required scalar numeric types and void; const objects; fixed arrays; matching lvalue references; declarations/definitions/calls and recursion; nested scopes; if/else, for, while, do/while, break/continue/return; arithmetic, comparison, logical short-circuiting, assignments and increments; object macros, include paths/cycle detection; controlled cout/endl and basic string operations; run/check/help/version and a persistent REPL.

Unsupported: classes, constructors/destructors, templates, overload resolution, pointers, general namespaces/STL, full preprocessing and the other explicitly listed language forms. No fake M2/M3 implementation is claimed. String native contracts and restrictive conversions are documented.

## Verification

Both Debug and Release builds succeeded using MSVC 2022 x64, CMake and Ninja. Each passed 15 frontend assertions, 244 integration checks, 13 adversarial/environment checks, and 134 differential programs. Invalid fixtures are rejected with phase and location diagnostics. The 12 examples plus help/version passed all 14 demonstration checks.

Demonstrated output: hello `Hello World\n`; arithmetic `14`; conditions `1`; loops `012345\n`; factorial `3628800`; Fibonacci `55`; function/value parameters `72`; scopes `21`; arrays `10`; reference mutation `9`; strings `hi!`. The diagnostic example intentionally returns 1 with a semantic location diagnostic. `check` emits no program output.

See TEST_RESULTS.md for category counts, DIFFERENTIAL_RESULTS.md for oracle methodology, JSON records for per-case evidence, build logs for compilation, and release-dependencies.txt for binary dependencies. Compiler-free PATH execution passed. Native runtime source has no subprocess or compiler integration.

## Acceptance gates

All twelve specified gates pass within the documented subset: builds; independent execution; independent frontend/checker/runtime; functions/recursion; lexical scopes; control flow; types/references; preprocessing; CLI/REPL; differential agreement; invalid-program diagnostics; accurate boundary documentation.

## Limitations and evidence

No recorded test failure remains. Conservative checking, fixed resource limits, restricted string types, incomplete UB detection, limited global constant evaluation and REPL recovery/completeness limitations remain. See KNOWN_LIMITATIONS.md and ../../docs/LIMITATIONS.md. Passing M1 is not a claim of full C++ conformance.

Evidence resides in `E:\PRAXIS\audit\m1`. `verification-manifest.json` records the source commit and binary hashes; all machine-readable final test reports match those hashes. Optional timing data is in benchmark.json and includes process startup; it is not a performance acceptance claim.
