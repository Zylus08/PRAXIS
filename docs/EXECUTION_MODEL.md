# Execution and Lifetime Model (Milestone M2)

## 1. Values and Object Identity

- `Value` contains a semantic type plus a variant data payload (`std::int64_t`, `double`, `std::string`, or `std::shared_ptr<Object>`). The variant is an internal storage vehicle; target-language behavior is governed entirely by explicit type metadata, conversion rules, and operator semantics.
- `Object` represents an lvalue storage location. It stores declared type, initialization status (`initialized`), storage activity (`alive`), construction completeness (`complete`), destruction re-entry guard (`destroying`), scalar payload, array elements, and class member structures (`fields` for name lookup, `members` for declaration-order operations).
- References (`T&`) alias existing `Object` instances without transferring ownership or creating duplicate storage.
- Reading or writing through an alias whose referent has expired (`alive == false`) is trapped and diagnosed at runtime:
  ```text
  runtime: object lifetime has ended
  ```

---

## 2. Stack Frames, Lexical Scopes & Lookup

- Each function call allocates a `Frame` containing a stack of lexical `Scope` objects. Member function calls additionally bind `Frame::self` to the implicit target object.
- Lookup searches the current frame's scopes from innermost to outermost, then falls back to globals. Caller stack frames are completely invisible.
- Unqualified references inside member methods search the local lexical scopes, then fall back to the implicit object's member fields (`Frame::self->fields`), then globals.
- Parameters share the function's outer scope; redeclaring a parameter name in the top-level function block is rejected at semantic analysis time.

---

## 3. Scope Ownership and Deterministic RAII Lifetime

Praxis implements complete, deterministic Resource Acquisition Is Initialization (RAII) semantics:
- Each lexical `Scope` maintains an `owned` list recording heap-allocated `Object` pointers strictly in the chronological order they were constructed. Reference aliases are inserted into `Scope::names`, but **never** into `Scope::owned`.
- When execution leaves a scope, `Interpreter::cleanup(Scope& scope)` unwinds `scope.owned` in **reverse construction order** (`rbegin()` to `rend()`):
  1. The class destructor (if defined) is invoked via `Interpreter::destroy()`.
  2. The object's member variables are destroyed in reverse declaration order.
  3. Array elements are destroyed in reverse index order.
  4. The object is tombstoned (`alive = false`, `complete = false`).
  5. The global live object count is decremented.

### 3.1 Control-Flow Scope Boundaries
- **Normal Block Exit:** Destroys all objects constructed within `{ ... }` upon reaching the closing brace.
- **Return (`return expr`):** Evaluates the return expression, copies an lvalue or detaches a prvalue temporary, unwinds and cleans up all active scopes on the call frame in reverse order, and returns the result to the caller.
- **Loop Control (`break` and `continue`):** Destroys all objects allocated within the current loop iteration before exiting the loop or proceeding to the step/condition.
- **Expression Temporaries:** Temporary objects created during expression evaluation are tracked in `Interpreter::temporaries` and cleaned up at the end of each full-expression statement via `cleanupTemporaries()`.

---

## 4. Construction Failure Safety

If construction is interrupted by an error (e.g. division by zero in an initializer, quota exhaustion):
- The incomplete object never enters the lexical scope.
- Sub-objects that completed construction prior to the failure are destroyed in reverse construction order.
- Incompletely initialized objects are never accessible to subsequent code.

---

## 5. Execution Limits and Profiling

Praxis bounds interpreter resource consumption to prevent host stack exhaustion, uncontrolled memory growth, or infinite loops:
- Step limit: 10,000,000 steps per run (configurable via `--max-steps`).
- Call depth: 128 frames (configurable via `--max-calls`).
- Template instantiation depth: 32 levels (configurable via `--max-template-depth`).
- Total template specializations: 256 instantiations (configurable via `--max-instantiations`).
- Live object limit: 100,000 concurrent objects (configurable via `--max-objects`).
- Parsing and preprocessing limits: AST height 128, expression depth 256, macro recursion 100, expansion size 1,000,000 tokens.

Telemetry is available via `praxis run FILE --profile`, which outputs a JSON breakdown of preprocessing, parsing, semantic analysis, template substitution, runtime dispatch, object constructions, destructors, and peak memory working set.
