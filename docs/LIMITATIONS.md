# Known limitations and non-goals

The precise subset is in LANGUAGE_SUPPORT.md and SEMANTICS.md. This milestone is not full C++ conformance and cannot parse system standard-library headers.

- No user-defined classes, templates, pointers, overloads, exceptions, destructors, or general STL.
- String literals and signed length results are restricted native facilities, not a complete model of C++ array-to-pointer decay or size_t.
- No const-reference temporary binding, reference returns, array parameters, or scalar brace initialization.
- Initialization-order-sensitive global programs beyond the documented constant-expression forms are excluded; general constexpr evaluation is absent.
- Return analysis and array list narrowing checks are conservative and can reject otherwise-valid C++.
- Out-of-range integral narrowing and non-finite floating results are deliberately rejected.
- General unsequenced-access UB, dangling class/pointer storage, unspecified-order portability and all undefined behavior are not analyzed. Only tested well-defined overlapping programs are suitable for differential comparison.
- The REPL's multiline completeness heuristic balances braces and double quotes; it does not fully lex comments/character literals. Keep braces out of comments and character literals in multiline REPL submissions. Files use the full lexer and have no such heuristic. An empty line forces submission of incomplete input for diagnostics.
- Runtime REPL failure is not transactional. Existing mutations and output remain, and the failed submission stays in declaration history; use `:reset` for a clean session.
- Diagnostics stop at the first failure; there is no error recovery tree, source excerpt, or interpreted call-stack display.
- Object-macro expansion preserves invocation location but not a complete expansion backtrace. Repeated project includes can produce redefinitions. Conditional inclusion and header guards are absent.
- Resource limits are fixed implementation limits. Memory is not quota-enforced; this is not a hardened execution sandbox. No concurrency or external I/O beyond native stdout is exposed.
- Validation targets MSVC 2022 x64 on this machine. Other toolchains are not claimed verified. Default float stream formatting is host-bound; locale/Unicode equivalence beyond the byte-oriented fixture set is not claimed.
- Differential coverage is a regression suite, not a proof of language conformance. No performance target or comparison against other interpreters is claimed.
