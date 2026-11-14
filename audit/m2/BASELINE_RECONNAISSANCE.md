# M2 baseline reconnaissance

Starting branch main, clean commit 389ecb585ef075113580f69aaf17f6169dacd46c. No AGENTS.md was found. Existing Git identity and independent repository retained.

Before implementation, Debug and Release were rebuilt with the existing MSVC 2022/CMake/Ninja scripts. Each passed 15 frontend assertions, 244 integration cases, 13 adversarial/environment checks and 134 differential programs: 406 checks/programs per configuration. The differential baseline reused SHA-256-matching actual MSVC oracle outputs. Raw logs and JSON are preserved as baseline-*; M1's committed audit artifacts were restored unchanged after the baseline harness regenerated them.

Inspected lexer/preprocessor, AST/type definitions, parser, analyzer, storage/frame/reference representation, evaluation/calls/cleanup, native bindings, CLI/REPL, verification harness and documented limitations. Source is a modular first-principles tree walker, with no compiler backend.

Gaps: scalar-only type identity; one function per name; no class declarations or general member AST; scope destruction only expires storage and cannot execute source destructors; no constructor/copy/result-object machinery; no template substitution/cache; fixed resource limits and no phase profiling. M2 extends those boundaries while preserving numeric/native behavior.

The M1 overload-rejection fixture is a deliberate unsupported-feature test. M2 must promote its expected result to success because genuine free-function overloading is required. The same source remains tested, with the expectation migration recorded explicitly rather than treating valid C++ as an error.
