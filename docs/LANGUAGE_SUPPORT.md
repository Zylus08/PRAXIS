# Praxis Language Boundary (Milestone M2)

## 1. Implemented C++ Subset

### 1.1 Types and Literals
- `bool`, signed 8-bit `char`, 32-bit `int`, 64-bit `long long`, binary32 `float`, binary64 `double`, `void` function results, `std::string`.
- Decimal integers (optional `LL`/`ll`), floating literals with exponents (optional `f`/`F`), byte character and string literals, `true`/`false`.
- Escapes: `\n`, `\r`, `\t`, `\0`, `\\`, `\'`, `\"`.
- One-dimensional fixed arrays with literal decimal bounds 1–1,000,000. Brace initialization, zero-filling omitted elements, indexing, element references.
- Lvalue references (`T&`, `const T&`) preserving referent identity.

### 1.2 Classes and Structs
- `class` and `struct` definitions.
- Default access rules: `class` members default to `private`; `struct` members default to `public`.
- Access control labels: `public:` and `private:`.
- Data members (scalar types, `std::string`, and nested class types).
- Member functions defined inside class bodies or out-of-class with `ClassName::MethodName` syntax.
- Implicit `this` member resolution within member methods.
- `const` member functions enforcing const-correctness on implicit object members.
- Member access via `.` operator on lvalues and prvalues.

### 1.3 Constructors and Destructors
- Implicit default constructor synthesis when no user constructor is defined.
- Parameterized constructors with overload resolution.
- Member initializer lists executing strictly in **declaration order**.
- In-class default member initializers (e.g. `int x = 5;`).
- Explicit member value-initialization (`Member()`) executing zero-initialization on scalars.
- Copy constructor synthesis and user-defined copy constructors (`ClassName(const ClassName&)`).
- Destructors (`~ClassName()`) executed automatically in reverse construction order upon scope exit.
- Construction failure unwinding: destroys previously constructed sub-objects without leaking incomplete objects.

### 1.4 Object Lifetime & RAII
- Deterministic destruction on normal block exit `{ ... }`.
- Deterministic destruction on early function exit via `return`.
- Deterministic destruction on loop control flow via `break` and `continue`.
- Prvalue copy elision in direct initialization and member initialization.
- Temporary expression object destruction at statement boundaries.
- Lifetime tracking with tombstoning (`alive = false`); access to expired objects is trapped.

### 1.5 Copy Semantics & Assignment
- Distinct copy construction and copy assignment operations.
- Synthesized memberwise copy construction and copy assignment.
- Self-assignment safety guard (`a = a;` is a safe no-op).
- Rejection of copy assignment on classes containing `const` data members.

### 1.6 Overload Resolution
- Free functions, member functions, constructors, and function templates.
- Parameter conversion ranking:
  - Rank 0: Exact match, reference binding.
  - Rank 1: Value-preserving promotion (`bool`/`char` $\to$ `int`, `float` $\to$ `double`).
  - Rank 2: Standard numeric conversion.
  - Rank 3: String literal to `std::string`.
- Const vs non-const member function candidate ranking.
- Ambiguity detection rejecting unresolvable ties (`semantic: ambiguous overload`).
- Non-template candidate preference over template instantiations upon tie.

### 1.7 Templates
- `template<typename T>` and `template<class T>` declarations.
- Function templates with explicit type arguments (e.g. `add<int>(1, 2)`).
- Function template type deduction from call arguments (scalars, records, nested types).
- Class templates parameterized by type names (e.g. `Box<T>`).
- Deep AST cloning and type substitution engine.
- Instantiation caching based on mangled signature keys.
- On-demand semantic analysis of instantiated bodies with exact instantiation diagnostics.

### 1.8 Preprocessing & Native Bridges
- Object-like `#define`, `#undef`, quoted and bracketed `#include`, cycle detection, `-I` include directories.
- Virtual capability headers:
  - `<iostream>`: Exposes `std::cout` and `std::endl` manipulator with operator `<<`.
  - `<string>`: Exposes `std::string` with concatenation, comparisons, `.size()`, `.length()`, `.empty()`.

---

## 2. Intentional Rejection & Unsupported Features

Constructs outside the documented M2 boundary are explicitly and cleanly rejected:
1. **Pointers:** Deliberately unsupported in M2 (`T* p`, `->`, `&`, `*`). Member access via `.` and references via `T&` are fully supported. Pointers are planned for M3's memory model.
2. **Inheritance & Virtual Functions:** Single/multiple inheritance and dynamic dispatch (`virtual`) are rejected at parse time.
3. **Exception Handling:** `throw`, `try`, and `catch` are rejected at parse time.
4. **Advanced Templates:** Non-type template parameters (`template<int N>`), variadics (`typename...`), partial specialization, concepts, and SFINAE are unsupported.
5. **Operator Overloading:** User-defined assignment (`operator=`) and custom arithmetic/comparison operators are unsupported.
6. **Other C++ Constructs:** Move semantics (`T&&`), lambdas, namespaces, `auto`, `enum`, `union`, dynamic memory (`new`/`delete`), casts (`static_cast`), static member variables, arrays of class objects, and general STL headers.
