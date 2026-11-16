# Milestone RC1.1 Parser and Semantic Integrity Audit

## 1. Parser Architecture and Grammar Integrity

The Praxis parser (`src/frontend/parser.cpp`) is a recursive descent parser operating over tokens produced by the lexical scanner (`src/frontend/lexer.cpp`) and macro preprocessor (`src/frontend/preprocessor.cpp`).

### Key Grammar Rules Verified:
1. **Pointers & Qualifiers:**
   `int*`, `const int*`, `int* const`, `const int* const`, `int**`, `T*&`.
   The parser accurately distinguishes pointee constness (`t.pointeeConstant`) from pointer constness (`t.constant`).
2. **Templates:**
   - Function templates: `template<typename T> T add(T a, T b) { ... }`
   - Class templates: `template<typename T> class Stack { ... };`
   - Explicit instantiation and automatic template argument deduction from call arguments.
   - Disambiguation of `<` and `>` in expressions vs template argument lists via lexical lookahead and record symbol tracking.
3. **Control Structures:**
   - Standard `if`/`else`, `while`, `do`/`while`, classical `for (init; cond; step)`.
   - Range-based `for (T x : container)` and `for (auto x : container)`.
4. **Member and Dynamic Access:**
   - Object member access `.`, pointer member access `->`.
   - Operator `new` and `delete` expressions.

## 2. Semantic Analysis & Type System

Semantic checking (`src/semantics/analyzer.cpp`, `classes.cpp`, `overloads.cpp`) performs full static validation before execution:
1. **Type Resolution:**
   Ensures type parameters are substituted, nested types resolved, and unknown symbols diagnosed.
2. **Access Control:**
   Strictly enforces `public` and `private` member accessibility, distinguishing `class` (default private) and `struct` (default public).
3. **Const Correctness:**
   - Calling non-const member functions on `const` class instances is rejected at analysis time.
   - Mutating members inside `const` methods is rejected.
   - Pointers to const pointees cannot be assigned to non-const pointee pointers without conversion errors.
4. **Overload Resolution Ranking:**
   - Rank 0: Exact match (including reference binding to compatible lvalue).
   - Rank 1: Standard promotions (bool/char to int, float to double, nullptr to pointer, pointer to const pointer).
   - Rank 2: Standard numeric conversions.
   - Rank 3: String conversions from literal text.
   - Ambiguous overloads and ties are cleanly rejected with informative error messages.

## 3. Diagnostic Reporting and Format

All compilation and analysis diagnostics conform to standard compiler diagnostic format:
```text
<filename>:<line>:<column>: <stage>: <message>
```
Stages include:
- `lex`: Unterminated character or string constants, unrecognized characters.
- `preprocessor`: Missing header files, macro syntax errors.
- `parse`: Unexpected tokens, missing semicolons, malformed declarations.
- `semantic`: Type mismatches, private access violations, unresolved symbols, const violations.
- `runtime`: Division by zero, out-of-bounds access, dereferencing expired or null pointers.

## 4. Adversarial Edge Cases Verified in RC1.1

1. **Template Specialization with Nested Containers:**
   `HistoryAccumulator<int>` containing `std::vector<int>` correctly synthesizes member initializers without mistaking `std::vector` for a user-defined class.
2. **Template Deduction from Container References:**
   `template<typename T> T sum_container(const std::vector<T>& v)` successfully deduces `T` from `std::vector<int>` by matching parameter record names and recursing into template argument lists.
3. **Pointer Pointee Const Overload Selection:**
   Functions overloaded on `const T*` vs `T*` select the most specific viable candidate based on pointee constness ranking.
4. **Elision of Prvalue Temporaries:**
   Direct initialization from prvalue objects elides redundant temporary creation while preserving constructor/destructor accounting.
