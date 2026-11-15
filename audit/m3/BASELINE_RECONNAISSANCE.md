# Milestone M3 Baseline Reconnaissance & Architectural Audit

**Milestone:** Project PRAXIS — Milestone M3 (Release Candidate)  
**Starting Commit:** `f80da66`  
**Host Platform:** Windows 11 Enterprise x64 / MSVC 2022 (Toolchain v143) / CMake 3.30.5 / Ninja 1.12.1  
**Verification Baseline Status:** 642/642 tests passing in Debug, 642/642 tests passing in Release.  

---

## 1. Executive Summary

Milestone M3 begins from a fully verified M2 baseline:
- Git working tree is clean on branch `main` at commit `f80da66`.
- Complete test suites for M1 and M2 were re-executed in both `Debug` and `Release` configurations.
- All 15 frontend unit tests, 31 M2 unit tests, 244 M1 integration tests, 126 M2 integration tests, 13 adversarial environment checks, 135 M1 differential programs, and 78 M2 differential programs passed with zero failures.
- Zero compiler toolchains or precompiled engines are invoked during interpreter runtime execution.

---

## 2. Subsystem Inspection and M3 Gap Analysis

### 2.1 Pointer Model & Memory Indirection
- **Current State (M2):** Pointers (`T*`, `&`, `*`, `->`, `nullptr`) are explicitly rejected at parse time. Storage is manipulated solely via direct values or lvalue references (`T&`).
- **Required Architecture (M3):**
  - First-class pointer values in the `Type` system (`bool pointer` or pointer indirection count).
  - Explicit pointer representation in runtime `Value` variant (`PointerValue` carrying target object ID, generation counter, or handle).
  - Address-of operator `&` taking lvalues.
  - Dereference operator `*` producing an lvalue `Object` handle.
  - Arrow operator `->` resolving pointer-to-class members.
  - `nullptr` literal value.
  - Pointer equality/inequality comparisons.
  - Detection of dangling pointers and use-after-free via object generation tracking.

### 2.2 Dynamic Memory Allocation (`new` and `delete`)
- **Current State (M2):** Objects exist only in static (global) or automatic (stack frame) scopes. No heap allocation operators exist.
- **Required Architecture (M3):**
  - `new T` and `new T(args...)` expressions allocating dynamically owned objects.
  - `delete ptr` destroying target object and reclaiming memory.
  - `delete nullptr` safe no-op.
  - Double-deletion detection.
  - Rejection of deleting automatic (stack) objects.
  - Dynamic allocation quota enforcement (`--max-objects`, heap tracking).

### 2.3 Standard Library: `<string>`, `<vector>`, `<utility>`, `<algorithm>`
- **Current State (M2):** Minimal virtual `<string>` and `<iostream>` built-ins. No `<vector>`, `<utility>`, `<algorithm>`, or containers.
- **Required Architecture (M3):**
  - `<vector>`: `std::vector<T>` container supporting scalar and class element types. Must participate in Praxis's object lifecycle machinery (element construction, copying, destruction on pop_back/clear/reallocation).
  - Extended `<string>`: `operator[]`, `at()`, `substr()`, range-based iteration.
  - `<utility>`: `std::pair<T1, T2>`, `std::move`, `std::swap`.
  - `<algorithm>`: `std::find`, `std::count`, `std::sort`, `std::reverse`, `std::min`, `std::max`.
  - Range-based `for` loop lowering: `for (T x : container)` or `for (T& x : container)`.
  - Initializer list syntax: `std::vector<int> v = {10, 20, 30};`.

### 2.4 Representative Program Verification
- Target program: `Accumulator<T>` instantiated with `std::vector<int>` initialized via `{10, 20, 30}`, accumulated through range-based `for`, yielding output `60`.

---

## 3. Architecture Risks and Mitigation Strategies

1. **Host Address Leaks:** Pointers must never expose raw host virtual memory addresses. Pointers will be modeled using abstract object handles containing generation IDs to detect use-after-free and dangling targets.
2. **Double Deletion & Stack Deletion:** The heap manager must tag objects as `StorageKind::Heap` vs `StorageKind::Automatic`. Deleting a non-heap object or already-deleted heap object will be trapped and diagnosed at runtime.
3. **Container Element Lifecycles:** When `std::vector<T>` grows or shrinks, class elements must be constructed and destroyed using Praxis's `Interpreter::construct` and `Interpreter::destroy`.
4. **Range-Based For Lowering:** Range `for` must lower to iterator traversal or indexed access without hardcoding to specific container variable names.
