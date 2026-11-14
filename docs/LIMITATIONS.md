# Known Limitations and Non-Goals (Milestone M2)

## 1. Scope and Conformance

Praxis M2 is an independent, class-aware, lifetime-correct C++ tree-walking interpreter for a defined subset of C++. It is not a fully conforming C++20 compiler and cannot parse system toolchain headers.

---

## 2. Language Feature Boundaries

- **Pointers:** Deliberately unsupported in M2. Member access via `.` and identity preservation via lvalue references (`T&`) are fully implemented and verified. Full pointer semantics, pointer-to-object (`->`), address-of, and heap allocation are deferred to Milestone M3's memory model.
- **Inheritance & Polymorphism:** Classes and structs do not support base classes (`class Derived : public Base`) or virtual methods (`virtual`). Object layouts are single-class flat structures.
- **Exception Handling:** Interpreted C++ exceptions are not implemented. Keywords `throw`, `try`, and `catch` are rejected at parse time. Internal runtime errors unwind interpreter frames safely without exposing simulated C++ catch blocks.
- **Operator Overloading:** Built-in operators behave according to C++ rules. User-defined operator overloading (such as `operator+` or custom `operator=`) is rejected.
- **Template System:** Single-phase lookup is used; instantiated bodies are analyzed upon instantiation against visible symbols. Partial specialization, non-type template parameters (`template<int N>`), variadics (`typename...`), concepts, and SFINAE are not supported.
- **Standard Library:** Only virtual capability definitions `<iostream>` and `<string>` are built-in. General STL headers (`<vector>`, `<memory>`, `<algorithm>`) cannot be included.

---

## 3. Analysis and Safety Policies

- **Structural Return Analysis:** Non-void functions other than `main` must structurally return on all control paths (e.g. direct return in a block or both branches of an `if-else`). Valid C++ functions relying on infinite loops without trailing returns are rejected as a safety policy.
- **Safety Narrowing Rejection:** Out-of-range integral conversions and non-finite floating operations (division by zero, overflow) produce immediate runtime diagnostics rather than undefined behavior or modular wraparound.
- **Array Lists:** Array list initialization conservatively rejects narrowing conversions between numeric ranks.

---

## 4. REPL Session Model

- **Multiline Heuristic:** The interactive REPL heuristic balances braces and quotes. Braces within multiline comments or character literals should be avoided. An empty line forces submission of an incomplete buffer to inspect diagnostics.
- **Non-Transactional Failures:** Runtime errors during REPL evaluation do not roll back state or stdout output. The failed declaration remains in declaration history; use `:reset` for a clean environment.

---

## 5. Resource Limits and Toolchain Context

- Resource consumption is bounded by configurable CLI flags (`--max-calls`, `--max-template-depth`, `--max-instantiations`, `--max-objects`, `--max-steps`). While these limits protect the host process from stack exhaustion and infinite loops, Praxis is not an OS-isolated security sandbox.
- Validation targets Microsoft Visual C++ 2022 (x64) on Windows 11.
