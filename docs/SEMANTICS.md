# Semantic Rules and Evaluation Model (Milestone M2)

## 1. Static Analysis and Execution Separation

Praxis enforces strict separation between static analysis and runtime evaluation:
1. **Frontend Parsing:** Constructs a typed Abstract Syntax Tree (AST).
2. **Semantic Analysis (`Analyzer`):** Resolves symbols, checks types, verifies access control, validates member initializer lists, performs overload resolution, instantiates templates on demand, and annotates AST nodes with type and value category information without executing interpreted code.
3. **Runtime Interpretation (`Interpreter`):** The runtime tree-walker consumes only statically verified ASTs.

---

## 2. Object Model and Value Categories

- **Lvalues and Prvalues:** Expressions evaluating to storage locations are marked `lvalue = true`. Literals, arithmetic results, and constructor expressions are prvalues (`lvalue = false`).
- **References:** Lvalue references (`T&`, `const T&`) bind directly to existing `Object` instances. References do not transfer ownership, and destroying a reference alias does not affect the referent's lifetime. Reading or writing through an alias to a destroyed object triggers a runtime diagnostic:
  ```text
  runtime: object lifetime has ended
  ```
- **Copy Elision:** In direct initialization of the form `C c = C(10);`, Praxis detects that the initializer is an un-aliased prvalue of matching type and elides the redundant copy constructor, taking direct ownership of the temporary storage.

---

## 3. Class Member Initialization and Order

- **Declaration Order Invariant:** Data members are always initialized in their order of declaration within the class body, regardless of the textual order in the constructor's member initializer list.
- **Default Member Initializers:** Data members with in-class initializers (e.g. `int x = 5;`) are automatically evaluated if omitted from the constructor's initializer list.
- **Scalar Value Initialization:** An empty member initializer `Member()` executes zero-initialization on scalar fields.
- **Construction Failure Safety:** If an exception or runtime error occurs during member initialization or constructor body execution:
  - Construction is immediately aborted.
  - Sub-objects that completed construction prior to the error are destroyed in reverse order of their construction.
  - The incompletely initialized object is never registered in the lexical scope.

---

## 4. Destructors and Deterministic RAII Lifetime

Destruction follows the exact reverse order of construction:
- **Scope Unwinding:** When leaving a lexical scope, all objects allocated within that scope (`Scope::owned`) are destroyed from newest to oldest (`rbegin()` to `rend()`).
- **Control Flow Boundaries:** Destructors run deterministically on:
  - Normal block exit `{ ... }`.
  - Early function exit via `return` (local objects destroyed before caller receives result).
  - Early loop exit via `break`.
  - Next-iteration step via `continue`.
  - Loop condition termination.
- **Member Cleanup:** An object's destructor body executes first, followed by the automatic destruction of its member objects in reverse declaration order.
- **Double Destruction Prevention:** Object storage is tombstoned upon destruction (`alive = false`), and re-entrant destruction calls are rejected via internal guard flags.

---

## 5. Copy Construction and Copy Assignment

- **Copy Construction:** Allocates a new `Object` and initializes its fields from a valid, complete source object. Synthesized copy constructors recursively copy-construct member sub-objects.
- **Copy Assignment:** Mutates an existing destination object's fields using values from a source object:
  - Self-assignment (`a = a;`) is detected and treated as a safe no-op.
  - Attempting copy assignment on a class containing `const` data members is rejected at compile time.

---

## 6. Overload Resolution Rules

When resolving function calls, constructor calls, or member method calls:
1. **Candidate Gathering:** Candidates matching the target name are collected into an overload set.
2. **Viability Filtering:** Candidates with incorrect arity, incompatible types, or const-mismatches are discarded.
3. **Conversion Ranking:**
   - **Rank 0 (Exact Match):** Identical types, or binding an lvalue to a reference of matching type and constness.
   - **Rank 1 (Promotion):** `bool`/`char` $\to$ `int`, `float` $\to$ `double`.
   - **Rank 2 (Standard Conversion):** Other numeric conversions.
   - **Rank 3 (String Conversion):** Text literal $\to$ `std::string`.
4. **Pareto Dominance:** Candidate $A$ is chosen over $B$ if $A$ is at least as good as $B$ on all parameters and strictly better on at least one parameter.
5. **Tie-Breaker:** Non-template functions are preferred over template instantiations when conversion ranks are equal.
6. **Ambiguity:** If no unique best candidate exists, analysis terminates with `semantic: ambiguous overload`.

---

## 7. Template Substitution and Instantiation

- **Type Parameters:** Uninstantiated template declarations store type parameters as `Base::Parameter`.
- **Deduction:** When a function template is called without explicit arguments, types are deduced from the call arguments. Contradictory deductions produce a semantic error.
- **Substitution:** A specialized AST is generated via deep cloning, replacing type parameter nodes with concrete types.
- **Caching:** Instantiated specializations are cached by signature key; repeated invocations reuse the cached specialization.
- **Lazy Semantic Validation:** Instantiated function bodies undergo semantic checking on demand. Semantic errors within the instantiated body report both the failure and the instantiation context.

---

## 8. Arithmetic, Conversion and Safety Diagnostics

- Windows LLP64 model: `char` (8-bit signed), `int` (32-bit signed), `long long` (64-bit signed), `float` (binary32), `double` (binary64).
- Arithmetic overflow, division by zero, out-of-range integer conversions, and non-finite floating values are trapped and diagnosed at runtime as safety policies.
