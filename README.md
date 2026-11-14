# Project PRAXIS — Milestone M2
## Classes, Object Lifetimes, RAII, Templates & C++ Semantic Foundations

Praxis is an independent, self-contained C++20 implementation of a **restricted C++ interpreter**. It lexes, preprocesses, parses, type-checks, and tree-walks user source itself. It does not compile user code or depend on a compiler backend at runtime. This is not a fully conforming C++ implementation.

Milestone M2 elevates Praxis from a scalar interpreter to a class-aware, lifetime-correct runtime supporting object-oriented and generic C++ programming.

---

## Key Milestone M2 Capabilities

- **Classes and Structs:** `class` (default private access) and `struct` (default public access), data members, access control labels (`public:`, `private:`), member functions, `const` member functions, and implicit `this` member resolution.
- **Constructors and Initializers:** Default constructors, parameterized constructors, copy constructors, member initializer lists executing strictly in **declaration order**, default member initializers, and scalar value initialization (`$zero`).
- **Destructors and Deterministic RAII:** Reverse construction-order destruction, automatic cleanup on normal block exit, `return`, `break`, and `continue`, nested object cleanup, temporary expression lifecycle management, and constructor failure safety.
- **Copy Semantics and Object Identity:** Explicit distinction between copy construction and copy assignment, synthesized memberwise copying, self-assignment guards, const field immutability, lvalue reference identity preservation (`T&`), and prvalue copy elision (`$elide`).
- **Function and Constructor Overloading:** Principled overload resolution supporting free functions, member functions, constructors, and templates with standardized conversion ranks (0–3), const-correctness ranking, and ambiguity detection.
- **Native Template Engine:** `template<typename T>` and `template<class T>`, function templates with explicit arguments and type deduction, class templates (`Box<T>`), deep AST cloning and type substitution, specialization caching, on-demand semantic analysis of instantiated bodies, and recursion limits.
- **Telemetry and Resource Bounds:** High-resolution JSON profiling via `--profile`, with configurable limits (`--max-calls`, `--max-template-depth`, `--max-instantiations`, `--max-objects`, `--max-steps`).

---

## Build and Use (Windows x64)

Prerequisites: Visual Studio 2022 Build Tools (MSVC v143 with C++ tools), CMake, and Ninja. Python 3 is required for verification testing.

```powershell
# Build Debug and Release configurations
./tools/build.ps1 Debug
./tools/build.ps1 Release

# Command-Line Usage
./build/Release/praxis.exe --version
./build/Release/praxis.exe --help
./build/Release/praxis.exe run examples/m2/basic_class.cpp
./build/Release/praxis.exe run examples/m2/class_template.cpp --profile
./build/Release/praxis.exe check examples/m2/basic_class.cpp
./build/Release/praxis.exe repl
```

---

## Example M2 Programs

### 1. Class with RAII Destructor & Member Initializer Order
```cpp
#include <iostream>
#include <string>

struct Resource {
    int id;
    Resource(int n) : id(n) { std::cout << "C" << id; }
    ~Resource() { std::cout << "D" << id; }
};

class Container {
    Resource first;
    Resource second;
public:
    // Initializer list text order differs, but declaration order governs execution
    Container() : second(2), first(1) {
        std::cout << "!";
    }
};

int main() {
    Container c;
}
// Outputs: C1C2!D2D1
```

### 2. Class Template with Type Deduction
```cpp
#include <iostream>
#include <string>

template<class T>
class Box {
    T value;
public:
    Box(T v) : value(v) {}
    T get() const { return value; }
};

template<class T>
T multiply(T a, T b) {
    return a * b;
}

int main() {
    Box<int> b(10);
    std::cout << b.get() << " " << multiply(2.5, 4.0) << std::endl;
}
// Outputs: 10 10
```

---

## Verification and Testing

Milestone M2 verification consists of 642 verification checks per configuration (1,284 total across Debug and Release):
- **CTest Suite:** Frontend unit tests (`praxis_unit`), M2 unit tests (`praxis_m2_unit`), M2 integration tests, and M1 integration regression tests.
- **Adversarial / Negative Tests:** 48 dedicated M2 negative and safety tests verifying parse rejection of unsupported constructs, semantic access control, overload ambiguity, const mutation, and constructor unwinding.
- **Differential Verification:** 213 differential programs (78 M2 programs + 135 M1 regression programs) compiled with MSVC (`cl.exe /std:c++20 /EHsc /Od /Zc:nrvo-`) and compared byte-for-byte against Praxis execution.

```powershell
# Run full M2 verification suite (Unit, Integration, Adversarial, Differential)
./tools/verify-m2.ps1 Debug
./tools/verify-m2.ps1 Release
```

For full documentation and verification evidence, see:
- [Milestone M2 Final Report](audit/m2/M2_FINAL_REPORT.md)
- [Baseline Reconnaissance](audit/m2/BASELINE_RECONNAISSANCE.md)
- [Object Model & Copy Semantics](audit/m2/OBJECT_MODEL.md)
- [Constructor & Destructor Semantics](audit/m2/CONSTRUCTOR_DESTRUCTOR_SEMANTICS.md)
- [Template Instantiation Engine](audit/m2/TEMPLATE_INSTANTIATION.md)
- [Overload Resolution](audit/m2/OVERLOAD_RESOLUTION.md)
- [Differential Results vs MSVC](audit/m2/DIFFERENTIAL_RESULTS.md)
- [Adversarial & Negative Results](audit/m2/ADVERSARIAL_RESULTS.md)
- [Performance Profile](audit/m2/PERFORMANCE_PROFILE.md)
- [Known Limitations](audit/m2/KNOWN_LIMITATIONS.md)
- [Architecture Guide](docs/ARCHITECTURE.md)
- [Execution Model](docs/EXECUTION_MODEL.md)
- [Language Support](docs/LANGUAGE_SUPPORT.md)
- [Semantic Rules](docs/SEMANTICS.md)
- [Limitations & Non-Goals](docs/LIMITATIONS.md)
