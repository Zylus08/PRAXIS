# Milestone RC1.1 Release Claim Ledger

This ledger defines the verified claims, boundaries, and deliberate exclusions for Praxis v0.1.0.

---

## 1. Verified Release Claims

1. **Independent C++ Execution Backend:**
   Praxis interprets user C++ code directly using an independent, from-scratch lexer, recursive-descent parser, typed AST semantic analyzer, and tree-walking interpreter. It does not invoke Clang, LLVM, GCC, or MSVC during interpretation.
2. **Object Lifetimes and RAII:**
   Automatic local objects and container elements are created declaration-order and destroyed in strict reverse order of construction. Destructors are guaranteed to execute upon scope exit, loop breaks, function returns, downward vector resizing, and vector clearing.
3. **Pointers and Allocation Safety:**
   Pointers are tracked via abstract generational handles (`Pointer`), preventing exposure of raw host addresses and diagnosing null pointer dereferences, use-after-free, double-free, dangling stack/vector pointers, and non-heap deletions at the exact point of violation.
4. **STL Container Subset:**
   Full, verified support for `<vector>` (`push_back`, `pop_back`, `clear`, `resize`, `reserve`, `size`, `empty`, `at`, `[]`, `front`, `back`, `begin`, `end`, range-for), `<string>`, `<utility>` (`std::pair`, `make_pair`, `swap`, `move`), `<algorithm>` (`min`, `max`, `sort`, `reverse`, `find`, `count`), and `<array>`.
5. **Template & Class Integration:**
   Full support for function templates, class templates, constructor overload resolution, member initializers, classes with container members, and template functions accepting container references.
6. **Differential Verification:**
   Byte-for-byte differential output parity against native MSVC (`cl.exe /std:c++20 /Od /Zc:nrvo-`) verified across 258 distinct C++ programs.

---

## 2. Explicitly Unsupported Features & Release Boundaries

| Feature Category | Unsupported ISO C++ Features |
|---|---|
| **Inheritance & Polymorphism** | Single/multiple inheritance, `virtual` functions, abstract base classes, vtables, `dynamic_cast`. |
| **Exceptions & RTTI** | `try`/`catch`/`throw` in interpreted user source code; `typeid`. |
| **Move Semantics & Rvalues** | Rvalue references (`T&&`), move constructors (`C(C&&)`), move assignment (`C::operator=(C&&)`). `std::move` casts to an rvalue falling back to copy construction. |
| **Modern C++20 Features** | Concepts, requires-clauses, coroutines, modules, spaceship operator `<=>`, ranges library. |
| **Standard Library Scope** | Filesystem (`<filesystem>`), multithreading (`<thread>`, `<mutex>`), associative containers (`<map>`, `<set>`), smart pointers (`std::unique_ptr`, `std::shared_ptr`). |
| **Input Streams** | `std::cin` is not supported in batch interpretation. |

---

## 3. Conformance Statement

Project PRAXIS makes **NO CLAIM** of general ISO C++20 standard conformance. Praxis v0.1.0 is certified as an educational and embedded C++ subset interpreter implementing a rigorously tested and lifetime-safe core dialect.
