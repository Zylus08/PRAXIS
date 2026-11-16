# Milestone RC1.1 Full Regression Results

## 1. Regression Testing Scope

Every test in the Praxis repository was executed across both Debug (`build/Debug`) and Release (`build/Release`) configurations, encompassing unit tests, adversarial environment checks, integration test suites, and differential verification against MSVC.

## 2. Test Configuration Comparison

| Test Category | Target / Runner | Debug Check Count | Release Check Count | Debug Pass | Release Pass |
|---|---|---|---|---|---|
| **M1 Unit Tests** | `praxis_unit` | 27 assertions | 27 assertions | **100%** | **100%** |
| **M2 Unit Tests** | `praxis_m2_unit` | 38 assertions | 38 assertions | **100%** | **100%** |
| **M3 Unit Tests** | `praxis_m3_unit` | 35 assertions | 35 assertions | **100%** | **100%** |
| **RC1.1 Unit Tests** | `praxis_rc1_1_unit` | 17 assertions | 17 assertions | **100%** | **100%** |
| **M1 Integration** | `tests/integration/run.py` | 134 cases | 134 cases | **100%** | **100%** |
| **M2 Integration** | `tests/m2/run.py` | 149 cases | 149 cases | **100%** | **100%** |
| **M3 Integration** | `tests/m3/run.py` | 55 cases | 55 cases | **100%** | **100%** |
| **Adversarial Env Checks** | `tests/integration/adversarial.py` | 13 checks | 13 checks | **100%** | **100%** |
| **M1 Differential** | `tests/differential/run.py` | 137 programs | 137 programs | **100%** | **100%** |
| **M2 Differential** | `tests/m2/differential.py` | 79 programs | 79 programs | **100%** | **100%** |
| **M3 Differential** | `tests/m3/differential.py` | 42 programs | 42 programs | **100%** | **100%** |
| **Acceptance Suite** | `tests/rc1_1_acceptance.py` | 9 programs | 9 programs | **100%** | **100%** |
| **Subtotal Checks** | | **798 checks** | **798 checks** | **100%** | **100%** |
| **Total Cross-Config Checks** | | **1,596 checks** | | **100%** | **100%** |

## 3. CTest Suite Summary

### Release Configuration (`ctest --test-dir build/Release --output-on-failure`)
```text
1/7 Test #1: unit .............................   Passed    0.02 sec
2/7 Test #2: m2_unit ..........................   Passed    0.02 sec
3/7 Test #3: m3_unit ..........................   Passed    0.02 sec
4/7 Test #4: rc1_1_unit .......................   Passed    0.02 sec
5/7 Test #5: m3_integration ...................   Passed    3.11 sec
6/7 Test #6: m2_integration ...................   Passed    7.03 sec
7/7 Test #7: integration ......................   Passed   13.21 sec

100% tests passed, 0 tests failed out of 7
Total Test time (real) = 23.45 sec
```

### Debug Configuration (`ctest --test-dir build/Debug --output-on-failure`)
```text
1/7 Test #1: unit .............................   Passed    0.54 sec
2/7 Test #2: m2_unit ..........................   Passed    0.65 sec
3/7 Test #3: m3_unit ..........................   Passed    0.72 sec
4/7 Test #4: rc1_1_unit .......................   Passed    0.78 sec
5/7 Test #5: m3_integration ...................   Passed    4.09 sec
6/7 Test #6: m2_integration ...................   Passed    8.75 sec
7/7 Test #7: integration ......................   Passed   15.11 sec

100% tests passed, 0 tests failed out of 7
Total Test time (real) = 30.64 sec
```

## 4. Zero Regressions Confirmed

Zero regressions occurred against M1, M2, or M3 baselines. All 258 differential programs passed byte-for-byte comparisons against MSVC in both configurations.
