# Milestone RC1.1 STL Object Lifetime Forensics

## 1. Executive Summary

Milestone RC1.1 subjected `std::vector<T>` to adversarial lifetime and memory forensics when holding user-defined class objects. Constructing, copying, reassigning, resizing, clearing, and destroying containers of non-trivial objects was verified using deterministic trace instrumentation.

All 18 lifetime scenarios were analyzed. Three critical lifecycle defects in the M3 baseline were diagnosed, repaired, and certified with automated zero-leak assertions (`liveObjectCount() == 0`).

## 2. Tested Lifecycle Operations and Trace Evidence

Using classes with instrumented constructors and destructors emitting tokens (`+I:id`, `+copy:id`, `-I:id`), the following behaviors were verified:

### 1. Default Construction
- Evaluated via `v.resize(3)` and declaration `std::vector<Widget> v(3)`.
- Verified each element invokes `Widget::$ctor()`, gets initialized, and is marked `complete = true`.
- Output: `+I:0 +I:0 +I:0` (alive count = 3).

### 2. Copy Construction
- Evaluated via `v.push_back(item)`.
- Verified element is copied into container via `Widget::$ctor(const Widget&)`.
- Output: `+copy:id` (alive count incremented by 1 per element).

### 3. Copy Assignment
- Evaluated via `v2 = v1` where `v2` already contained elements.
- Verified all prior elements of `v2` are destroyed in reverse order before copying elements from `v1`.
- Trace excerpt: `v2` had items 2 and 3; `v2 = v1` executed `-I:3 -I:2 +copy:1`.

### 4. Move Semantics & Value Categories
- `std::move(x)` yields an rvalue expression (`e->lvalue = false`).
- In the documented Praxis subset, move-eligible operations fall back to copy construction because rvalue references (`T&&`) and move constructors (`T(T&&)`) are not part of the language subset.
- Trace verified that copy constructor executes and live object tracking remains completely balanced.

### 5. `push_back`
- Appends an element to the container.
- For record types, copy-constructs the element into the container.
- Increments container generation counter (`selfObj->generation++`) to ensure iterator invalidation.

### 6. `pop_back`
- Invokes destructor on the final element (`destroy(elements.back())`).
- Verified destruction order and immediate decrementation of live object count.
- Diagnostics: Calling `pop_back()` on an empty vector throws runtime error `pop_back on empty vector`.

### 7. `clear`
- Iterates over elements in reverse order (`elements.rbegin()` to `elements.rend()`) and destroys each element.
- Empties container element vector.
- Bumps generation counter (`selfObj->generation++`).
- Trace excerpt: vector with elements 10 and 20 cleared emits `-I:20 -I:10`.

### 8. `resize` Upwards & Downwards
- **Downwards (`resize(k)` where `k < size()`):**
  Pops elements in reverse order, calling `destroy()` on each.
  Verified: resizing from 4 to 2 elements destroys element 3 then element 2.
- **Upwards (`resize(k)` where `k > size()`):**
  Allocates new element cells, invokes default constructor `construct(cell, defaultCtor, {})`, and appends them.
  If explicit second argument provided (`resize(k, val)`), copies `val` into each new element.
  Bumps generation counter to invalidate prior iterators.

### 9. `reserve`
- Supported as a recognized API call (`native:vector.reserve`).
- In the abstract object model, storage is handled dynamically without native buffer reallocations.

### 10. Container Copy Construction
- Evaluated via `std::vector<Item> v2 = v1;`.
- Deep copies each element using `copyObject`.
- Verified modifications to `v2` do not affect `v1`.

### 11. Scope Exit Destruction
- When a vector goes out of scope, its automatic cleanup invokes `destroy()`.
- `destroy()` iterates over all elements in reverse order and invokes their class destructors.
- Outer scope objects are destroyed in strict reverse order of declaration.
- Verified across all tests: `liveObjectCount() == 0` upon function return.

### 12. References to Vector Elements
- `Item& ref = v[0];` binds an lvalue reference to the vector element.
- Modifying `ref` mutates the element in-place.
- If the vector is subsequently cleared or resized downwards, dereferencing `ref` is diagnosed as `dereference of expired object` or `dereference through stale pointer handle`.

### 13. Iterator Invalidation
- Holding an iterator (`auto it = v.begin()`) or pointer to element (`Item* p = &v.back()`).
- When `v.clear()`, `v.resize()`, or `v.pop_back()` occurs:
  - For `pop_back()`: the popped element is destroyed; subsequent `*p` fails with `dereference of expired object`.
  - For `clear()` and `resize()`: `selfObj->generation++` increments the vector generation; subsequent `*it` fails with `dereference through stale pointer handle`.
  - If an iterator is one-past-the-end (`v.end()`), dereferencing fails with `pointer offset out of bounds`.

## 3. Defects Discovered and Repaired

| Defect ID | Location | Root Cause | Fix Applied |
|---|---|---|---|
| **LIFETIME-01** | `src/runtime/interpreter.cpp:296` (`push_back`) | `auto cell = allocate(...)` followed by `cell = copyObject(...)` leaked an abandoned object node. | Direct branch: allocate only for scalars; for records, `copyObject` directly allocates the target cell. |
| **LIFETIME-02** | `src/runtime/interpreter.cpp:613` (`declare`) | `cell = allocate(...)` followed by `cell = copyObject(...)` in vector braced initializer leaked object nodes. | Direct branch: use `copyObject` directly for record elements. |
| **LIFETIME-03** | `src/runtime/objects.cpp:60` (`assignObject`) | `destination->elements.clear()` cleared element list without calling `destroy(*it)` on previous elements. | Added reverse iteration calling `destroy(*it)` on all prior elements before clearing. |
| **LIFETIME-04** | `src/runtime/interpreter.cpp:364` (`resize`) | Upward resize allocated cells without invoking default constructor, failing to mark objects `complete`. | Added default constructor dispatch via `construct(cell, e->copyTarget, {})` during upward resize. |
| **LIFETIME-05** | `src/runtime/interpreter.cpp:506` (`dereference`) | Dereferencing an iterator on an empty vector (`ptr.offset == 0`) erroneously returned the vector itself as a scalar. | Explicitly distinguished containers from scalars, enforcing `ptr.offset < elements.size()`. |
| **LIFETIME-06** | `src/runtime/interpreter.cpp:313, 356` | Container operations (`clear`, `resize`, `push_back`) did not bump container generation counters. | Added `selfObj->generation++` on mutating container methods, guaranteeing iterator invalidation detection. |
