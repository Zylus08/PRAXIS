# Known limitations

No unresolved failure remains in the recorded test suites. This does not imply that all programs within the intended subset are bug-free.

The supported language intentionally omits classes, templates, pointers, overloads, namespaces, exceptions, general STL and full preprocessing. References cannot bind temporaries or be returned; arrays cannot decay or be passed as parameters. Native string lengths use signed long long, and literals have a restricted text representation.

Conservative return analysis and array narrowing rules reject some valid C++. Out-of-range narrowing and non-finite arithmetic are diagnosed. General constexpr evaluation and initialization-order-sensitive cases beyond the documented forms are excluded. General UB/alias-effect analysis is absent. Resource limits are fixed and do not create a security sandbox.

REPL multiline detection is a brace/quote heuristic; comments and character literals containing braces may confuse it. Runtime failures retain prior mutations/output and declaration history. Static failures do not commit the submission. Diagnostics report the first error without an interpreted call-stack trace.

Detailed boundaries: ../../docs/LANGUAGE_SUPPORT.md, ../../docs/SEMANTICS.md, ../../docs/LIMITATIONS.md (relative from audit/m1, use ../../docs).
