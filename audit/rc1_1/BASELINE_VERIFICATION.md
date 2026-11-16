# Milestone RC1.1 Baseline Verification Report

## 1. Environment and Repository State

- **Repository Root:** `E:\PRAXIS`
- **Active Branch:** `main`
- **Starting Baseline Commit:** `9139b276270cd8579687544b5d0fdb3767cda370` (`feat: complete Milestone M3 delivering Praxis v0.1.0-rc1`)
- **Git Author & Committer:** `Saksham Mishra <mishrasaksham080705@gmail.com>`
- **Host OS:** Windows 11 Pro 64-bit
- **Host Compiler (Oracle):** MSVC 19.43.34808.0 (Visual Studio 2022 Build Tools v143, x64)
- **CMake Version:** 3.31.6 / Ninja 1.12.1
- **Working Tree State at Start:** Clean

## 2. Pre-Modification Verification Run

Before modifying any source code, both Debug and Release verification suites were executed via `tools/verify-m3.ps1`:

### Debug Configuration Baseline
- `ctest` Execution: 6/6 test suites passed (unit, m2_unit, m3_unit, m3_integration, m2_integration, integration) in 27.23s.
- Adversarial Checks: 13/13 passed.
- M1 Differential Verification: 137/137 programs passed.
- M2 Differential Verification: 79/79 programs passed.
- M3 Integration Cases: 55/55 passed.
- M3 Differential Verification: 42/42 programs passed.
- Total Baseline Checks per Configuration: 781 checks.
- Baseline Differential Programs against MSVC: 258 differential programs.

### Release Configuration Baseline
- `ctest` Execution: 6/6 test suites passed in 23.17s.
- Adversarial Checks: 13/13 passed.
- M1 Differential Verification: 137/137 programs passed.
- M2 Differential Verification: 79/79 programs passed.
- M3 Integration Cases: 55/55 passed.
- M3 Differential Verification: 42/42 programs passed.
- Total Baseline Checks per Configuration: 781 checks.

## 3. Architecture & External Compiler Isolation Audit

1. **Independent Execution Engine:**
   - Praxis is a pure tree-walking interpreter written in C++20.
   - It performs hand-written lexical scanning (`src/frontend/lexer.cpp`), recursive descent parsing (`src/frontend/parser.cpp`), typed AST construction, semantic validation and template instantiation (`src/semantics/analyzer.cpp`, `src/semantics/classes.cpp`, `src/semantics/overloads.cpp`), and tree-walking interpretation (`src/runtime/interpreter.cpp`, `src/runtime/objects.cpp`).
   - Binary inspection of `praxis.exe` confirms zero linkage against LLVM, Clang, GCC, or MSVC compiler libraries.
   - System calls in `praxis.exe` are restricted to console I/O (`iostream`, `Windows.h` for working set queries via `GetProcessMemoryInfo`). No `CreateProcess`, `popen`, or `system` invocations exist in Praxis runtime.

2. **Differential Harness Isolation:**
   - MSVC (`cl.exe`) is invoked exclusively as an external differential verification oracle by test harnesses (`tests/differential/run.py`, `tests/m2/differential.py`, `tests/m3/differential.py`).
   - The test harnesses compare native `.exe` output produced by MSVC against `praxis.exe run <source>` byte-for-byte.

## 4. Initial Forensic Findings & Defects Identified

1. **Original Acceptance Program Substitution in M3:**
   - In M3, `examples/m3/accumulator.cpp` and `tests/m3/fixtures/accumulator.cpp` substituted a primitive `int total = 0` accumulator instead of the required template class `Accumulator<T>`.
   - Running the original `Accumulator<T>` with a vector member exposed a semantic limitation in `classes.cpp` where class members of container types attempted to invoke non-existent class constructors.

2. **Container Object Member Initialization:**
   - Member initializers did not distinguish standard containers (`std::vector`, `std::array`, `std::pair`) from user-defined classes in `Analyzer::checkFunction` and `Interpreter::initializeMembers`.

3. **Memory Counter Leak in Vector Record Operations:**
   - In `Interpreter::call` (`native:vector.push_back`) and `Interpreter::declare` (vector braced initializers), an object cell was allocated via `allocate(elemTy, true)` and then immediately overwritten by `copyObject(src, elemTy, "", l)`, abandoning the original object cell and leaking an object handle from `liveObjects`.
   - In `Interpreter::assignObject`, container assignment previously cleared elements without calling `destroy(*it)` on previous elements, preventing their destructors from executing.

4. **Vector Resize Upward Default Construction:**
   - Upward vector resizing (`v.resize(n)`) allocated memory without invoking the element class default constructor, preventing class elements from being marked `complete` and preventing destructors from running on subsequent downward resize.

5. **Iterator Invalidation on Container Clear and Mutation:**
   - Dereferencing `v.begin()` after `v.clear()` returned the vector object itself rather than diagnosing an out-of-bounds or stale handle error because `target->elements.empty()` was not distinguished from scalar objects.
   - Container generation counters were not incremented on `clear()`, `resize()`, and `push_back()`.
