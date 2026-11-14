# Template Instantiation Architecture & Type Substitution Engine

**Milestone:** Project PRAXIS — Milestone M2  
**Target Specifications:** Sections 8, 14, 15  

---

## 1. Overview and Architecture

Praxis M2 implements a native C++ template engine operating directly on typed AST structures within the interpreter's frontend and semantic analysis phases. 

**Key Design Principle:** No host C++ compiler or external tooling is involved in template parsing, substitution, semantic checking, or instantiation. All template processing is executed directly by Praxis's own algorithms.

```text
Template Declaration AST (Pattern)
          |
   Call / Specialization
          |
Type Deduction / Explicit Arguments
          |
Instantiation Cache Lookup (Hit -> Reuse Specialization)
          | (Miss)
AST Cloning & Recursive Type Substitution
          |
Instantiated AST Registration
          |
Semantic Analysis of Instantiated Body (On-Demand)
          |
Interpreted Execution (Tree Walking)
```

---

## 2. Template Syntax and Parsing

Praxis supports both standard C++ template parameter declaration syntaxes:
- `template<typename T>`
- `template<class T>`
- Multiple template type parameters: `template<class T, class U>`

### 2.1 Dependent Type Parameter Representation
In uninstantiated template declarations, type parameters are parsed with:
```cpp
Type.base = Base::Parameter;
Type.name = "T"; // The parameter identifier
```
The AST pattern remains immutable throughout execution.

---

## 3. AST Cloning and Type Substitution

Type substitution is implemented via deep AST cloning and type rewriting (`include/praxis/ast/substitution.hpp`, `src/ast/substitution.cpp`):
```cpp
using Substitution = std::map<std::string, Type>;
Type substitute(Type, const Substitution &);
E cloneExpr(const E &, const Substitution &);
S cloneStmt(const S &, const Substitution &);
std::shared_ptr<Function> cloneFunction(const std::shared_ptr<Function> &, const Substitution &);
std::shared_ptr<Class> cloneClass(const std::shared_ptr<Class> &, const Substitution &);
```

### 3.1 Type Rewriting Rules
- If a type has `Base::Parameter` and its `name` is present in `Substitution`, it is replaced with the mapped concrete `Type`.
- If a composite type (`Base::Record`) has template arguments, substitution recursively rewrites each type argument.
- Qualifiers (`const`, `reference`, `extent`) are preserved or combined according to standard reference/const collapsing rules.

### 3.2 Deep AST Duplication
When instantiating a template, a complete deep clone of the function or class AST is constructed with substituted types:
- Member types and variable declarations are rewritten with concrete types.
- Expression types, operator nodes, and literal types are cloned.
- Uninstantiated templates never execute directly; only fully resolved, substituted ASTs are dispatched to the runtime.

---

## 4. Function Templates & Template Type Deduction

### 4.1 Explicit Type Arguments
Function templates may be called with explicit template arguments:
```cpp
template<class T>
T add(T a, T b) { return a + b; }

int main() {
    return add<int>(2, 3);
}
```
`Analyzer::select` matches explicit arguments against the function's `templateParameters`.

### 4.2 Template Type Deduction
Praxis implements automatic template argument deduction from call arguments:
```cpp
template<class T>
T twice(T x) { return x + x; }

int main() {
    twice(5);     // Deduces T = int
    twice(2.5);   // Deduces T = double
}
```
During candidate evaluation in `Analyzer::select`:
1. `deduce(Type param, Type actual)` compares the declared parameter type against the caller's argument type.
2. If `param.base == Base::Parameter`, the parameter name is bound to the bare argument type.
3. If the same type parameter appears multiple times, subsequent deductions must match the previously deduced type exactly. Conflicting deductions (e.g. `add(1, 2.0)`) fail deduction unless explicit arguments are provided.
4. Deduction correctly traverses composite and nested record types (e.g., deducing `T` from `Box<T>`).

---

## 5. Class Templates

Class templates parameterized by type names are fully supported:
```cpp
template<class T>
class Box {
    T value;
public:
    Box(T x) : value(x) {}
    T get() const { return value; }
};
```
1. When `Box<int>` or `Box<double>` is encountered in a declaration, `Analyzer::record()` is invoked.
2. Checks whether the specialized class key (`"Box<int>"`) already exists in `Program::compiledClasses`.
3. If not cached, it binds `T -> int`, clones `Box`'s field declarations and member methods, and invokes `Analyzer::defineClass` on the specialization.
4. Each specialization is a distinct class type with distinct member offsets, layout, and member functions.

---

## 6. Instantiation Caching & Lifecycle

### 6.1 Specialization Key & Cache
To prevent redundant work and ensure type uniqueness:
- Function specializations are keyed by `template:Name<Arg1,Arg2,...>Param1;Param2;`.
- Class specializations are keyed by `typeName(Type)` (e.g. `"Box<int>"`).
- All specializations are cached in `Program::compiledFunctions` and `Program::compiledClasses`.
- Cache hits are recorded in `Program::cacheHits` telemetry.

### 6.2 Lazy Semantic Analysis of Instantiated Bodies
In accordance with C++ semantic rules, instantiated function and method bodies are analyzed **on demand**:
- If an invalid operation exists within an uninstantiated template, or within an instantiated class method that is never called, no diagnostic is emitted if valid syntax was parsed.
- When an instantiated function is selected for a call, `Analyzer::checkFunction()` runs semantic analysis on the substituted body.
- If an invalid operation occurs under the substituted types (e.g. using `+` on an un-addable struct), semantic analysis fails and reports the instantiation context:
  ```text
  examples/m2/invalid_template.cpp:3:75: semantic: instantiating template:add<S,>T;T;: operator requires arithmetic operands
  ```

---

## 7. Configurable Limits and Diagnostics

To prevent infinite recursion during template instantiation (e.g. recursive template instantiation cycles):
- `--max-template-depth=N` (default: 32, max: 64)
- `--max-instantiations=N` (default: 256, max: 4096)
Exceeding either threshold halts analysis with a clean diagnostic:
```text
semantic: template instantiation limit
semantic: template instantiation depth limit
```

---

## 8. Documented Limitations

The following advanced C++ template features are explicitly outside the M2 subset:
1. **No Non-Type Template Parameters:** Only `typename` and `class` type parameters are supported (`template<int N>` is unsupported).
2. **No Partial Specialization:** Classes are either template definitions or concrete instantiations.
3. **No Variadic Templates:** Parameter packs (`typename... Args`) are unsupported.
4. **No Concepts / Constraints:** `requires` clauses and concept definitions are unsupported.
5. **No SFINAE:** Substitution failures in candidate function bodies produce diagnostics rather than silent candidate elimination.
6. **Lookup Model:** Instantiated bodies undergo single-phase semantic analysis upon instantiation against currently visible declarations.
