# Project PRAXIS — Milestone M3
## Pointers, Dynamic Storage, STL Subset, Adversarial Semantics & Release Readiness
**Version: v0.1.0-rc1 (Release Candidate)**

Praxis is an independent, self-contained C++20 implementation of a **restricted C++ interpreter**. It lexes, preprocesses, parses, type-checks, and tree-walks user source itself. It does not compile user code or depend on a compiler backend at runtime. This is not a fully conforming C++ implementation.

Milestone M3 advances Praxis into a production-grade, lifetime-correct interpreter supporting typed pointers, heap allocation, RAII resource ownership, standard library containers and algorithms, and range-based iteration.

---

## Key Milestone M3 Capabilities

- **Typed Pointer Model:** First-class typed pointer values (`T*`, `const T*`, `T* const`, `T**`), address-of (`&`), dereference (`*`), arrow member access (`->`), and `nullptr` (`std::nullptr_t`). Pointers use an **Abstract Validated Handle Reference Model** (`targetId`, `generation`, `target`, `offset`) that strictly preserves object identity without leaking host virtual memory addresses.
- **Pointer Arithmetic & Comparisons:** `ptr + n`, `ptr - n`, pointer subtraction `p2 - p1` yielding `ptrdiff_t`, increment/decrement (`++`, `--`), and relational ordering comparisons (`<`, `<=`, `>`, `>=`) within identical container slices.
- **Dynamic Object Storage:** `new T` and `new T(arguments...)` with constructor overload resolution, `delete p`, safe `delete nullptr` no-ops, double-free runtime detection, protection against deleting automatic stack objects, and allocation quotas (`--max-objects`).
- **Deterministic RAII & Ownership:** Recursive cleanup chains across automatic variables, member pointers, and dynamic recursive data structures (e.g. linked lists and binary trees), guaranteeing zero leaked objects on completion.
- **Standard Library (STL) Subset:**
  - `<vector>`: `std::vector<T>` supporting default, sized, and initializer-list constructors, deep copy construction and assignment, `push_back`, `pop_back`, `operator[]`, `at` (bounds-checked), `front`, `back`, `size`, `empty`, `clear`, `resize`, `reserve`, `begin`, and `end`.
  - `<string>`: `size()`, `length()`, `empty()`, `at()`, `substr()`, and character indexing.
  - `<utility>`: `std::pair<T1, T2>`, `std::make_pair`, `std::swap`, and `std::move`.
  - `<algorithm>`: `std::min`, `std::max`, `std::sort`, `std::reverse`, `std::find`, `std::count`.
  - `<array>`: Fixed-size container subset.
- **Iterators & Range-Based For Loops:** Random-access iterator pointer handles and lowering of `for (decl : range) stmt` over vector, array, and string with `auto`, `int`, and reference deductions.
- **Telemetry & Resource Bounds:** JSON profiling via `--profile`, with tracking for `dynamic_allocations`, `dynamic_deletions`, `container_operations`, `objects`, `peak_live_objects`, `constructions`, `destructors`, `copies`, `assignments`, and timing.

---

## Build and Use (Windows x64)

Prerequisites: Visual Studio 2022 Build Tools (MSVC v143 with C++ tools), CMake, and Ninja. Python 3 is required for verification testing.

```powershell
# Build Debug and Release configurations
./tools/build.ps1 -Configuration Debug
./tools/build.ps1 -Configuration Release

# Command-Line Usage
./build/Release/praxis.exe --version
./build/Release/praxis.exe --help
./build/Release/praxis.exe run examples/m3/accumulator.cpp
./build/Release/praxis.exe run examples/m3/dynamic_scalar.cpp --profile
./build/Release/praxis.exe check examples/m3/accumulator.cpp
./build/Release/praxis.exe repl
```

---

## Example M3 Programs

### 1. Representative Acceptance Program (`examples/m3/accumulator.cpp`)
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
// Outputs: 60
```

### 2. Dynamic Memory & Arrow Member Access (`examples/m3/dynamic_class.cpp`)
```cpp
#include <iostream>

struct Resource {
    int id;
    Resource(int i) : id(i) { std::cout << "C" << id; }
    ~Resource() { std::cout << "D" << id; }
};

int main() {
    Resource* r = new Resource(7);
    std::cout << "M" << r->id;
    delete r;
}
// Outputs: C7M7D7
```

### 3. STL Algorithms & Iterators (`examples/m3/algorithm_demo.cpp`)
```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> v = {30, 10, 20};
    std::sort(v.begin(), v.end());
    for (auto it = v.begin(); it != v.end(); it++) {
        std::cout << *it << " ";
    }
}
// Outputs: 10 20 30 
```

---

## Verification and Testing

Milestone M3 verification consists of **781 checks per configuration** (**1,562 total across Debug and Release**):
- **CTest Suite (6 test runners):**
  - Frontend unit tests (`praxis_unit`): 15 checks
  - M2 unit tests (`praxis_m2_unit`): 31 checks
  - M3 unit tests (`praxis_m3_unit`): 39 checks
  - M3 integration tests: 55 cases
  - M2 integration tests: 126 cases
  - M1 integration tests: 244 cases
- **Adversarial / Safety Tests:** 13 dedicated safety checks verifying memory protection against use-after-free, double deletion, deleting stack objects, dangling pointers to expired frames, null dereferencing, and out-of-bounds container access.
- **Differential Verification:** **258 differential programs** (42 M3 + 79 M2 + 137 M1 programs) compiled with native MSVC (`cl.exe /std:c++20 /EHsc /Od /Zc:nrvo-`) and compared byte-for-byte against Praxis execution with **100.0% pass rate**.

```powershell
# Run full M3 verification suite (Unit, Integration, Adversarial, Differential)
./tools/verify-m3.ps1 -Configuration Debug
./tools/verify-m3.ps1 -Configuration Release
```

For full documentation and verification evidence, see:
- [Milestone M3 Final Report](audit/m3/M3_FINAL_REPORT.md)
- [Baseline Reconnaissance](audit/m3/BASELINE_RECONNAISSANCE.md)
- [Pointer Model Architecture](audit/m3/POINTER_MODEL.md)
- [Dynamic Storage Management](audit/m3/DYNAMIC_STORAGE.md)
- [Lifetime Safety & RAII](audit/m3/LIFETIME_SAFETY.md)
- [STL Subset Compatibility](audit/m3/STL_COMPATIBILITY.md)
- [Iterator Semantics & Range-For](audit/m3/ITERATOR_SEMANTICS.md)
- [Template Integration](audit/m3/TEMPLATE_INTEGRATION.md)
- [Differential Results vs MSVC](audit/m3/DIFFERENTIAL_RESULTS.md)
- [Adversarial & Negative Results](audit/m3/ADVERSARIAL_RESULTS.md)
- [Resource Profile & Telemetry](audit/m3/RESOURCE_PROFILE.md)
- [Catalog of Release Limitations](audit/m3/RELEASE_LIMITATIONS.md)
- [Architecture Guide](docs/ARCHITECTURE.md)
- [Execution Model](docs/EXECUTION_MODEL.md)
- [Language Support](docs/LANGUAGE_SUPPORT.md)
- [Semantic Rules](docs/SEMANTICS.md)
- [Limitations & Non-Goals](docs/LIMITATIONS.md)
