# PRAXIS M3 — Adversarial Semantics & Negative Safety Verification

## 1. Safety & Adversarial Testing Philosophy

A production-ready C++ interpreter cannot rely on undefined behavior or host crashes when executing erroneous or malicious source code. Praxis enforces deterministic static rejection for ill-formed code and structured, safe diagnostics for dynamic memory errors.

---

## 2. Static Phase Rejections (Lex, Preprocess, Parse, Semantic)

| Test Case | Stage | Target Misuse | Diagnostic Emitted | Verified Result |
|---|---|---|---|:---:|
| `err_deref_non_pointer` | Semantic | `*5;` (dereferencing integer) | `semantic: dereference requires pointer operand` | PASS |
| `err_arrow_non_pointer` | Semantic | `s->x` where `s` is not a pointer | `semantic: dereference requires pointer operand` | PASS |
| `err_const_pointee_write` | Semantic | `const int* p; *p = 2;` | `semantic: expression is not a modifiable scalar lvalue` | PASS |
| `err_const_pointer_reassign` | Semantic | `int* const p; p = &y;` | `semantic: expression is not a modifiable scalar lvalue` | PASS |
| `err_delete_non_pointer` | Semantic | `delete 10;` | `semantic: delete operand must be a pointer` | PASS |
| `err_address_of_rvalue` | Semantic | `&(1 + 2);` | `semantic: address-of requires lvalue operand` | PASS |
| `unknown_header` | Preprocess | `#include <future>` | `preprocess: cannot open include file: future` | PASS |
| `include_cycle` | Preprocess | Circular include inclusion | `preprocess: include cycle detected` | PASS |

---

## 3. Dynamic Runtime Safety Catches

When memory hazards occur during execution, Praxis detects the invalid operation, terminates execution with exit code 1, and reports the exact source location and error phase:

| Test Case | Hazard Type | Code Pattern | Diagnostic Emitted | Status |
|---|---|---|---|:---:|
| `err_double_free` | Double Free | `int* p = new int; delete p; delete p;` | `runtime: double free detected` | PASS |
| `err_delete_stack` | Delete Non-Heap | `int x; delete &x;` | `runtime: cannot delete non-heap object` | PASS |
| `err_use_after_free` | Use After Free | `int* p = new int; delete p; *p;` | `runtime: use after free` | PASS |
| `err_dangling_stack` | Expired Stack Frame | `int* f() { int x; return &x; } *f();` | `runtime: dereference of expired object` | PASS |
| `err_null_deref` | Null Dereference | `int* p = nullptr; *p;` | `runtime: null pointer dereference` | PASS |
| `err_vector_at_bounds` | Vector Range Error | `v.at(5)` where `v.size() == 2` | `runtime: vector::at out of range` | PASS |
| `err_string_at_bounds` | String Range Error | `s.at(10)` where `s.size() == 3` | `runtime: string::at out of range` | PASS |
| `call_depth` | Call Depth Limit | Infinite recursion `int f() { return f(); }` | `runtime: call depth limit exceeded` | PASS |
| `quota_objects` | Allocation Quota | Exceeding `--max-objects` limit | `runtime: live object limit exceeded` | PASS |

---

## 4. Host Crash Immunity

In each of the adversarial tests above:
- The host process did NOT crash (`0xC0000005`, `0xC00000FD`, segmentation faults are completely absent).
- Intermediate heap allocations are cleanly destroyed.
- Memory leak inspection verifies `liveObjects == 0` upon exit.
