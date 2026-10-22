---
name: verify-openmp-runtime
description: Drive and verify the LOMP compiler ABI runtime on native Linux after changing runtime or scheduling behavior.
---

# Verification status: DRAFT

The first native workflow run exposed four CTest failures; see the maintained
P26 implementation report and retained evidence for their exact diagnostics.
The initialization recursion fix, expected unsupported-team-size fixture, and
long-task timeout adjustment still require a successful hosted rerun.
`src/target.h` rejects Windows, and this machine has no local Linux runtime
environment. Do not treat portable arithmetic checks as proof of the LOMP
runtime.

## Launch

From the repository root on Ubuntu with CMake, Ninja, Clang 18, LLVM OpenMP 18
headers, and pthread development files installed:

```sh
LOMP_VERIFY_RUN_ID=local-check bash scripts/verify-openmp-runtime.sh launch
```

The configure and build commands are short-lived; no runtime service stays
running. Use the same run ID for each driver action in a local lifecycle. Build
outputs belong only in `build/verify-openmp-runtime/<run-id>/`.

## Doctor

Run before driving if the host or build is uncertain:

```sh
test "$(uname -s)" = Linux
command -v clang-18
command -v clang++-18
command -v cmake
command -v ninja
LOMP_VERIFY_RUN_ID=local-check bash scripts/verify-openmp-runtime.sh doctor
```

If any command fails, stop and report the missing prerequisite or build output.
The driver passes `clang-18` and `clang++-18` explicitly to CMake and uses
`clang++-18` for the system-runtime comparison, keeping its compiler and
`libomp-18-dev` installation aligned even when another Clang is earlier on
`PATH`.

## Drive

From the repository root:

```sh
LOMP_VERIFY_RUN_ID=local-check bash scripts/verify-openmp-runtime.sh drive
LOMP_VERIFY_RUN_ID=local-check bash scripts/verify-openmp-runtime.sh evidence
```

The native scheduling journey is `tests/test_fac2.cc`:
compiles and runs against the just-built `libomp`, checks exact coverage and
reductions, and exercises descriptor reuse. Compare the executable's resolved
library path with `build/verify-openmp/src/libomp.so` using `ldd`; system
`libomp` is not acceptable evidence for LOMP behavior. The driver also runs
the same irregular loop benchmark against LOMP and the installed Clang
runtime. Each process warms all schedules, then interleaves five trials per
schedule. It repeats five paired process runs at two and four threads.

## Evidence

Keep configure, build, CTest, benchmark, and linkage output under
`artifacts/native-runs/<run-id>/`. The benchmark prints elapsed nanoseconds,
iteration counts, and work per thread as JSON lines. Preserve output from
failed runs before cleanup. These measurements do not establish a speedup.
A passing portable helper test alone does not establish ABI, concurrency, or
runtime correctness.

## Cleanup

This project has no persistent verification service. The workflow owns the
ephemeral runner. Local cleanup may remove only the build directory created by
these Launch commands:

```sh
LOMP_VERIFY_RUN_ID=local-check bash scripts/verify-openmp-runtime.sh cleanup
```

Do not remove retained CI logs or curated evidence. Never kill processes by
name; the driver creates no long-lived processes.

`all` and `negative-control` write a durable `result.json` and install INT/TERM
traps that clean only the marked build directory while retaining evidence.
The workflow runs `negative-control` under a second run ID; it builds and
passes Doctor, injects an intentional command failure without running CTest or
the benchmark suite, then verifies a nonzero exit, `passed:false`, successful
cleanup, and retained failure log.

## Helpers

The workflow at `.github/workflows/linux-runtime.yml` invokes the maintained
driver's `all` action. It installs only Ubuntu runner packages, builds the
checked-out source, runs CTest and the compiler-ABI consumer, compares the
same workload against the installed Clang runtime, captures linkage, cleans
only its owned build, and retains logs as a workflow artifact. It is read-only
with respect to deployments and releases.
