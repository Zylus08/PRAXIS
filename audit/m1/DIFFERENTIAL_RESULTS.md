# Differential verification — PASS

- Reference: MSVC 2022, compiler 19.43.34809, C++20 `/EHsc /Od`.
- Debug: **134/134** supported, well-defined fixture programs matched stdout and exit status.
- Release: **134/134** matched the same independently compiled reference behavior.
- Praxis semantic-check failures: 0. Reference compilation failures: 0. Output/status mismatches: 0.
- Praxis was run twice for every comparison to verify determinism.
- Final Debug pass reused 132 prior actual reference runs by fixture SHA-256, compiling the two newly added numeric regressions; Release reused 134 matching reference runs. Reuse avoids recompiling unchanged oracle programs; it does not replace execution of the tested interpreter.

Each JSON row includes source SHA-256, reference compilation status, reference stdout/status, Praxis stdout/status, check status, and reuse provenance. The executable SHA-256 ties each report to the delivered binary. Mutable state is observed through explicit printed variables, array elements, reference aliases and function results.

Coverage includes precedence, promotion, conversions, exact long-long comparison, float rounding, signed zero, lvalue assignments, loops and loop control, scopes, globals and constant initialization, array initialization/access, references, recursive functions, native strings/output and object macros.

Runtime-failure programs (overflow, uninitialized reads, invalid indices, etc.) are excluded from reference execution; the reference oracle is never asked to execute intentional UB. Invalid syntax/type programs are verified by the separate rejection corpus. These regression results establish agreement for these fixtures only, not full C++ conformance.

The interpreter never invokes the reference compiler. Only the external Python harness launches `cl.exe` and the generated reference executables.
