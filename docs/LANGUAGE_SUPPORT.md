# Praxis Language Boundary (Praxis v0.1.0)

## 1. Implemented C++ Subset

### 1.1 Types and Literals
- `bool`, signed 8-bit `char`, 32-bit `int`, 64-bit `long long`, binary32 `float`, binary64 `double`, `void`, `std::string`, `std::nullptr_t`.
- Decimal integers (optional `LL`/`ll`), floating literals with exponents (optional `f`/`F`), byte character and string literals, `true`/`false`, `nullptr`.
- Escapes: `\n`, `\r`, `\t`, `\0`, `\\`, `\'`, `\"`.
- One-dimensional fixed arrays with literal decimal bounds 1–1,000,000. Brace initialization, zero-filling omitted elements, indexing, element references.
- Lvalue references (`T&`, `const T&`) preserving referent identity.
- First-class pointer types (`T*`, `const T*`, `T* const`, `T**`).
- `auto` type deduction for local variable declarations and range-based for loops.

### 1.2 Typed Pointers & Dynamic Memory
- Address-of operator `&` requiring lvalue operands.
- Dereference operator `*` requiring pointer operands, yielding lvalues.
- Arrow operator `->` lowering to `(*ptr).member`.
- `nullptr` literal and comparisons (`==`, `!=`) and boolean truthiness.
- Pointer arithmetic: `p + n`, `n + p`, `p - n`, `p2 - p1` yielding `ptrdiff_t`, `++`, `--`.
- Pointer relational comparisons (`<`, `<=`, `>`, `>=`) within identical container slices.
- Dynamic object allocation via `new T` and `new T(arguments...)`.
- Dynamic deallocation via `delete p` with null deletion safety and double-free detection.
- Non-heap deletion prevention (rejects deleting automatic variables).

### 1.3 Classes and Structs
- `class` and `struct` definitions with default private/public access rules.
- Access control labels: `public:` and `private:`.
- Data members (scalars, `std::string`, pointers, and nested classes).
- Member functions defined inside class bodies or out-of-class with `ClassName::MethodName`.
- Implicit `this` member resolution within member methods.
- `const` member functions enforcing const-correctness.
- Member access via `.` and `->`.

### 1.4 Constructors and Destructors
- Implicit default constructor synthesis when no user constructor is defined.
- Parameterized constructors with overload resolution.
- Member initializer lists executing strictly in **declaration order**.
- In-class default member initializers (e.g. `int x = 5;`).
- Explicit member value-initialization (`Member()`) executing zero-initialization on scalars.
- Copy constructor synthesis and user-defined copy constructors (`ClassName(const ClassName&)`).
- Destructors (`~ClassName()`) executed automatically in reverse construction order upon scope exit.
- Construction failure unwinding: destroys previously constructed sub-objects without leaking.

### 1.5 Object Lifetime & RAII
- Deterministic destruction on normal block exit `{ ... }`.
- Deterministic destruction on early function exit via `return`.
- Deterministic destruction on loop control flow via `break` and `continue`.
- Prvalue copy elision in direct initialization and member initialization.
- Temporary expression object destruction at statement boundaries.
- Lifetime tracking with generation validation; access to expired objects is caught.
- Dynamic recursive RAII structures (linked lists, trees) with chained destructor deallocation.

### 1.6 Copy Semantics & Assignment
- Distinct copy construction and copy assignment operations.
- Synthesized memberwise copy construction and copy assignment.
- Self-assignment safety guard (`a = a;` is a safe no-op).
- Rejection of copy assignment on classes containing `const` data members.

### 1.7 Standard Library (STL) Subset
- Virtual capability headers:
  - `<iostream>`: `std::cout` and `std::endl` with operator `<<`.
  - `<string>`: `std::string` with `.size()`, `.length()`, `.empty()`, `.at()`, `.substr()`, character indexing.
  - `<vector>`: `std::vector<T>` with default/sized/init-list constructors, copy construction/assignment, `push_back`, `pop_back`, `operator[]`, `at`, `front`, `back`, `size`, `empty`, `clear`, `resize`, `reserve`, `begin`, `end`.
  - `<utility>`: `std::pair<T1, T2>`, `std::make_pair`, `std::swap`, `std::move`.
  - `<algorithm>`: `std::min`, `std::max`, `std::sort`, `std::reverse`, `std::find`, `std::count`.
  - `<array>`: Fixed-size container subset.

### 1.8 Iterators & Range-Based For Loops
- Random access pointer iterator handles for vector and array.
- Lowering of `for (declaration : range) statement` for vector, array, and string.
- Deduction of `auto`, `int`, and reference loop variables.

### 1.9 Overload Resolution
- Free functions, member functions, constructors, and function templates.
- Parameter conversion ranking (Ranks 0–3) including pointer conversion ranking.
- Ambiguity detection rejecting unresolvable ties.
- Non-template candidate preference over template instantiations upon tie.

### 1.10 Templates
- `template<typename T>` and `template<class T>` declarations.
- Function templates with explicit type arguments and type deduction.
- Class templates parameterized by type names (e.g. `Box<T>`).
- Template specialization caching and deep AST substitution.
- Recursive struct templates and incomplete pointers (`Node<T>* next`).

---

## 2. Intentional Rejection & Unsupported Features

Constructs outside the documented v0.1.0-rc1 boundary are explicitly and cleanly rejected:
1. **Inheritance & Virtual Functions:** Single/multiple inheritance and dynamic dispatch (`virtual`) are rejected.
2. **Exception Handling:** `throw`, `try`, and `catch` are rejected.
3. **Advanced Templates:** Variadics (`typename... Args`), partial specialization, concepts, and SFINAE are unsupported.
4. **General Operator Overloading:** Custom arithmetic/comparison operators are unsupported (except copy assignment and stream output).
5. **Pointer-to-Member & Placement New:** `T::*` and `new (buf) T` are unsupported.
6. **Other Features:** Lambdas, namespaces, `enum`, `union`, `reinterpret_cast`, and multithreading (`<thread>`).
