# Milestone RC1.1 Original Acceptance Program Verification

## 1. The Original Acceptance Program

The original program requested for Milestone M3 and mandated for RC1.1 certification integrates class templates, member methods, const qualification, copy initialization, range-based for loops over `std::vector<int>`, and console output:

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
```

## 2. Differential Verification against Native MSVC

### Execution Results
- **Praxis Execution:**
  ```text
  Command: praxis.exe run examples/rc1_1/accumulator_template.cpp
  Exit code: 0
  Stdout: b'60\r\n'
  Stderr: b''
  ```
- **MSVC Native Execution:**
  ```text
  Compiler: cl.exe /nologo /std:c++20 /EHsc /O2 examples/rc1_1/accumulator_template.cpp
  Exit code: 0
  Stdout: b'60\r\n'
  Stderr: b''
  ```
- **Byte-for-Byte Comparison:** Identical (`b'60\r\n' == b'60\r\n'`).

## 3. Required Variant Testing

Each required variant was implemented as an independent test program and verified differentially against native MSVC:

| Variant Test | Source File | Praxis Output | MSVC Output | Differential Match |
|---|---|---|---|---|
| Original Acceptance | `examples/rc1_1/accumulator_template.cpp` | `60\r\n` | `60\r\n` | **MATCH** |
| `Accumulator<double>` | `examples/rc1_1/acc_double.cpp` | `7.25\r\n` | `7.25\r\n` | **MATCH** |
| `Accumulator<long long>` | `examples/rc1_1/acc_long_long.cpp` | `60000000000\r\n` | `60000000000\r\n` | **MATCH** |
| Multiple Instantiations (`int`, `double`, `long long`) | `examples/rc1_1/acc_multiple_instantiations.cpp` | `30 4 300\r\n` | `30 4 300\r\n` | **MATCH** |
| Multiple Instances in Same Scope | `examples/rc1_1/acc_multiple_instances_scope.cpp` | `11 22 33\r\n` | `11 22 33\r\n` | **MATCH** |
| Const Member Calls | `examples/rc1_1/acc_const_member.cpp` | `42\r\n` | `42\r\n` | **MATCH** |
| Class Member with Container (`std::vector<T> items`) | `examples/rc1_1/acc_container_member.cpp` | `60 2\r\n` | `60 2\r\n` | **MATCH** |
| Template Function Accepting Container Ref | `examples/rc1_1/sum_container.cpp` | `10\r\n` | `10\r\n` | **MATCH** |
| Copying Class Template Object | `examples/rc1_1/acc_copy.cpp` | `30 60\r\n` | `30 60\r\n` | **MATCH** |

## 4. Root-Cause Diagnosis and General Architectural Fixes

1. **Root Cause:**
   In `src/semantics/classes.cpp`, checking member initializers in constructor definitions previously assumed all `Base::Record` data members were user-defined classes defined in the AST. When a class template contained a member of standard container type (such as `std::vector<T> items`), constructor checking attempted to look up `record(d.type)->methods` to find constructors and threw `no viable overload`.

2. **General Compiler Repair:**
   - In `src/semantics/classes.cpp`, standard containers (`std::vector`, `std::array`, `std::pair`) are recognized as built-in container types during member initializer analysis. Their initializers are marked with `$container`.
   - In `src/runtime/objects.cpp`, `initializeMembers` recognizes `$container`, allocates the container cell, sets `initialized = true` and `complete = true`, and binds explicit initializers or default container storage.
   - In `src/runtime/objects.cpp`, copy constructors for classes with container members cleanly invoke `copyObject`, ensuring deep element duplication.
   - In `src/runtime/objects.cpp`, `assignObject` calls `assignObject` recursively on container members, clearing prior elements, invoking their destructors, and copying source elements.
   - No pattern-matching or program-specific hacks were introduced; the fixes operate uniformly on all class types containing standard container members.
