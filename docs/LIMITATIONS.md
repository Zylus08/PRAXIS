# Known Limitations and Non-Goals (v0.1.0-rc1 / Milestone M3)

## 1. Scope and Conformance

Praxis v0.1.0-rc1 is an independent, lifetime-correct C++ tree-walking interpreter for a documented subset of C++20. It is not a fully conforming C++20 compiler and cannot parse unmapped system toolchain headers.

---

## 2. Language Feature Boundaries

- **Inheritance & Polymorphism:** Classes and structs do not support base classes (`class Derived : public Base`) or virtual methods (`virtual`). Object layouts are single-class flat structures.
- **Exception Handling:** Interpreted C++ exceptions are not implemented. Keywords `throw`, `try`, and `catch` are rejected at parse time. Internal runtime errors unwind interpreter frames safely without exposing simulated C++ catch blocks.
- **Operator Overloading:** Built-in operators behave according to C++ rules. User-defined operator overloading (such as custom `operator+` or non-copy `operator=`) is rejected.
- **Template System:** Instantiated bodies are analyzed upon instantiation against visible symbols. Variadic templates (`typename... Args`), partial specialization, concepts, and SFINAE are not supported.
- **Standard Library Headers:** Virtual capability definitions exist for `<iostream>`, `<string>`, `<vector>`, `<utility>`, `<algorithm>`, and `<array>`. Other standard headers (`<map>`, `<memory>`, `<thread>`, `<chrono>`, `<filesystem>`) are unmapped and trigger preprocess rejection.
- **Pointers & Memory:** Pointers use an abstract handle model. Pointer-to-member (`T::*`), placement new (`new (buf) T`), and raw virtual address casting (`reinterpret_cast`) are deliberately unsupported.

---

## 3. Analysis and Safety Policies

- **Structural Return Analysis:** Non-void functions other than `main` must structurally return on all control paths (e.g. direct return in a block or both branches of an `if-else`).
- **Safety Traps:** Out-of-range integral conversions and non-finite floating operations (division by zero, overflow) produce immediate runtime diagnostics rather than undefined behavior or modular wraparound.
- **Memory Violation Interception:** Null dereference, use-after-free, double free, and dereferencing expired stack frames are actively intercepted and diagnosed rather than causing host process memory corruption.

---

## 4. REPL Session Model

- **Multiline Heuristic:** The interactive REPL heuristic balances braces and quotes. An empty line forces submission of an incomplete buffer to inspect diagnostics.
- **Non-Transactional Failures:** Runtime errors during REPL evaluation do not roll back state. Use `:reset` for a clean environment.

---

## 5. Resource Limits and Toolchain Context

- Resource consumption is bounded by configurable CLI flags (`--max-calls`, `--max-template-depth`, `--max-instantiations`, `--max-objects`, `--max-steps`).
- Host platform: Microsoft Visual C++ 2022 (x64) on Windows 11.
