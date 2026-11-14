# M2 Baseline Reconnaissance and Architectural Audit

**Milestone:** Project PRAXIS — Milestone M2  
**Baseline Commit:** `389ecb585ef075113580f69aaf17f6169dacd46c`  
**Target Branch:** `main`  
**Host Platform:** Windows 11 (x64) / MSVC 2022 (Toolchain v143) / CMake 3.30.5 / Ninja 1.12.1  

---

## 1. Executive Summary

Before implementing Milestone M2, a complete reconnaissance of the Project PRAXIS M1 codebase was performed. The starting state was verified by rebuilding both `Debug` and `Release` configurations under MSVC and executing the entire M1 test and verification harness.

The baseline verification established:
- **Build Configurations:** MSVC 2022 x64, `/Od /Z7 /RTC1` (Debug) and `/O2` (Release), with strict flags `/W4 /permissive- /EHsc /utf-8`.
- **Frontend Unit Checks:** 15/15 assertions passed in both configurations.
- **Integration Test Cases:** 244/244 test fixtures passed in both configurations.
- **Adversarial/Environment Checks:** 13/13 security, boundary, and memory checks passed.
- **Differential Programs:** 134/134 well-defined programs produced byte-for-byte identical output and return codes when compared against native MSVC (`cl.exe`) executions.
- **Total Baseline Verification Count:** 406 verification checks/programs per configuration (812 total across Debug and Release). Raw logs and SHA-256 digests are recorded in `audit/m2/baseline-*`.

Praxis is confirmed to be an independent, self-contained C++ tree-walking interpreter. It contains **zero** compiler backends (no LLVM, Clang, GCC, or MSVC runtime invocation during interpretation). External compilers are utilized strictly as black-box oracles in external Python differential harnesses.

---

## 2. Baseline Architecture Subsystem Inspection

### 2.1 Lexer and Preprocessing Implementation
- **Source Location:** `include/praxis/frontend/lexer.hpp`, `src/frontend/lexer.cpp`, `include/praxis/frontend/preprocessor.hpp`, `src/frontend/preprocessor.cpp`.
- **Lexer:** Hand-written lexical scanner. Generates typed `Token` structures containing token kind, source text, length, and precise `Location` (file, line, column). Distinguishes literal content from grammatical spelling.
- **Preprocessor:** Token-based preprocessor supporting object-like `#define`, `#undef`, recursive expansion suppression, and bracketed/quoted `#include` with cycle detection.
- **Virtual Native Headers:** `<iostream>` and `<string>` are intercepted by the preprocessor and flagged as available capability tokens without loading host system headers.
- **M1 Boundary:** No template angle bracket disambiguation (`<...>` vs operator `<`); no `class` or `struct` member parsing; no user namespaces.

### 2.2 AST Hierarchy
- **Source Location:** `include/praxis/ast/ast.hpp`, `src/ast/type.cpp`.
- **Type Representation:** Struct `Type` storing `Base` enum (`Void`, `Bool`, `Char`, `Int`, `Long`, `Float`, `Double`, `String`, `Text`, `Stream`, `Endl`), `constant`, `reference`, and fixed array `extent`.
- **Expressions:** Struct `Expr` (`Kind::Literal`, `Name`, `Unary`, `Binary`, `Assign`, `Call`, `Index`, `Postfix`). Expressions carry inferred `Type`, `lvalue` boolean category, AST height, and argument vectors.
- **Statements:** Struct `Stmt` (`Kind::Block`, `Declaration`, `Expression`, `If`, `While`, `Do`, `For`, `Return`, `Break`, `Continue`, `Empty`).
- **Declarations:** Struct `Decl` carrying token name, type descriptor, initializers, array bounds, and direct initialization status.
- **Functions:** Struct `Function` carrying name, return type, formal parameter declarations, and statement body.
- **M1 Gap:** No AST nodes for `class`/`struct` definitions, access specifiers (`public`, `private`), constructors, destructors, member initializer lists, member access (`.`), template parameters, or template instantiations.

### 2.3 Parser
- **Source Location:** `include/praxis/frontend/parser.hpp`, `src/frontend/parser.cpp`.
- **Grammar Architecture:** Recursive-descent parser for statements and declarations; precedence-climbing parser for binary and unary expressions.
- **Precedence Hierarchy:** Implements canonical C++ operator precedence (postfix, unary prefix, multiplicative, additive, relational, equality, logical AND, logical OR, assignment). Dangling `else` binds to the nearest enclosing `if`.
- **M1 Gap:** Failed on encountering `class`, `struct`, `template`, access control keywords, and member declarations.

### 2.4 Semantic Analyzer and Type Representation
- **Source Location:** `include/praxis/semantics/analyzer.hpp`, `src/semantics/analyzer.cpp`.
- **Scope Model:** Symbol tables tracking object names, types, constant qualifiers, and initializations.
- **Type Checking:** Strict type conversions, arithmetic promotion rules (bool/char to int, float to double), array bound enforcement, constant mutation checks, structural return validation on all control paths.
- **Function Registry:** M1 utilized a single-entry map (`std::map<std::string, Function>`), mapping each function name to exactly one declaration.
- **M1 Gap:** No overload resolution; no candidate ranking; rejected multiple functions with the same identifier; no class member lookup; no template substitution or instantiation logic.

