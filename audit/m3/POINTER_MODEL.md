# PRAXIS M3 — Typed Pointer Model Architecture

## 1. Executive Summary

Milestone M3 establishes first-class typed pointer values in Project PRAXIS. Unlike native compilers that expose raw virtual memory addresses, and unlike scripting engines that conflate pointers with generic reference wrappers, Praxis implements an **Abstract Validated Handle Reference Model**.

This architecture strictly enforces interpreted C++ object identity, lifetime tracking, and boundary protection while guaranteeing that host process memory addresses are never leaked to interpreted user programs.

---

## 2. Abstract Pointer Value Representation

Pointers in Praxis are represented by the `praxis::Pointer` structure within `praxis::Value`:

```cpp
struct Pointer {
    std::uint64_t targetId = 0;       // Unique monotonic object identifier
    std::uint64_t generation = 0;     // Object incarnation/epoch counter
    std::shared_ptr<Object> target;   // Reference to target storage descriptor
    int offset = 0;                   // Relative element offset (for arrays/containers)
    bool isNull = true;               // Explicit null indicator
};
```

### 2.1 Key Design Principles
1. **Absence of Host Memory Exposure:**  
   Host pointer addresses are not visible to user code. Outputting or inspecting a pointer prints its abstract identity or dereferenced value, never raw virtual host addresses.
2. **Stable Object Identity:**  
   Every allocated object (automatic or dynamic) receives a globally unique monotonic `uint64_t` identifier (`targetId`). Comparing two pointers for equality (`p1 == p2`) evaluates:
   - Null status equivalence (`isNull`),
   - Object identity (`targetId`),
   - Generation parity (`generation`),
   - Element offset parity (`offset`).
3. **Generation Counter Validation:**  
   Every storage slot maintains a `generation` counter initialized to 1. On destruction or deallocation, `generation` is incremented and `alive` is set to `false`. Any pointer retaining an older generation number immediately fails validation upon dereference (`use after free` or `stale handle`).

---

## 3. Supported Pointer Syntax and Type System

The type system was extended with:
- `Type::pointer` (`int` counting indirection levels: `T*`, `T**`, etc.)
- `Type::constant` (const qualification on the pointer itself: `T* const`)
- `Type::pointeeConstant` (const qualification on the referenced entity: `const T*`)
- `Base::Nullptr` (`std::nullptr_t` and `nullptr` literal)

### 3.1 Operations Supported
| Syntax | Semantics |
|---|---|
| `&lvalue` | Takes abstract address of modifiable or const lvalue, yielding `Pointer` with `offset = 0` |
| `*ptr` | Dereferences pointer. Validates nullity, target liveness, generation parity, and offset bounds |
| `ptr->member` | Syntactic lowering to `(*ptr).member` |
| `nullptr` | Typed null pointer constant convertible to any `T*` and `bool` |
| `ptr == nullptr` / `ptr != nullptr` | Null pointer equality comparisons |
| `if (ptr)` / `if (!ptr)` | Contextual conversion to `bool` (`!isNull && targetId != 0`) |
| `ptr++` / `++ptr` | Advances element offset by 1 |
| `ptr--` / `--ptr` | Decrements element offset by 1 |
| `ptr + n` / `n + ptr` | Computes pointer with offset shifted by integer `n` |
| `ptr - n` | Computes pointer with offset shifted backwards by integer `n` |
| `p2 - p1` | Pointer difference: yields `ptrdiff_t` (`p2.offset - p1.offset`) when targets match |
| `p1 < p2`, `p1 <= p2`, etc. | Relational comparison between offsets into identical target storage |

---

## 4. Safety Invariants & Detection

1. **Null Pointer Dereference:**  
   Evaluating `*p` where `p.isNull == true` or `p.targetId == 0` immediately raises a runtime diagnostic: `null pointer dereference`.
2. **Use-After-Free Detection:**  
   If the heap target of a pointer has been deleted (`target->deleted == true`), dereferencing raises: `use after free`.
3. **Dangling Stack Object Detection:**  
   If an automatic stack frame has exited (`target->alive == false`), dereferencing raises: `dereference of expired object`.
4. **Out-of-Bounds Pointer Offsets:**  
   If offset pointer arithmetic moves outside the bounds of the target container (`offset < 0 || offset >= elements.size()`), dereferencing or indexing raises: `pointer offset out of bounds`.

---

## 5. Verification Coverage

The pointer model is comprehensively verified across:
- `tests/unit/m3.cpp` (Checks 1–13, 28)
- `tests/m3/cases.json` (`pointer_scalar_int`, `pointer_scalar_double`, `pointer_nullptr`, `pointer_arrow_struct`, `pointer_to_class`, `pointer_param`, `pointer_multi_level`, `pointer_const_pointee`, `pointer_const_pointer`, `pointer_arithmetic`, `pointer_subtraction`, `pointer_relational`)
- External MSVC differential testing (100% agreement across Debug and Release configurations).
