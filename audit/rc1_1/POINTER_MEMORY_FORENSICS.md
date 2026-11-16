# Milestone RC1.1 Pointer and Allocation Memory Forensics

## 1. Abstract Handle Representation and Memory Safety Model

Praxis deliberately rejects exposing raw host virtual addresses to interpreted programs. All pointers in Praxis are represented through a strongly validated abstract handle:

```cpp
struct Pointer {
    std::uint64_t targetId = 0;       // Unique logical identity of target storage
    std::uint64_t generation = 0;     // Monotonic generation counter of target object
    std::shared_ptr<Object> target;   // Reference to runtime object structure
    int offset = 0;                   // Relative element offset (for arrays/containers)
    bool isNull = true;               // Null pointer discriminant
};
```

### Safety Invariants Enforced on Dereference (`*ptr` and `ptr->member`):
1. **Null Check:** `ptr.isNull || ptr.targetId == 0` throws `null pointer dereference`.
2. **Use-After-Free Check:** `!target || target->deleted` throws `use after free`.
3. **Expired Lifetime Check:** `!target->alive` throws `dereference of expired object`.
4. **Stale Generation Check:** `target->generation != ptr.generation` throws `dereference through stale pointer handle`.
5. **Bounds Check:** For containers and arrays, `ptr.offset < 0 || ptr.offset >= target->elements.size()` throws `pointer offset out of bounds`. For scalars, `ptr.offset != 0` throws `pointer offset out of bounds`.

## 2. Forensic Test Results

| Test Scenario | Description | Target Code Pattern | Observed Behavior | Status |
|---|---|---|---|---|
| **P-01** | Null Pointer Dereference | `int* p = nullptr; return *p;` | Runtime error: `null pointer dereference` | **VERIFIED SAFE** |
| **P-02** | Deleting Null Pointer | `int* p = nullptr; delete p;` | Safe no-op, 0 deallocations counted | **VERIFIED SAFE** |
| **P-03** | Use After Free | `int* p = new int(10); delete p; return *p;` | Runtime error: `use after free` | **VERIFIED SAFE** |
| **P-04** | Double Free | `int* p = new int(10); delete p; delete p;` | Runtime error: `double free` | **VERIFIED SAFE** |
| **P-05** | Deleting Non-Heap Object | `int x = 10; int* p = &x; delete p;` | Runtime error: `deleting non-heap object` | **VERIFIED SAFE** |
| **P-06** | Dangling Stack Pointer | Returning address of local variable or accessing pointer after block exit | Runtime error: `dereference of expired object` | **VERIFIED SAFE** |
| **P-07** | Aliased Pointer Deletion | `int* a = new int(1); int* b = a; delete a; return *b;` | Runtime error: `use after free` | **VERIFIED SAFE** |
| **P-08** | Multi-level Pointers (`T**`) | `int x = 5; int* p = &x; int** pp = &p; **pp = 20; return x;` | Evaluates to 20; all layers bounds-checked | **VERIFIED SAFE** |
| **P-09** | One-Past-The-End Iterator | `auto it = v.end(); return *it;` | Runtime error: `pointer offset out of bounds` | **VERIFIED SAFE** |
| **P-10** | Out-of-bounds Arithmetic | `int arr[3] = {1, 2, 3}; int* p = &arr[0] + 5; return *p;` | Runtime error: `pointer offset out of bounds` | **VERIFIED SAFE** |
| **P-11** | Popped Vector Element | `int* p = &v.back(); v.pop_back(); return *p;` | Runtime error: `dereference of expired object` | **VERIFIED SAFE** |
| **P-12** | Cleared Vector Iterator | `auto it = v.begin(); v.clear(); return *it;` | Runtime error: `dereference through stale pointer handle` or `pointer offset out of bounds` | **VERIFIED SAFE** |
| **P-13** | Generation Monotonicity | When storage is destroyed, `target->generation++` prevents any stale pointer from matching subsequent reallocations. | Verified in `objects.cpp:26` and `interpreter.cpp:314, 357`. | **VERIFIED SAFE** |

## 3. Dynamic Storage Resource Management

1. **Quota Enforcement:**
   Praxis enforces a configurable maximum live object quota (`limits.objects`, default `100,000`, configurable via `--max-objects=N`). Exceeding the quota halts execution cleanly with `live object limit exceeded`.
2. **Leak Detection:**
   At interpreter termination (`Interpreter::finish()`), all scopes and temporaries are unwound. `liveObjectCount()` tracks active nodes. In all passing regression and acceptance test suites, `liveObjectCount()` strictly reaches `0`.
