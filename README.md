# Project PRAXIS — M1

Praxis is a standalone C++20 implementation of a **restricted C++ interpreter**. It lexes, preprocesses, parses, type-checks, and tree-walks user source itself. It does not compile user programs or depend on a compiler at runtime. This is not a fully conforming C++ implementation.

## Build and use (Windows x64)

Install Visual Studio 2022 Build Tools with C++ tools and CMake/Ninja. Python 3 is used only for verification.

```powershell
./tools/build.ps1 Debug
./tools/build.ps1 Release
./build/Release/praxis.exe --version
./build/Release/praxis.exe --help
./build/Release/praxis.exe run examples/hello.cpp
./build/Release/praxis.exe check examples/hello.cpp
./build/Release/praxis.exe run my_program.cpp -I my_headers
./build/Release/praxis.exe repl
```

`build.ps1` discovers the existing MSVC installation without changing persistent environment or Git configuration. Builds run sequentially to limit compiler memory usage. Debug uses `/Od /Z7 /RTC1`; Release uses `/O2`. Both use `/W4 /permissive-`. The executable uses the corresponding MSVC runtime; the Debug runtime is intended for development machines.

Standard CMake is also supported from a developer shell:

```powershell
cmake -S . -B build/Release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release --parallel 1
ctest --test-dir build/Release --output-on-failure
```

## A small program

```cpp
#include <iostream>
long long factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
int main() {
    std::cout << factorial(10) << std::endl;
}
```

The bundled `iostream` and `string` headers are virtual capability definitions implemented by Praxis. They never load toolchain headers. Project headers are read from the source directory and explicit `-I` paths.

## Verification

```powershell
./tools/verify.ps1 Debug -Differential
./tools/verify.ps1 Release -Differential
```

Only `tests/differential/run.py` compiles fixture programs with MSVC, as an external oracle. The interpreter executable has no process-launch or compiler integration code. The harness compares stdout and exit status, re-runs Praxis for determinism, and records reference-compilation failures separately. Release verification may reuse actual Debug-reference outputs only when each source SHA-256 matches; the Release interpreter is still checked and run twice. Program state is made observable through output assertions. Runtime-error fixtures are intentionally excluded from reference execution.

CTest runs C++ frontend unit assertions and Python integration fixtures. Reports are written to `audit/m1/`. See [test results](audit/m1/TEST_RESULTS.md), [differential results](audit/m1/DIFFERENTIAL_RESULTS.md), and the [milestone report](audit/m1/M1_FINAL_REPORT.md).

## REPL

Enter declarations and statements with semicolons. Definitions and blocks may span lines with balanced braces. `#include` and object macros persist, as do variables, references, and function definitions. `:quit` exits; `:reset` clears state. Output is explicit through `std::cout`; expressions are not implicitly printed.

A submission is parsed and checked against the declaration history before execution. Static rejection does not commit the submission or preprocessing changes. Runtime failure does not roll back preceding mutations or output; its declaration remains in history. Redeclaration and function replacement are rejected. A declared function must already have a definition by the end of any submission that calls it. Top-level REPL objects have static storage. EOF with an incomplete submission returns failure. Recoverable submission errors are reported on stderr while the session continues.

See [language support](docs/LANGUAGE_SUPPORT.md), [semantics](docs/SEMANTICS.md), [execution model](docs/EXECUTION_MODEL.md), [architecture](docs/ARCHITECTURE.md), and [limitations](docs/LIMITATIONS.md) before using the subset.
