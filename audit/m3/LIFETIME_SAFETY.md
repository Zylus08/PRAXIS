# PRAXIS M3 — Lifetime Safety & RAII Resource Management

## 1. Scope & Objective

Milestone M3 expands Project PRAXIS's lifetime management subsystem to govern interactions among automatic variables, heap-allocated dynamic objects, class member pointers, and nested owning resources.

The fundamental goal is to uphold standard C++ RAII (Resource Acquisition Is Initialization) semantics while preventing silent use-after-free, double destruction, and memory corruption.

---

## 2. Object Lifecycle Model

Praxis models storage slots via `std::shared_ptr<praxis::Object>`. While C++ shared pointers are used by the host runtime to prevent host crashes, **interpreted object lifetimes are governed strictly by the source program's lexical and dynamic scopes**.

```
[Object Allocation] 
        │
        ▼
   alive = true, initialized = false, generation = 1
        │
        ▼ (Construction)
   initialized = true, complete = true
        │
        ▼ (Active Execution)
   Dereference checks: alive == true, deleted == false, generation matches
        │
        ▼ (Scope Exit or delete)
   destroy():
     1. Invoke User Destructor (~T)
     2. Destroy members in reverse declaration order
     3. Destroy container elements
     4. generation++
     5. alive = false, initialized = false, complete = false
```

---

## 3. Tombstone & Expiration Validation

When a scope exits, all `owned` automatic objects are destroyed via `Interpreter::cleanup(Scope&)`.
When an object is destroyed:
1. `o->alive` is set to `false`.
2. `o->generation` is incremented.
3. If dynamic, `o->deleted` is set to `true`.

### 3.1 Dangling Stack Pointer Detection
If a function returns the address of a local variable:
```cpp
int* f() {
    int x = 42;
    return &x;
}
```
Upon exiting `f()`, `x` is destroyed and its `alive` flag is set to `false`.
When the caller evaluates `*p`, the interpreter inspects `ptr.target->alive`:
- Because `!ptr.target->alive`, Praxis aborts with:  
  `runtime: dereference of expired object`.

### 3.2 Use-After-Free Detection
If heap storage is deleted:
```cpp
int* p = new int(10);
delete p;
int v = *p;
```
When `delete p` executes, `target->deleted` becomes `true`.
The subsequent read evaluates `*p`, checks `ptr.target->deleted`, and raises:  
`runtime: use after free`.

---

## 4. RAII Chains and Recursive Destruction

Praxis supports owning wrapper classes and recursive dynamic data structures.

### 4.1 Recursive Dynamic Linked List
```cpp
struct Node {
    int val;
    Node* next;
    Node(int v, Node* n) : val(v), next(n) {}
    ~Node() { delete next; }
};

int main() {
    Node* head = new Node(1, new Node(2, new Node(3, nullptr)));
    delete head;
}
```
- Construction Order: `Node(3)` -> `Node(2)` -> `Node(1)`.
- Destruction Order on `delete head`:
  - `~Node(1)` executes -> `delete next` (Node 2)
  - `~Node(2)` executes -> `delete next` (Node 3)
  - `~Node(3)` executes -> `delete next` (nullptr safe no-op)
  - Node 3 destroyed -> Node 2 destroyed -> Node 1 destroyed.
- Verification: Monitored via `Interpreter::liveObjectCount()` which returns `0` upon completion. Zero leaked objects.

---

## 5. Early Returns and Unwinding

When an early `return`, `break`, or `continue` executes from inside nested blocks, `Interpreter::execute` iteratively cleans up all active scopes up to the target boundary:
- Local objects are destroyed in exact reverse order of construction.
- Temporary sub-expressions created during the statement are finalized via `cleanupTemporaries(mark)`.
- Return values are preserved by value or copied before local storage is reclaimed.

---

## 6. Verification Summary

Lifetime safety mechanisms have been verified across both Debug and Release builds:
- Automatic cleanup on normal exit: Verified
- Cleanup on early `return`: Verified (`raii_early_return`)
- Cleanup on `break` and `continue`: Verified
- Detection of dangling stack references: Verified (`err_dangling_stack`)
- Detection of use-after-free: Verified (`err_use_after_free`)
- Prevention of double deletion: Verified (`err_double_free`)
- Clean zero-leak reclamation: Verified (`praxis_m3_unit.exe` Check 27)
