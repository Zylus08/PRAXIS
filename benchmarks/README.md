# Optional timing probe

`python benchmarks/run.py build/Release/praxis.exe` measures ten process invocations of the factorial example and writes raw seconds plus median to `build/benchmark.json`. This includes process startup and is not a microbenchmark or a milestone acceptance target. No cross-interpreter performance claim is made.
