# PRAXIS v0.1.0-rc1 — Catalog of Release Limitations & Unsupported Features

## 1. Scope & Release Candidate Integrity

Project PRAXIS is an independent C++ subset interpreter designed for educational verification, deterministic execution, and formal lifetime analysis.

In accordance with Section 1.11 of the M3 specifications ("Never report unsupported language constructs as fully implemented"), this document catalogs every deliberate limitation, boundary condition, and unsupported language construct in **Praxis v0.1.0-rc1**.

---

## 2. Object Model & Class Hierarchies

1. **Inheritance:**  
   Single inheritance (`class Derived : public Base`), multiple inheritance, and virtual base classes are **not supported**. Declaring base classes in class definitions produces a syntax/parse diagnostic.
2. **Virtual Member Functions & Polymorphism:**  
   The `virtual` keyword, `override`, `final`, vtable generation, and dynamic virtual dispatch are **not supported**.
3. **Friend Declarations:**  
   `friend class` and `friend function` declarations are not implemented.

---

## 3. Exception Handling & Stack Unwinding

1. **Exceptions:**  
   `throw`, `try`, and `catch` blocks are deliberately rejected during parsing.
2. **Interpreter Unwinding:**  
   Unwinding occurs strictly along deterministic lexical and control-flow paths (`return`, `break`, `continue`, block termination). Host C++ exception mechanisms are internal to the interpreter runtime and are never exposed as user-language exceptions.

---

## 4. Pointers & Memory Model Limitations

1. **Unrestricted Pointer Arithmetic:**  
   Pointer arithmetic is restricted to elements within the same identified object or container slice. Arithmetic extending outside bounds is intercepted at runtime rather than causing undefined memory corruption.
2. **Raw Memory Address Access:**  
   Host virtual addresses cannot be inspected or cast to integers (`reinterpret_cast` from pointer to `uintptr_t` is unsupported).
3. **Pointers to Members:**  
   Pointer-to-member syntax (`int S::* ptr`) is not supported.
4. **Placement New:**  
   Constructing objects into pre-allocated memory buffers via `new (buffer) T(...)` is not implemented.

---

## 5. Templates & Metaprogramming

1. **Variadic Templates:**  
   Template parameter packs (`typename... Args`) and fold expressions are not supported.
2. **Template Template Parameters:**  
   Passing templates as arguments to other templates (`template<template<class> class TT>`) is unsupported.
3. **C++20 Concepts & Requires Clauses:**  
   `concept` and `requires` constraints are not implemented.
4. **Non-Type Template Parameters:**  
   Only integer literal bounds (e.g. for `std::array<T, N>`) are supported; general expression non-type template parameters are not implemented.

---

## 6. Standard Library Subset Boundaries

Praxis includes built-in virtual implementations for:
- `<iostream>`: `std::cout`, `std::endl`, stream insertion `<<`.
- `<string>`: `std::string` with `.size()`, `.length()`, `.empty()`, `.at()`, `.substr()`, character indexing.
- `<vector>`: `std::vector<T>` with standard constructor, element access, iterators, and modifier methods.
- `<utility>`: `std::pair`, `std::make_pair`, `std::swap`, `std::move`.
- `<algorithm>`: `std::min`, `std::max`, `std::sort`, `std::reverse`, `std::find`, `std::count`.
- `<array>`: fixed-size container subset.

All other standard library headers (e.g. `<map>`, `<unordered_map>`, `<memory>`, `<thread>`, `<chrono>`, `<filesystem>`, `<regex>`) are unmapped and trigger preprocess diagnostics upon `#include`.
