# Semantic Rules and Evaluation Model (v0.1.0-rc1 / Milestone M3)

## 1. Static Analysis and Execution Separation

Praxis enforces strict separation between static analysis and runtime evaluation:
1. **Frontend Parsing:** Constructs a typed Abstract Syntax Tree (AST).
2. **Semantic Analysis (`Analyzer`):** Resolves symbols, checks types, verifies access control, validates member initializer lists, performs overload resolution, instantiates templates on demand, and annotates AST nodes with type, value category, and overload information without executing interpreted code.
3. **Runtime Interpretation (`Interpreter`):** The runtime tree-walker consumes only statically verified ASTs.

---

## 2. Object Model and Value Categories

- **Lvalues and Prvalues:** Expressions evaluating to storage locations are marked `lvalue = true`. Literals, arithmetic results, and constructor expressions are prvalues (`lvalue = false`).
- **References:** Lvalue references (`T&`, `const T&`) bind directly to existing `Object` instances. References do not transfer ownership, and destroying a reference alias does not affect the referent's lifetime. Reading or writing through an alias to a destroyed object triggers a runtime diagnostic: `runtime: object lifetime has ended`.
- **Pointers vs References:** Unlike references, pointers are first-class values stored in `praxis::Value` as abstract handles (`targetId`, `generation`, `target`, `offset`). Pointers can be reassigned, point to null, and undergo arithmetic.
- **Copy Elision:** In direct initialization of the form `C c = C(10);`, Praxis detects that the initializer is an un-aliased prvalue of matching type and elides the redundant copy constructor, taking direct ownership of the temporary storage.

---

## 3. Class Member Initialization and Order

- **Declaration Order Invariant:** Data members are always initialized in their order of declaration within the class body, regardless of the textual order in the constructor's member initializer list.
- **Default Member Initializers:** Data members with in-class initializers (e.g. `int x = 5;`) are automatically evaluated if omitted from the constructor's initializer list.
- **Scalar Value Initialization:** An empty member initializer `Member()` executes zero-initialization on scalar fields.
- **Construction Failure Safety:** If an error occurs during member initialization or constructor body execution:
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
- **Double Destruction Prevention:** Object storage is tombstoned upon destruction (`alive = false`, `generation++`), and re-entrant destruction calls are rejected via internal guard flags.

---

## 5. Pointer Semantics & Abstract Handle Invariants

- **Address-Of (`&`):** Produces a `Pointer` referencing the lvalue operand's `id` and current `generation`, with `offset = 0`.
- **Dereference (`*`):**
  1. Validates `!ptr.isNull` and `ptr.targetId != 0` (otherwise `null pointer dereference`).
  2. Validates `!target->deleted` (otherwise `use after free`).
  3. Validates `target->alive` (otherwise `dereference of expired object`).
  4. Validates `target->generation == ptr.generation` (otherwise `dereference through stale pointer handle`).
  5. If `target` has elements (`vector`, `array`), bounds-checks `0 <= ptr.offset < target->elements.size()` and returns the element at `ptr.offset`.
- **Pointer Arithmetic:**
  - `ptr + n` and `ptr - n` modify `offset` without changing `targetId` or `generation`.
  - `p2 - p1` computes difference between offsets when pointing to the same container object.
  - Comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=`) evaluate identity, generation, and relative offset.

---

## 6. Dynamic Storage Allocation & Deallocation

- **`new T` / `new T(...)`:**
  - Enforces quota `liveObjects < limits.objects`.
  - Allocates heap object with `isHeap = true` and `generation = 1`.
  - Executes scalar initialization or constructor overload.
  - Tracks metric `++profile.dynamicAllocations`.
  - Returns pointer handle.
- **`delete ptr`:**
  - If `ptr` is null, safely returns as a no-op without incrementing deallocations.
  - Validates `ptr` targets a heap object (`isHeap == true`).
  - Detects double deletion (`target->deleted == true`).
  - Marks `target->deleted = true`, increments `target->generation`, and calls `destroy(target)`.
  - Tracks metric `++profile.dynamicDeletions`.

---

## 7. Container & Range-Based For Semantics

- **Range-For Lowering:**
  - `for (decl : range) stmt` evaluates `range` once.
  - Iterates over container elements from `0` to `size - 1`.
  - For reference declarations (`T&`, `const auto&`), loop variable aliases the element directly.
  - For value declarations (`T`, `auto`), element is deep-copied or value-converted.
  - Nested scope is created and destroyed per iteration.
- **Container Copy & Assignment:**
  - Copy constructor deep-copies each element into fresh storage cells.
  - Copy assignment destroys existing elements and deep-copies elements from source.