### 2.5 Runtime Value and Object Model
- **Source Location:** `include/praxis/runtime/value.hpp`, `src/runtime/value.cpp`.
- **Value:** `Value` variant containing `Type` and data payload (`std::int64_t`, `double`, `std::string`, `std::shared_ptr<Object>`).
- **Object:** Heap-allocated `Object` structure representing an lvalue storage location. Contains `Type`, current `Value`, `initialized` flag, `alive` flag, and child `elements` for arrays.
- **Object Identity:** Objects possess distinct memory addresses. References alias existing `Object` instances without transferring ownership.
- **M1 Gap:** No class object layout; no member storage map; no destruction lifecycle states (`destroying`, `complete`); no support for class copy construction or copy assignment.

### 2.6 Stack Frames and Lexical Scopes
- **Source Location:** `include/praxis/runtime/interpreter.hpp`, `src/runtime/interpreter.cpp`.
- **Frame:** Each function invocation pushes a `Frame` holding a stack of lexical `Scope` objects.
- **Scope:** Contains `names` (name-to-object lookup map) and `owned` (list of allocated `Object` pointers created in that scope).
- **Lookup:** Resolves identifiers by searching the current frame's scopes from innermost to outermost, then falling back to globals. Caller stack frames are completely invisible.
- **M1 Gap:** Scope exit in M1 simply marked `alive = false` on owned objects. It did not execute user destructors or perform reverse-order cleanup.

### 2.7 Reference Representation
- **Source Location:** `src/runtime/interpreter.cpp` (`declare`, `lookup`, `call`).
- **Binding:** Reference declarations bind directly to the target `Object` pointer in the current scope's `names` map. The referenced object is NOT added to `owned`, ensuring the alias does not own or alter the referent's lifetime.
- **Dangling Prevention:** Reading through a reference whose referent has `alive == false` triggers an immediate runtime diagnostic (`"object lifetime has ended"`).

### 2.8 Function Call Execution
- **Source Location:** `src/runtime/interpreter.cpp` (`Interpreter::call`).
- **Mechanism:** Evaluates arguments left-to-right. Parameters passed by value allocate new `Object` instances in the callee frame; reference parameters alias caller objects.
- **Execution:** Dispatches function body statements. Flow propagation captures `Flow::Return`, converts return value to declared result type, cleans up callee frame scopes, and returns the result.

### 2.9 CLI and REPL Implementation
- **Source Location:** `src/cli/main.cpp`.
- **CLI Commands:** Supports `--version`, `--help`, `repl`, `run FILE [-I DIR]`, `check FILE [-I DIR]`.
- **REPL:** Interactive read-eval-print loop with multi-line input tracking (balanced braces and quotes), incremental AST parsing, semantic analysis against cumulative declaration history, and execution of new items. Supports `:reset` and `:quit`.

### 2.10 Differential Test Harness
- **Source Location:** `tests/differential/run.py`, `tests/integration/run.py`, `tests/integration/adversarial.py`.
- **Harness:** Python test harness that executes Praxis against test fixtures, records stdout, stderr, and exit codes. For differential tests, compiles the same fixtures with MSVC (`cl.exe /std:c++20 /EHsc /Od /Zc:nrvo-`) and asserts 100% equivalence.

### 2.11 M1 Limitations
- No user-defined classes or structs.
- No constructors or destructors.
- No member access or member functions.
- No function or constructor overloading.
- No templates (neither function templates nor class templates).
- No RAII cleanup on scope exit, return, break, or continue.
- No copy construction or assignment semantics for composite types.
- Fixed resource budgets without phase-specific profiling or limit configuration.

---

## 3. M2 Architectural Requirements & Planned Transitions

To achieve Milestone M2 compliance, the following architectural transformations are required:

| Subsystem | M1 Baseline | M2 Target Architecture |
| :--- | :--- | :--- |
| **Type System** | Scalar types, arrays, strings (`Base` enum) | Added `Base::Record` and `Base::Parameter`; parameterized types with `arguments` vector |
| **AST Hierarchy** | Expressions, Statements, Functions | Added `Class`, `MemberInit`, `Decl` extensions, member expressions, constructor calls |
| **Class Model** | None | Full `class` and `struct`, access control, member methods, implicit `this` |
| **Constructors** | None | Default, parameterized, member initializer lists (declaration order), copy ctor |
| **Destructors & RAII**| Storage expiration (`alive = false`) | Explicit reverse-order destruction on normal exit, `return`, `break`, `continue` |
| **Overloading** | Single function per name | Multi-candidate overload sets, conversion ranking (0-3), ambiguity detection |
| **Templates** | None | `template<class T>`, type substitution, AST cloning, instantiation caching, deduction |
| **CLI & Profiling** | Basic execution | `--profile` JSON telemetry, configurable `--max-*` resource limits |

---

## 4. Verification Baseline Evidence Summary

Prior to making any M2 changes, the baseline was committed and recorded:
- **Baseline Git Commit:** `389ecb585ef075113580f69aaf17f6169dacd46c`
- **Verification Logs:**
  - `audit/m2/baseline-debug-build.log`
  - `audit/m2/baseline-debug-verify.log`
  - `audit/m2/baseline-release-build.log`
  - `audit/m2/baseline-release-verify.log`
  - `audit/m2/baseline-differential-Debug.json` (134 programs)
  - `audit/m2/baseline-differential-Release.json` (134 programs)
  - `audit/m2/baseline-integration-Debug.json` (244 cases)
  - `audit/m2/baseline-integration-Release.json` (244 cases)
  - `audit/m2/baseline-adversarial-Debug.json` (13 checks)
  - `audit/m2/baseline-adversarial-Release.json` (13 checks)

*Note on Overload Rejection Test Migration:* In M1, fixture `overload.cpp` was a negative test asserting that multiple functions sharing an identifier were rejected. For M2, where free-function overloading is a core requirement, this fixture's expectation was promoted to success. With this promotion, the M1 differential regression suite comprises 135 passing programs.
