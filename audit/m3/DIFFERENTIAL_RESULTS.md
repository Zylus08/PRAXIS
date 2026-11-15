# PRAXIS M3 — Differential Verification Results vs MSVC C++20

## 1. Methodology & Environmental Invariants

To guarantee that Praxis executes interpreted C++ code independently and correctly according to standard language rules, Praxis is verified against native Microsoft Visual C++ (`cl.exe /std:c++20 /EHsc /Od /Zc:nrvo-`).

### 1.1 Non-Negotiable Oracle Constraints
- **Strictly External Oracle:** MSVC is invoked exclusively by external verification test scripts (`tests/differential/run.py`, `tests/m2/differential.py`, `tests/m3/differential.py`).
- **No Embedded Backend:** Praxis contains zero links or invocations to MSVC, Clang, LLVM, or GCC.
- **Repeat Run Determinism:** Every program is executed twice through Praxis (`run` and `repeat`), confirming that exit codes, stdout, and stderr are 100% deterministic and free of state pollution.

---

## 2. Milestone Tally & Regression Parity

| Verification Suite | Candidates | Passing (Debug) | Passing (Release) | Pass Rate |
|---|---|---|---|---|
| **M1 Differential Suite** | 137 | 137 | 137 | 100.0% |
| **M2 Differential Suite** | 79 | 79 | 79 | 100.0% |
| **M3 Differential Suite** | 42 | 42 | 42 | 100.0% |
| **Total Differential Programs** | **258** | **258** | **258** | **100.0%** |

---

## 3. M3 Differential Test Fixtures (42/42 Passed)

| Test Case Name | Category | Praxis Output | MSVC Output | Match |
|---|---|---|---|:---:|
| `pointer_scalar_int` | pointer | `25 25` | `25 25` | PASS |
| `pointer_scalar_double` | pointer | `7.25` | `7.25` | PASS |
| `pointer_nullptr` | pointer | `14` | `14` | PASS |
| `pointer_arrow_struct` | pointer | `12,34` | `12,34` | PASS |
| `pointer_to_class` | pointer | `6` | `6` | PASS |
| `pointer_param` | pointer | `30` | `30` | PASS |
| `pointer_multi_level` | pointer | `99` | `99` | PASS |
| `pointer_const_pointee` | pointer | `42` | `42` | PASS |
| `pointer_const_pointer` | pointer | `50` | `50` | PASS |
| `dynamic_scalar` | dynamic | `42` | `42` | PASS |
| `dynamic_class` | dynamic | `C7M7D7` | `C7M7D7` | PASS |
| `delete_null` | dynamic | `OK` | `OK` | PASS |
| `dynamic_loop` | dynamic | `150` | `150` | PASS |
| `raii_raw_wrapper` | raii | `C100D` | `C100D` | PASS |
| `raii_linked_list` | raii | `C2C1D2D1` | `C2C1D2D1` | PASS |
| `raii_early_return` | raii | `C1C2D2D142` | `C1C2D2D142` | PASS |
| `vector_default` | stl_vector | `0 empty` | `0 empty` | PASS |
| `vector_init_list` | stl_vector | `3 10 20 30` | `3 10 20 30` | PASS |
| `vector_push_pop` | stl_vector | `322` | `322` | PASS |
| `vector_front_back` | stl_vector | `100 300` | `100 300` | PASS |
| `vector_indexing` | stl_vector | `5 50 25` | `5 50 25` | PASS |
| `vector_clear` | stl_vector | `0 empty` | `0 empty` | PASS |
| `vector_copy_ctor` | stl_vector | `2 3` | `2 3` | PASS |
| `vector_copy_assign` | stl_vector | `2 3` | `2 3` | PASS |
| `vector_iterator_loop` | stl_vector | `123` | `123` | PASS |
| `utility_pair` | stl_utility | `10 20` | `10 20` | PASS |
| `utility_make_pair` | stl_utility | `30 40` | `30 40` | PASS |
| `utility_swap` | stl_utility | `2 1` | `2 1` | PASS |
| `utility_move` | stl_utility | `42` | `42` | PASS |
| `algo_min_max` | stl_algorithm | `10 20` | `10 20` | PASS |
| `algo_sort` | stl_algorithm | `10 20 30 ` | `10 20 30 ` | PASS |
| `algo_reverse` | stl_algorithm | `3 2 1 ` | `3 2 1 ` | PASS |
| `algo_find` | stl_algorithm | `20` | `20` | PASS |
| `algo_count` | stl_algorithm | `3` | `3` | PASS |
| `range_for_vector_int` | range_for | `60` | `60` | PASS |
| `range_for_vector_auto` | range_for | `24` | `24` | PASS |
| `range_for_string` | range_for | `6` | `6` | PASS |
| `accumulator` | acceptance | `60` | `60` | PASS |
| `string_methods` | stl_string | `14 14 Praxis H` | `14 14 Praxis H` | PASS |
| `pointer_arithmetic` | pointer | `200 300 100` | `200 300 100` | PASS |
| `pointer_subtraction` | pointer | `3` | `3` | PASS |
| `pointer_relational` | pointer | `1 0 1 0` | `1 0 1 0` | PASS |

---

## 4. Conclusion

Praxis v0.1.0-rc1 achieves complete differential parity with MSVC across all 258 differential programs without using MSVC as an interpreter runtime backend.
