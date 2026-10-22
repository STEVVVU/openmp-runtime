# OpenMP runtime repository

This is LOMP, a compiler ABI runtime. Keep changes inside its existing ABI,
team, and loop-descriptor design; do not add a parallel scheduler or claim full
OpenMP compliance.

## Platform and commands

- LOMP supports Linux and macOS. `src/target.h` deliberately rejects Windows,
  so Windows may run portable helper tests but cannot prove the runtime.
- Configure on Linux with `cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++`.
- Build with `cmake --build build --parallel` and run the suite with
  `ctest --test-dir build --output-on-failure`.
- The repository's verification driver and feature map are in
  `.agents/skills/verify-openmp-runtime/`. Its mapped native Linux paths passed
  in run 39718820166; re-drive affected paths after changes.
- Keep generated builds and run output outside tracked source. Preserve failed
  test logs and evidence; never label a Windows helper check as runtime proof.

## Compatibility

Preserve the upstream source, compiler-facing ABI, history, license, notices,
and existing static, dynamic, guided, and auto scheduling semantics. Add new
schedule behavior through the existing `dynamicLoop` and runtime schedule
selection paths. Keep descriptor state isolated across concurrent `nowait`
loops and reset it before descriptor reuse.
