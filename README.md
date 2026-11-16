# Project PRAXIS — Release v0.1.0
## Classes, Object Lifetimes, RAII, Templates, Pointers, Dynamic Storage & STL Subset
**Version: v0.1.0 (Certified Final Release)**

Praxis is an independent, self-contained C++20 implementation of a **restricted C++ interpreter**. It lexes, preprocesses, parses, type-checks, and tree-walks user source itself. It does not compile user code or depend on an external compiler backend at runtime. This is not a general ISO C++ implementation, but a verified, lifetime-safe dialect.

Milestone RC1.1 certifies Praxis v0.1.0 for release with lifetime forensics, memory safety guarantees, and differential parity against native MSVC.

---

## Key Capabilities in Praxis v0.1.0

- **Typed Pointer Model:** First-class typed pointer values (`T*`, `const T*`, `T* const`, `T**`), address-of (`&`), dereference (`*`), arrow member access (`->`), and `nullptr` (`std::nullptr_t`). Pointers use an **Abstract Validated Handle Reference Model** (`targetId`, `generation`, `target`, `offset`) that strictly preserves object identity without leaking host virtual memory addresses.
- **Pointer Arithmetic & Comparisons:** `ptr + n`, `ptr - n`, pointer subtraction `p2 - p1` yielding `ptrdiff_t`, increment/decrement (`++`, `--`), and relational ordering comparisons (`<`, `<=`, `>`, `>=`) within identical container slices.
- **Dynamic Object Storage:** `new T` and `new T(arguments...)` with constructor overload resolution, `delete p`, safe `delete nullptr` no-ops, double-free runtime detection, protection against deleting automatic stack objects, and allocation quotas (`--max-objects`).
- **Deterministic RAII & Ownership:** Recursive cleanup chains across automatic variables, member pointers, and dynamic recursive data structures, guaranteeing zero leaked objects on completion.
- **Standard Library (STL) Subset:**
  - `<vector>`: `std::vector<T>` supporting default, sized, and initializer-list constructors, deep copy construction and assignment, `push_back`, `pop_back`, `operator[]`, `at` (bounds-checked), `front`, `back`, `size`, `empty`, `clear`, `resize`, `reserve`, `begin`, and `end`.
  - `<string>`: `size()`, `length()`, `empty()`, `at()`, `substr()`, and character indexing.
  - `<utility>`: `std::pair<T1, T2>`, `std::make_pair`, `std::swap`, and `std::move`.
  - `<algorithm>`: `std::min`, `std::max`, `std::sort`, `std::reverse`, `std::find`, `std::count`.
  - `<array>`: Fixed-size container subset.
- **Iterators & Range-Based For Loops:** Random-access iterator pointer handles and lowering of `for (decl : range) stmt` over vector, array, and string with `auto`, `int`, and reference deductions.
- **Class and Function Templates:** Generic function and class templates, automatic template deduction from call arguments and container references, template classes with standard container members, and instantiation memoization.
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
./build/Release/praxis.exe run examples/rc1_1/accumulator_template.cpp
./build/Release/praxis.exe run examples/m3/dynamic_scalar.cpp --profile
./build/Release/praxis.exe check examples/rc1_1/accumulator_template.cpp
./build/Release/praxis.exe repl
```

---

## Example Programs

### 1. Mandatory Acceptance Program (`examples/rc1_1/accumulator_template.cpp`)
```cpp
#include <iostream>
#include <vector>

template<typename T>
class Accumulator {
    T value;

public:
    Accumulator(T initial) : value(initial) {}

    void add(T x) {
        value += x;
    }

    T result() const {
        return value;
    }
};

int main() {
    std::vector<int> numbers = {10, 20, 30};

    Accumulator<int> sum(0);

    for (int n : numbers) {
        sum.add(n);
    }

    std::cout << sum.result() << "\n";
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

Praxis v0.1.0 verification consists of **798 checks per configuration** (**1,596 total across Debug and Release**):
- **CTest Suite (7 test runners):**
  - Frontend unit tests (`praxis_unit`): 27 assertions
  - M2 unit tests (`praxis_m2_unit`): 38 assertions
  - M3 unit tests (`praxis_m3_unit`): 35 assertions
  - RC1.1 unit tests (`praxis_rc1_1_unit`): 17 assertions
  - M3 integration tests: 55 cases
  - M2 integration tests: 149 cases
  - M1 integration tests: 134 cases
- **Adversarial / Safety Tests:** 13 dedicated safety checks verifying memory protection against use-after-free, double deletion, deleting stack objects, dangling pointers to expired frames, null dereferencing, and out-of-bounds container access.
- **Differential Verification:** **258 differential programs** (42 M3 + 79 M2 + 137 M1 programs) compiled with native MSVC (`cl.exe /std:c++20 /EHsc /Od /Zc:nrvo-`) and compared byte-for-byte against Praxis execution with **100.0% pass rate**.

```powershell
# Run full verification suite (Unit, Integration, Adversarial, Differential)
./tools/verify-m3.ps1 -Configuration Debug
./tools/verify-m3.ps1 -Configuration Release
```

For full certification audit artifacts, see:
- [RC1.1 Baseline Verification](audit/rc1_1/BASELINE_VERIFICATION.md)
- [Original Acceptance Program](audit/rc1_1/ORIGINAL_ACCEPTANCE_PROGRAM.md)
- [STL Object Lifetime Forensics](audit/rc1_1/STL_OBJECT_LIFETIME.md)
- [Pointer Memory Forensics](audit/rc1_1/POINTER_MEMORY_FORENSICS.md)
- [Move Semantics Audit](audit/rc1_1/MOVE_SEMANTICS.md)
- [STL Compatibility Support Matrix](audit/rc1_1/STL_API_SUPPORT_MATRIX.md)
- [Parser & Semantic Integrity Audit](audit/rc1_1/PARSER_SEMANTIC_AUDIT.md)
- [Differential Harness Audit](audit/rc1_1/DIFFERENTIAL_HARNESS_AUDIT.md)
- [Performance & Resource Profile](audit/rc1_1/PERFORMANCE_RESOURCE_RESULTS.md)
- [Regression Test Results](audit/rc1_1/REGRESSION_RESULTS.md)
- [Release Claim Ledger](audit/rc1_1/RELEASE_CLAIM_LEDGER.md)
- [Final Release Certification Decision](audit/rc1_1/FINAL_RELEASE_DECISION.md)
- [Architecture Guide](docs/ARCHITECTURE.md)
- [Execution Model](docs/EXECUTION_MODEL.md)
- [Language Support](docs/LANGUAGE_SUPPORT.md)
- [Semantic Rules](docs/SEMANTICS.md)
- [Limitations & Non-Goals](docs/LIMITATIONS.md)
