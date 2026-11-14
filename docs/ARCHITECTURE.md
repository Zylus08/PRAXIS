# Praxis Architecture (Milestone M2)

```text
SourceManager -> Preprocessor + Lexer -> Token Stream -> Parser -> AST
                                                                |
                                                 Analyzer (Scopes / Classes / Templates / Overloads)
                                                                |
                                                      Interpreter (Tree Walk)
                                                       /                  \
                                              Object Storage (RAII)      Native Bindings
```

Praxis is an independent, modular, self-contained C++ tree-walking interpreter. It contains no compiler backends, generates no intermediate or machine code, and does not invoke external compilers during interpreted program execution.

---

## 1. Core Subsystems

### 1.1 Source and Frontend
- `common/source.hpp`, `frontend/source.cpp`: Source file loading, location tracking (`Location` with file, line, column), and phase-tagged diagnostic formatting (`preprocess:`, `parse:`, `semantic:`, `runtime:`).
- `frontend/lexer.*`: Hand-written lexical scanner. Produces typed `Token` streams distinguishing keywords, literal payloads, operators, identifiers, and punctuators.
- `frontend/preprocessor.*`: Token-level preprocessor supporting object-like `#define`, `#undef`, recursive expansion suppression, cycle detection, and virtual includes (`<iostream>`, `<string>`).
- `frontend/parser.*`: Recursive-descent parser for statements, classes, structs, and templates; precedence-climbing parser for binary and unary expressions. Operates without external parser generator dependencies.

### 1.2 AST and Type Hierarchy
- `ast/ast.hpp`, `ast/type.cpp`:
  - `Type`: Descriptor supporting primitive scalar types, strings, arrays, composite records (`Base::Record`), and template parameters (`Base::Parameter`). Carries `constant`, `reference`, fixed `extent`, and template `arguments`.
  - `Expr`: Expression nodes (`Literal`, `Name`, `Unary`, `Binary`, `Assign`, `Call`, `Index`, `Postfix`, `Member`, `Construct`). Carries inferred `type`, `lvalue` value category, AST height, and overload targets.
  - `Stmt`: Statements (`Block`, `Declaration`, `Expression`, `If`, `While`, `Do`, `For`, `Return`, `Break`, `Continue`, `Empty`).
  - `Class`: Class/struct schema containing declaration-ordered `fields`, `methods`, access control flags, template parameters, and special-member IDs.
  - `Function`: Function/method declarations carrying return type, formal parameters, statement body, member initializer list, and const/constructor/destructor flags.

### 1.3 AST Cloning & Substitution Engine
- `include/praxis/ast/substitution.hpp`, `src/ast/substitution.cpp`:
  - Implements deep AST duplication and recursive type substitution for template instantiation.
  - Rewrites dependent type parameters to concrete specialized types without mutating uninstantiated template patterns.

### 1.4 Semantic Analyzer & Type Checker
- `semantics/analyzer.hpp`, `src/semantics/analyzer.cpp`:
  - Symbol table management across lexical scopes, member validation, const-correctness, lvalue enforcement, and structural return path checking.
- `src/semantics/classes.cpp`:
  - Class definition validation, access control verification, member initializer list reordering into declaration order, synthesized default/copy constructor generation, and destructor registration.
- `src/semantics/overloads.cpp`:
  - Function, constructor, and method overload resolution. Viability filtering, conversion ranking (Ranks 0–3), template argument deduction, and ambiguity detection.

### 1.5 Runtime Execution & Object Storage
- `runtime/value.hpp`, `src/runtime/value.cpp`:
  - `Value`: Runtime data variant (`int64`, `double`, `string`, `std::shared_ptr<Object>`).
  - `Object`: Lvalue storage unit storing concrete type, scalar payload, `initialized`, `alive`, and `complete` lifecycle flags, name-indexed member map (`fields`), and declaration-ordered member list (`members`).
- `runtime/interpreter.hpp`, `src/runtime/interpreter.cpp`, `src/runtime/objects.cpp`:
  - Execution of statements and expressions via tree walking.
  - Stack frame management (`Frame`), lexical scope stack (`Scope`).
  - Chronological ownership tracking (`Scope::owned`) and deterministic reverse-order RAII destruction (`Interpreter::cleanup()`).
  - Prvalue copy elision and temporary expression lifecycle management (`cleanupTemporaries`).

### 1.6 Native Runtime Bridges
- `native/bindings.hpp`, `src/native/bindings.cpp`:
  - Bridges virtual `<iostream>` (`std::cout`, `std::endl`) and `<string>` (`std::string`) operations directly to host C++ facilities.

### 1.7 CLI, REPL & Profiling
- `cli/main.cpp`:
  - Command dispatcher (`--version`, `--help`, `repl`, `run`, `check`).
  - Incremental REPL submission session manager with state reset (`:reset`).
  - JSON execution profiler (`--profile`) and configurable resource limit flags (`--max-*`).

---

## 2. Verification Architecture

- `tests/unit/`: Direct C++ frontend unit checks (`praxis_unit.exe`, `praxis_m2_unit.exe`).
- `tests/integration/`: High-coverage fixture execution verifying positive execution, semantic rejection, unsupported language rejection, and runtime safety.
- `tests/differential/`: Independent differential testing against native MSVC (`cl.exe /std:c++20 /EHsc /Od /Zc:nrvo-`). Compares standard output, exit codes, and determinism.

---

## 3. M3 Extension Points

Future milestones will extend Praxis into standard library abstractions and dynamic memory management:
- Pointer types, address-of (`&`), dereference (`*`), pointer-to-member (`->`), and null pointer values.
- Heap memory model with dynamic allocation (`new` / `delete`).
- Container runtime models supporting interpreted `std::vector<T>`, `std::array<T, N>`, and `std::pair`.
- Standard algorithms and move semantics (`std::move`).
