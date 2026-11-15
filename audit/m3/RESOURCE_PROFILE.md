# PRAXIS M3 — Resource Profile & Telemetry Report

## 1. Overview

Milestone M3 incorporates comprehensive runtime profiling and telemetry counters directly into the interpreter engine. Telemetry is activated via the `--profile` CLI flag, emitting machine-readable JSON metrics upon execution completion.

---

## 2. Telemetry Schema & Profile Counters

| Profile Counter | Description |
|---|---|
| `dynamic_allocations` | Total dynamic storage allocations performed via `new` |
| `dynamic_deletions` | Total dynamic storage releases performed via `delete` |
| `container_operations` | High-level operations performed on STL containers (`vector`, `pair`, `array`) |
| `objects` | Total storage objects allocated over the program lifetime |
| `peak_live_objects` | High-water mark of simultaneously active objects |
| `live_objects` | Live objects remaining at termination (must equal 0 on clean exit) |
| `constructions` | Class object constructor executions |
| `destructors` | Class object destructor executions |
| `copies` | Class copy constructions |
| `assignments` | Class copy assignments |
| `calls` | Function calls executed |
| `preprocess_ms` | Elapsed milliseconds spent in lexing & preprocessing |
| `parse_ms` | Elapsed milliseconds spent in recursive descent parsing |
| `semantic_ms` | Elapsed milliseconds spent in static type checking |
| `runtime_ms` | Elapsed milliseconds spent in AST interpretation |
| `peak_working_set_bytes` | Peak host memory working set reported by OS (`GetProcessMemoryInfo`) |

---

## 3. Benchmark Telemetry Data

### 3.1 Representative Acceptance Program (`examples/m3/accumulator.cpp`)
```json
{
  "preprocess_ms": 0.3143,
  "parse_ms": 0.0741,
  "semantic_ms": 0.0448,
  "runtime_ms": 0.1179,
  "live_objects": 0,
  "objects": 8,
  "peak_live_objects": 6,
  "dynamic_allocations": 0,
  "dynamic_deletions": 0,
  "container_operations": 1,
  "calls": 1,
  "destructors": 1,
  "peak_working_set_bytes": 4726784
}
```

### 3.2 Dynamic Scalar Lifecycle (`examples/m3/dynamic_scalar.cpp`)
```json
{
  "preprocess_ms": 0.3377,
  "parse_ms": 0.0548,
  "semantic_ms": 0.0366,
  "runtime_ms": 0.0919,
  "live_objects": 0,
  "objects": 2,
  "peak_live_objects": 2,
  "dynamic_allocations": 1,
  "dynamic_deletions": 1,
  "container_operations": 0,
  "calls": 1,
  "destructors": 0,
  "peak_working_set_bytes": 4714496
}
```

---

## 4. Resource Efficiency Observations

1. **Sub-Millisecond Pipeline:**  
   The entire interpreted execution cycle (preprocessing, parsing, semantic analysis, and runtime interpretation) finishes in under **0.6 milliseconds** for realistic benchmark fixtures.
2. **Compact Working Set:**  
   The peak process working set stays below **4.8 MB** throughout execution, reflecting minimal memory footprint without bloat.
3. **Leak-Free Guarantee:**  
   Across all verified benchmarks, `live_objects` reliably drops to `0` upon completion, demonstrating deterministic cleanup of all heap and automatic storage.
