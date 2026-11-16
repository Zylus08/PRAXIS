# Milestone RC1.1 `std::move` and Value Categories Audit

## 1. Scope and Value Category Model

Praxis implements a clearly defined expression value category model:
- **Lvalues (`e->lvalue == true`):**
  Named variables, data members of lvalues, array/vector indexing expressions (`v[i]`), pointer dereference expressions (`*p`), prefix increment/decrement (`++x`), and reference-bound bindings.
- **Rvalues / Non-lvalues (`e->lvalue == false`):**
  Literal constants, arithmetic and boolean operation results, functional constructions, returned temporary objects, and expressions cast via `std::move(e)`.

## 2. Semantics of `std::move`

In ISO C++20, `std::move(t)` is an unconditional cast to an rvalue reference (`static_cast<std::remove_reference_t<T>&&>(t)`).

In Praxis v0.1.0:
1. **Semantic Analysis (`src/semantics/overloads.cpp`):**
   ```cpp
   if (name == "std::move" || name == "move") {
       if (args.size() != 1) fail(e->token.loc, "semantic", "std::move requires 1 argument");
       e->target = "native:move";
       auto res = args[0]->type;
       res.reference = false;
       e->lvalue = false; // casts expression from lvalue to rvalue
       return res;
   }
   ```
2. **Execution (`src/runtime/interpreter.cpp`):**
   ```cpp
   if (e->target.starts_with("native:move")) {
       return eval(e->args[1]);
   }
   ```
   The evaluated value is propagated without lvalue identity.

## 3. Copy vs. Move Constructor Selection

### Implemented Subset vs. Unsupported ISO Features
- **Supported:**
  - Copy constructors with signature `C::C(const C&)` or `C::C(C&)`.
  - Copy assignment operators.
  - Passing rvalues to by-value parameters via copy constructor or direct temporary elision.
  - `std::move` on scalars, pointers, and class objects.
- **Deliberately Restricted / Unsupported:**
  - Rvalue references (`T&&`) are not recognized by the type parser or analyzer.
  - Move constructors (`C::C(C&&)`) and move assignment operators (`C::operator=(C&&)`) are **not implemented**.
  - When `std::move(x)` is passed to a constructor or function, overload resolution binds to `const C&` (the copy constructor) in accordance with ISO rules when move constructors are absent.
  - Move-only types (such as `std::unique_ptr` or move-only user classes) are **not supported**.

## 4. Moved-From State Guarantees

Because Praxis treats `std::move` in this subset as an rvalue cast falling back to copy construction:
1. The source object remains completely valid and retains its value (equivalent to a copy).
2. For scalar types, values are copied bitwise.
3. For `std::vector` and classes containing containers, deep element copies are performed.
4. No indeterminate, uninitialized, or partially-moved states are produced.

## 5. Formal Certification Ledger Entry

Praxis v0.1.0 certifies:
- `std::move` is available via `<utility>` as an rvalue expression adapter.
- Value category transitions (`lvalue -> rvalue`) are strictly checked by semantic analysis.
- Praxis does **NOT** claim support for C++11/C++20 rvalue references (`T&&`) or user-defined move constructors. Move operations fall back to copy construction within the documented language subset.
