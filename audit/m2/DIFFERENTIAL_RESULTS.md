# Differential Verification Results (Praxis vs MSVC)

**Milestone:** Project PRAXIS — Milestone M2  
**Target Specifications:** Sections 11, 13, 16  
**Reference Compiler:** Microsoft Visual C++ 2022 (cl.exe v19.42.34433 x64)  
**Compilation Flags:** `/nologo /std:c++20 /EHsc /Od /Zc:nrvo-`  
**Praxis Configurations:** `Debug` and `Release` (Windows 11 x64)  

---

## 1. Differential Verification Protocol

Differential testing against an independent, production-grade reference compiler provides objective proof of semantic correctness without using the compiler as an interpreter backend.

### 1.1 Methodology
1. **Source Identical Execution:** The exact same `.cpp` fixture source code is compiled by MSVC and evaluated by Praxis.
2. **Oracle Execution:** MSVC compiles the source into a native x64 executable (`/Od /Zc:nrvo-` disables named return value optimization to ensure strict destructor/copy visibility). The native binary runs, and its standard output and process exit code are recorded.
3. **Praxis Execution:** Praxis executes the program via `praxis run`. The standard output, exit code, and standard error are recorded.
4. **Deterministic Re-Run:** Praxis executes the source a second time. The output and exit code of the second run must identically match the first run.
5. **Exact Match Assertion:** The harness requires:
   $$\text{stdout}_{\text{MSVC}} == \text{stdout}_{\text{Praxis}} \quad \land \quad \text{exit}_{\text{MSVC}} == \text{exit}_{\text{Praxis}} \quad \land \quad \text{stderr}_{\text{Praxis}} == \emptyset$$

---

## 2. Quantitative Summary

| Test Suite | Configuration | Total Programs | Passed | Failed | Pass Rate |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **M2 Differential Suite** | Debug | 78 | 78 | 0 | **100%** |
| **M2 Differential Suite** | Release | 78 | 78 | 0 | **100%** |
| **M1 Regression Differential Suite** | Debug | 135 | 135 | 0 | **100%** |
| **M1 Regression Differential Suite** | Release | 135 | 135 | 0 | **100%** |
| **Total Differential Verification** | **Combined** | **213** | **213** | **0** | **100%** |

*Full machine-readable reports are available in:*
- `audit/m2/differential-Debug.json`
- `audit/m2/differential-Release.json`
- `audit/m2/regression-differential-Debug.json`
- `audit/m2/regression-differential-Release.json`

---

## 3. Verified Language Capabilities Across Differential Programs

The 78 M2 differential test programs systematically exercise all core language features introduced in Milestone M2:

| Capability Group | Representative Fixtures | MSVC Agreement |
| :--- | :--- | :---: |
| **Class & Struct Basics** | `basic_class`, `struct`, `out_of_class_member`, `implicit_member_call`, `member_shadowing` | **100%** |
| **Access Control** | `access_control` | **100%** |
| **Constructors & Initializers** | `constructor_destructor`, `member_initializer_order`, `constructor_overload`, `default_member_initializer`, `scalar_value_init_member`, `constructor_body_mutation`, `value_initialized_member`, `value_initialized_temporary` | **100%** |
| **Copy Semantics & Assignment** | `copy_constructor`, `copy_assignment`, `copy_nested`, `assign_nested`, `self_assignment`, `class_by_value`, `return_local_object`, `return_prvalue_object`, `direct_prvalue_elision`, `member_prvalue_elision`, `parenthesized_prvalue` | **100%** |
| **References & Identity** | `object_reference`, `const_object_reference`, `reference_overload` | **100%** |
| **Const Correctness** | `const_member`, `const_overload`, `template_const_deduction` | **100%** |
| **Overload Resolution** | `free_overload`, `member_overload`, `reference_overload`, `template_nontemplate_tie` | **100%** |
| **Function & Class Templates** | `function_template`, `class_template`, `template_deduction`, `nested_template`, `template_reference`, `template_two_parameters`, `template_class_copy`, `template_class_dtor`, `template_repeated`, `unused_template_member`, `nested_type_deduction`, `unselected_template_body` | **100%** |
| **Template Type Deduction** | `deduce_int`, `deduce_long_long`, `deduce_float`, `deduce_double`, `deduce_std_string` | **100%** |
| **RAII & Scope Cleanup** | `raii_return`, `raii_loop_control`, `nested_destruction`, `global_destruction`, `recursive_raii`, `destructor_local_object`, `while_cleanup`, `do_cleanup` | **100%** |
| **Object Lifecycle Stress** | `objects_1`, `objects_2`, `objects_7`, `objects_31`, `objects_100`, `objects_1000` | **100%** |
| **Deep Object Nesting** | `nesting_1`, `nesting_2`, `nesting_3`, `nesting_4`, `nesting_5`, `nesting_6` | **100%** |
| **Temporaries & Expressions** | `temporary_statement`, `temporary_method`, `temporary_condition`, `prvalue_parameter` | **100%** |
| **Static & Global Objects** | `global_zero_class`, `nested_value_zero` | **100%** |

---

## 4. Key Differential Test Comparisons

### 4.1 Nested Destruction (`nested_destruction.cpp`)
```cpp
#include <iostream>
#include <string>
struct Inner { Inner(){std::cout<<"b";} ~Inner(){std::cout<<"B";} };
struct Outer { Inner in; Outer(){std::cout<<"a";} ~Outer(){std::cout<<"A";} };
int main() { Outer obj; }
```
- **MSVC Output:** `abBA`
- **Praxis Output:** `abBA`
- **Verification:** Construction executes inner first (`b`), then outer (`a`). Destruction runs in reverse order: outer destructor (`A`), then inner member destructor (`B`).

### 4.2 RAII with Loop Control Flow (`raii_loop_control.cpp`)
```cpp
#include <iostream>
#include <string>
struct G { int i; G(int v):i(v){std::cout<<"C"<<i;} ~G(){std::cout<<"D"<<i;} };
int main() {
    for (int i=0; i<3; ++i) {
        G g(i);
        if (i==1) break;
    }
}
```
- **MSVC Output:** `C0D0C1D1`
- **Praxis Output:** `C0D0C1D1`
- **Verification:** When `break` executes at `i==1`, the destructor `~G()` runs immediately on `g(1)` before exiting the loop.

### 4.3 Template Type Deduction & Instantiation (`template_deduction.cpp`)
```cpp
#include <iostream>
#include <string>
template<class T> T mult(T a, T b){return a*b;}
int main(){ std::cout<<mult(2,3)<<" "<<mult(1.5,2.5); }
```
- **MSVC Output:** `6 3.75`
- **Praxis Output:** `6 3.75`
- **Verification:** Template argument deduced as `int` for call 1 and `double` for call 2. Separate instantiations produced and executed without loss of precision.

### 4.4 High Object Throughput Stress (`objects_1000.cpp`)
```cpp
#include <iostream>
#include <string>
int created=0; int destroyed=0;
struct G { G(){++created;} ~G(){++destroyed;} };
int main(){
    for(int i=0; i<1000; ++i){ G g; }
    std::cout<<created<<":"<<destroyed;
}
```
- **MSVC Output:** `1000:1000`
- **Praxis Output:** `1000:1000`
- **Verification:** Exactly 1,000 objects constructed and 1,000 objects destroyed deterministically with no memory accumulation.
