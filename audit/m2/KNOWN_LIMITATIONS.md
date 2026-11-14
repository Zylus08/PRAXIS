# Known Limitations and Language Boundary (Milestone M2)

**Milestone:** Project PRAXIS — Milestone M2  
**Target Specifications:** Sections 1, 3, 4, 5, 6, 7, 8, 9, 10, 15  

---

## 1. Architectural Philosophy on Limitations

Project PRAXIS adheres to a strict non-negotiable principle:
> *"Never report unsupported language constructs as fully implemented. Distinguish supported, partial, and unsupported semantics accurately."*

Milestone M2 introduces genuine C++ classes, structs, constructors, destructors, RAII lifetimes, copy semantics, function/constructor overloading, and template type substitution. It does **not** claim full ISO C++20 standard compliance. Constructs outside the documented M2 boundary are explicitly and cleanly rejected with actionable diagnostics at parse or semantic analysis time.

---

## 2. Detailed Capability Matrix

### 2.1 Classes and Structs
- **Supported:**
  - `class` and `struct` declarations.
  - Member variables (scalar, string, and nested class types).
  - Public and private access control (`public:`, `private:`).
  - Member functions defined in-class or out-of-class.
  - Implicit `this` member resolution.
  - `const` member functions and const-correctness enforcement.
  - Member access operator `.`.
  - Nested class object aggregation.
- **Unsupported (Explicitly Rejected):**
  - Inheritance (single or multiple) is rejected at parse time (`class B : public A`).
  - Virtual functions and dynamic dispatch are rejected at parse time (`virtual void f()`).
  - Arrays of class objects (`C arr[5]`).
  - Reference members in classes (`int& ref;`).
  - Static member variables or static member functions.
  - Bitfields.
  - Unions and anonymous structs.

### 2.2 Constructors and Destructors
- **Supported:**
  - Default constructors (implicit synthesis and user-defined).
  - Parameterized constructors with overload resolution.
  - Member initializer lists executing strictly in **declaration order**.
  - Default member initializers (e.g. `int x = 5;`).
  - Scalar member value initialization (`$zero`).
  - Copy constructors (implicit synthesis and user-defined `ClassName(const ClassName&)`).
  - Destructors (`~ClassName()`) executed automatically in reverse construction order at scope exit.
  - Full RAII cleanup on normal block exit, `return`, `break`, `continue`, and loop iterations.
  - Construction failure unwinding (partial member destruction without leaking incompletely initialized objects).
- **Unsupported (Explicitly Rejected):**
  - Delegating constructors (`C() : C(0)`).
  - Inheriting constructors (`using Base::Base`).
  - Brace-enclosed list initialization for classes (`C c{1, 2}`).
  - Move constructors (`ClassName(ClassName&&)`).

### 2.3 Copy Semantics & Assignment
- **Supported:**
  - Distinct copy construction vs copy assignment.
  - Synthesized memberwise copy construction.
  - Synthesized memberwise copy assignment.
  - Self-assignment safety guard (`if (dest == src) return;`).
  - Rejection of copy assignment on classes containing `const` members.
  - Prvalue copy elision in direct initialization and member initialization (`$elide`).
- **Unsupported (Explicitly Rejected):**
  - User-defined assignment operators (`C& operator=(const C&)`) are rejected at parse time.
  - Move assignment (`operator=(C&&)`).
  - Custom overloaded operators (`operator+`, `operator==`, etc.).

### 2.4 Overload Resolution
- **Supported:**
  - Free functions, member functions, constructors, and function templates.
  - Standard conversion ranking: Rank 0 (Exact match / reference binding), Rank 1 (Promotion), Rank 2 (Standard numeric conversion), Rank 3 (Text to string).
  - Const vs non-const member function candidate ranking.
  - Preference for non-template over template candidates upon tie.
  - Strict ambiguity detection (`semantic: ambiguous overload`).
- **Unsupported (Explicitly Rejected):**
  - Overloading differing only by return type (rejected at declaration).
  - Default parameter values (`void f(int x = 0)`).
  - User-defined conversion operators (`operator int()`).
  - Ellipsis parameter lists (`...`).

### 2.5 Templates
- **Supported:**
  - `template<typename T>` and `template<class T>`.
  - Single and multiple type parameters (`template<class T, class U>`).
  - Function templates with explicit type arguments.
  - Function template type deduction from call arguments (scalars, records, nested types).
  - Class templates with type parameters (`Box<T>`).
  - Deep AST cloning and type substitution engine.
  - Instantiation caching based on mangled signature keys.
  - On-demand semantic checking of instantiated function bodies.
  - Exact diagnostics for invalid instantiations.
  - Configurable limits: `--max-template-depth`, `--max-instantiations`.
- **Unsupported (Explicitly Rejected):**
  - Non-type template parameters (`template<int N>`).
  - Template template parameters (`template<template<class> class TT>`).
  - Partial specialization of class templates.
  - Variadic templates / parameter packs (`typename... Args`).
  - Concepts and `requires` constraints.
  - SFINAE (Substitution Failure Is Not An Error).

### 2.6 Pointers
- **Status: Deliberately Unsupported in M2**
- Per Section 9 of the project requirements:
  > *"If pointers cannot be completed safely in M2, keep them explicitly unsupported and ensure member access via . and references works correctly."*
- Pointers (`T* p`, `->`, `&`, `*`) are explicitly rejected at parse time (`tests/m2/fixtures/pointer.cpp`).
- Member access via `.` and identity preservation via lvalue references (`T&`) are fully implemented and verified.
- Sound pointer representations, pointer arithmetic, and heap allocation (`new`/`delete`) are deferred to Milestone M3's memory model.

### 2.7 Exception Handling
- **Status: Deliberately Unsupported in M2**
- Per Section 5 of the project requirements:
  > *"Exception handling is not mandatory for M2. If unsupported, reject throw, try, and catch rather than implying correct exception-driven unwinding."*
- Keywords `throw`, `try`, and `catch` are rejected at parse time (`tests/m2/fixtures/throw.cpp`, `tests/m2/fixtures/try_catch.cpp`).
- Internal interpreter exceptions are utilized strictly for interpreter error propagation and unwinding, never exposed as interpreted C++ language features.

### 2.8 Standard Library
- Praxis does not include or parse host system C++ headers.
- Native virtual headers `<iostream>` (`std::cout`, `std::endl`) and `<string>` (`std::string`) provide minimal, verified built-in facilities.
- Complex STL containers (`std::vector`, `std::map`, `std::unique_ptr`) are not yet implemented and are planned for M3.
