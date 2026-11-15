# PRAXIS M3 — Template Integration with Pointers & Containers

## 1. Overview

Milestone M3 seamlessly marries Milestone M2's template instantiation engine with Milestone M3's typed pointer model and STL container types.

Key challenges addressed:
1. Type substitution across arbitrary pointer indirection levels (`T*`, `T**`, `const T*`).
2. Template argument deduction for STL utilities (`std::make_pair`, `std::min`, `std::max`).
3. Preserving target types during dynamic allocation AST substitutions (`new T`).
4. Supporting recursive incomplete class template pointers (e.g., linked list nodes).

---

## 2. Pointer Type Substitution Engine

When a template is instantiated with a type argument `T = int`, the substitution engine in `src/ast/type.cpp` and `src/ast/substitution.cpp` recursively propagates type metadata:

```cpp
Type substitute(Type t, const std::map<std::string, Type>& m) {
    if (t.base == Base::Parameter) {
        auto it = m.find(t.name);
        if (it != m.end()) {
            Type res = it->second;
            res.pointer += t.pointer;
            if (t.constant) res.constant = true;
            if (t.pointeeConstant) res.pointeeConstant = true;
            if (t.reference) res.reference = true;
            return res;
        }
    }
    // Propagate into template type arguments recursively
    for (auto& arg : t.arguments)
        arg = substitute(arg, m);
    return t;
}
```

### 2.1 Critical Bugfix in AST Cloning (`cloneExpr`)
During M3 development, an issue arose where template specialization of `new T(args...)` wiped the target type to `Base::Void`.
Investigation revealed `cloneExpr` in `src/ast/substitution.cpp` was clearing `c->type = {}`.
**Resolution:** Line 10 of `src/ast/substitution.cpp` was updated to:
```cpp
c->type = substitute(e->type, m);
```
This correctly preserves the target pointee type across template instantiation.

---

## 3. Template Deduction for STL Functions

Template functions in `<algorithm>` and `<utility>` automatically deduce template arguments from call sites:

1. `std::make_pair(a, b)`:
   - Deduces `T1` from argument 1 type, `T2` from argument 2 type.
   - Instantiates `std::pair<T1, T2>`.
2. `std::min(a, b)` / `std::max(a, b)`:
   - Deduces common type `T` from both arguments.
3. `std::sort(first, last)` / `std::reverse(first, last)`:
   - Accepts iterator pointer handles and executes over the identified element types.

---

## 4. Recursive Struct Templates and Incomplete Pointers

In Milestone M2, referencing a class record eagerly looked up its definition. In M3, when `type.pointer > 0`, class resolution avoids eager definition lookups:

```cpp
if (t.pointer > 0)
    return t; // Pointers to incomplete classes are valid forward declarations
```

This permits recursive structures such as:
```cpp
template<class T>
struct Node {
    T val;
    Node* next;
    Node(T v, Node* n) : val(v), next(n) {}
    ~Node() { delete next; }
};
```
Specializing `Node<int>` allows `Node* next` to be stored as a pointer member without infinite recursive type definition.

---

## 5. Verification Evidence

- `tests/unit/m3.cpp`:
  - Check 18: `std::make_pair` template deduction.
  - Check 19: `std::swap`, `std::min`, `std::max`.
  - Check 27: Recursive template-capable dynamic node structure.
- Differential verification: 100% agreement with MSVC C++20 on all template-related fixtures.
