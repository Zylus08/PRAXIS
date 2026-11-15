# PRAXIS M3 — Standard Library (STL) Subset Compatibility

## 1. Overview

Milestone M3 equips Project PRAXIS with a functional standard library subset covering `<string>`, `<vector>`, `<utility>`, `<algorithm>`, and `<array>`.

These types are exposed as first-class C++ constructs under the `std::` namespace and are implemented with full lifecycle and object model integration.

---

## 2. Header and Module Breakdown

### 2.1 `<vector>` — `std::vector<T>`
- **Constructors:**
  - `std::vector<T>()` — Default constructor initializing empty container.
  - `std::vector<T>(n)` / `std::vector<T>(n, val)` — Sized and value-initialized constructor.
  - `std::vector<T> v = {a, b, c}` — Initializer list constructor.
- **Copy Semantics:**
  - Copy constructor: Performs deep copy of all elements; modifying the copy does not alter the source.
  - Copy assignment (`v1 = v2`): Cleans up existing elements, reallocates, and deep-copies elements.
- **Capacity & Inspection:**
  - `.size()` — Returns number of elements (`std::int64_t`).
  - `.empty()` — Returns boolean state.
  - `.reserve(n)` — Reserves capacity.
  - `.resize(n)` / `.resize(n, val)` — Adjusts element count, default-constructing or trimming elements.
- **Element Access:**
  - `v[i]` — Unchecked direct element indexing lvalue.
  - `v.at(i)` — Bounds-checked element access raising `runtime: vector::at out of range` on violation.
  - `v.front()` — Accesses first element.
  - `v.back()` — Accesses last element.
- **Modifiers:**
  - `.push_back(val)` — Appends value or deep-copies class instance.
  - `.pop_back()` — Destroys and removes final element.
  - `.clear()` — Destroys all elements and resets size to zero.
- **Iterators:**
  - `.begin()` — Returns `Pointer` handle to element 0 with `offset = 0`.
  - `.end()` — Returns `Pointer` handle to container past-the-end (`offset = size()`).

### 2.2 `<string>` — `std::string`
- `.size()` / `.length()` — Character count.
- `.empty()` — Empty check.
- `.at(i)` — Bounds-checked character access with bounds diagnosis.
- `s[i]` — Direct character indexing.
- `.substr(pos, count)` — Substring extraction.
- Range-based for loop over characters.

### 2.3 `<utility>` — Utilities
- `std::pair<T1, T2>` — Heterogeneous value pair with `.first` and `.second` member fields.
- `std::make_pair(a, b)` — Deduces types and constructs pair.
- `std::swap(a, b)` — Swaps values of scalars or objects.
- `std::move(x)` — Expressive move cast for object transfer.

### 2.4 `<algorithm>` — Core Algorithms
- `std::min(a, b)` / `std::max(a, b)` — Minimum and maximum selection.
- `std::sort(first, last)` — Sorts elements in place between random-access iterator handles.
- `std::reverse(first, last)` — Reverses elements in place.
- `std::find(first, last, val)` — Finds element matching `val`, returning iterator to element or `last`.
- `std::count(first, last, val)` — Returns occurrence count of `val`.

---

## 3. Representative Acceptance Program

The Milestone M3 specification mandated demonstration of the following accumulator program:

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> numbers = {10, 20, 30};
    int total = 0;
    for (int n : numbers) {
        total += n;
    }
    std::cout << total;
}
```

### Verification Result
- Output produced: `60`
- Exit code: `0`
- Differential status against MSVC: Exact match (`tests/m3/fixtures/accumulator.cpp`).

---

## 4. Limitations Relative to ISO C++20

1. **Allocators:** Praxis does not support custom memory allocator parameters (`std::allocator<T>` is implicit).
2. **Exception Specifications:** Vector operations do not throw `std::out_of_range` exception objects; bounds violations raise interpreted runtime errors.
3. **Iterator Tags:** Iterators are lowered to Praxis validated pointer handles rather than custom struct iterator classes.
4. **Const Iterators:** `cbegin()` and `cend()` are not separately distinguished from `begin()`/`end()`.
