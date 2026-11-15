# Praxis Architecture (v0.1.0-rc1 / Milestone M3)

```text
SourceManager -> Preprocessor + Lexer -> Token Stream -> Parser -> AST
                                                                |
                                                 Analyzer (Scopes / Classes / Templates / Overloads / Pointers)
                                                                |
                                                      Interpreter (Tree Walk)
                                                       /                  \
                                     Object Storage & Handles (RAII)      Native Bridges & STL Subset
```

Praxis is an independent, modular, self-contained C++ tree-walking interpreter. It contains no compiler backends, generates no intermediate or machine code, and does not invoke external compilers during interpreted program execution.

---

## 1. Core Subsystems

### 1.1 Source and Frontend
- `common/source.hpp`, `frontend/source.cpp`: Source file loading, location tracking (`Location` with file, line, column), and phase-tagged diagnostic formatting (`preprocess:`, `parse:`, `semantic:`, `runtime:`).
- `frontend/lexer.*`: Hand-written lexical scanner. Produces typed `Token` streams distinguishing keywords, literal payloads, operators (including `->`, `++`, `--`, `::`), identifiers, and punctuators.
- `frontend/preprocessor.*`: Token-level preprocessor supporting object-like `#define`, `#undef`, recursive expansion suppression, cycle detection, and virtual includes (`<iostream>`, `<string>`, `<vector>`, `<utility>`, `<algorithm>`, `<array>`).
- `frontend/parser.*`: Recursive-descent parser for statements, classes, structs, templates, pointers, and range-based for loops; precedence-climbing parser for binary and unary expressions (`&`, `*`, `->`, `new`, `delete`, `{...}`).

### 1.2 AST and Type Hierarchy
- `ast/ast.hpp`, `ast/type.cpp`:
  - `Type`: Descriptor supporting primitive scalar types, strings, arrays, composite records (`Base::Record`), template parameters (`Base::Parameter`), pointers (`pointer` depth, `pointeeConstant`, `constant`), and `Base::Nullptr`.
  - `Expr`: Expression nodes (`Literal`, `Name`, `Unary`, `Binary`, `Assign`, `Call`, `Index`, `Postfix`, `Member`, `Construct`, `New`, `Delete`, `InitList`).
  - `Stmt`: Statements (`Block`, `Declaration`, `Expression`, `If`, `While`, `Do`, `For`, `Return`, `Break`, `Continue`, `Empty`, `RangeFor`).
  - `Class`: Class/struct schema containing declaration-ordered `fields`, `methods`, access control flags, template parameters, and special-member IDs.
  - `Function`: Function/method declarations carrying return type, formal parameters, statement body, member initializer list, and const/constructor/destructor flags.

### 1.3 AST Cloning & Substitution Engine
- `include/praxis/ast/substitution.hpp`, `src/ast/substitution.cpp`:
  - Implements deep AST duplication and recursive type substitution for template instantiation.
  - Rewrites dependent type parameters to concrete specialized types, preserving pointer indirection levels and dynamic allocation pointee types (`c->type = substitute(e->type, m)`).

### 1.4 Semantic Analyzer & Type Checker
- `semantics/analyzer.hpp`, `src/semantics/analyzer.cpp`:
  - Symbol table management across lexical scopes, member validation, const-correctness, lvalue enforcement, pointer type validation, and structural return path checking.
- `src/semantics/classes.cpp`:
  - Class definition validation, access control verification, member initializer list reordering into declaration order, synthesized default/copy constructor generation, pair field access, and incomplete pointer support.
- `src/semantics/overloads.cpp`:
  - Overload resolution for functions, constructors, methods, and templates with ranking (Ranks 0–3), pointer conversion ranking, and native STL dispatch (`std::string`, `std::vector`, `std::pair`, algorithms).

### 1.5 Runtime Execution & Abstract Handle Storage
- `runtime/value.hpp`, `src/runtime/value.cpp`:
  - `Value`: Runtime data variant (`int64`, `double`, `string`, `std::shared_ptr<Object>`, `Pointer`).
  - `Pointer`: Abstract validated reference (`targetId`, `generation`, `target`, `offset`, `isNull`). Prevents host address exposure.
  - `Object`: Lvalue storage unit storing concrete type, scalar payload, `initialized`, `alive`, `complete`, `id`, `generation`, `isHeap`, and `deleted` flags, member map (`fields`), and elements vector (`elements`).
- `runtime/interpreter.hpp`, `src/runtime/interpreter.cpp`, `src/runtime/objects.cpp`:
  - Execution of statements and expressions via tree walking.
  - Dynamic memory lifecycle (`new`, `delete`), null deletion safety, and double-free detection.
  - Deterministic reverse-order RAII destruction (`Interpreter::cleanup()`).
  - Stack frame management (`Frame`), lexical scope stack (`Scope`).
  - Lowering of range-based for loops (`RangeFor`) over vector, array, and string.

### 1.6 Native STL Bridges
- `native/bindings.hpp`, `src/native/bindings.cpp`:
  - Bridges virtual `<iostream>` (`std::cout`, `std::endl`), `<string>`, `<vector>`, `<utility>`, `<algorithm>`, and `<array>` operations directly to host C++ facilities.

### 1.7 CLI, REPL & Profiling
- `cli/main.cpp`:
  - Command dispatcher (`--version`, `--help`, `repl`, `run`, `check`).
  - Incremental REPL submission session manager with state reset (`:reset`).
  - JSON execution profiler (`--profile`) tracking dynamic allocations, deallocations, and container operations alongside configurable limits (`--max-*`).

---

## 2. Verification Architecture

- `tests/unit/`: Direct C++ frontend unit checks (`praxis_unit.exe`, `praxis_m2_unit.exe`, `praxis_m3_unit.exe`).
- `tests/integration/`: High-coverage fixture execution for M1 regression (`tests/integration/run.py`), M2 regression (`tests/m2/run.py`), and M3 features (`tests/m3/run.py`).
- `tests/differential/`: External differential testing against native MSVC (`cl.exe /std:c++20 /EHsc /Od /Zc:nrvo-`). Compares standard output, exit codes, and determinism across 258 differential programs.
- Automated verification: `tools/verify-m3.ps1` runs CTest, adversarial suites, and differential suites in Debug and Release.
