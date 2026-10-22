# OpenMP runtime

A small LLVM compiler ABI runtime with an added factoring-2 (FAC2) loop
scheduler. FAC2 lets Clang-compiled `schedule(runtime)` loops use dynamically
claimed chunks whose size is recomputed after each team-sized batch.

This repository extends the complete [LOMP runtime](https://github.com/parallel-runtimes/lomp),
which provides persistent workers, compiler dispatch entry points, standard
static/dynamic/guided schedules, synchronization, reductions, and tasks. The
upstream README is preserved in [`README.upstream.md`](README.upstream.md), and
the upstream license and source history remain in the repository.

## FAC2 schedule

At the start of a batch, FAC2 computes

```text
chunk = max(minimum_chunk, ceil(remaining_iterations / (2 * team_size)))
```

It keeps that chunk size for up to one claim per team thread, then recomputes
from the remaining range. The final claim is capped at the remaining work.
Integer quotient/remainder implements ceiling division without overflowing the
addition. FAC2 is a bounded scheduling implementation inspired by the paper;
it is not a reproduction of the paper's evaluation or a guarantee of a speedup.

Select the extension for `schedule(runtime)` with either:

```sh
OMP_SCHEDULE=fac2,4 ./your-program
```

or the public runtime API:

```cpp
omp_set_schedule(lomp_sched_fac2, 4);
#pragma omp parallel for schedule(runtime)
for (int i = 0; i < count; ++i) {
  // Work for i.
}
```

The chunk argument is the minimum chunk in loop iterations. Standard OpenMP
schedule values retain their existing behavior; `auto` is not redirected to
FAC2.

## Build and verify

LOMP supports Linux and macOS. It intentionally rejects Windows in
[`src/target.h`](src/target.h). On Linux with Clang, CMake, Ninja, pthread
headers, and LLVM OpenMP development headers installed, run:

```sh
bash scripts/verify-openmp-runtime.sh all
```

The driver builds the runtime and consumers, runs the complete CTest suite,
checks that the FAC2 consumer resolves to this build's `libomp`, and compares
the same irregular loop against the installed Clang runtime. It removes only
its marked build directory. Configure, build, test, benchmark, and linkage logs
remain under `artifacts/native-runs/`. Native Linux verification passed in
[run 39718820166](https://github.com/STEVVVU/openmp-runtime/actions/runs/39718820166).
All 25 CTest entries, actual LOMP/system-library linkage checks, benchmark
checksums and the injected-failure cleanup control passed. The [retained
evidence](docs/evidence/linux-39718820166/) includes every measured sample.

The standalone arithmetic check uses only C++17 and can run on Windows. It
validates FAC2 claim arithmetic, not the OpenMP runtime.

## Native measurements

[Run 39729269942](https://github.com/STEVVVU/openmp-runtime/actions/runs/39729269942)
passed all 25 CTest entries, LOMP/LLVM linkage checks, 450 measurement processes
and the separate failure-cleanup control. Source was `88108a219e4ccbea20c591d0ff337acb1cd49826`,
using Ubuntu 24.04, Clang/LLVM OpenMP 18.1.3 Release and an AMD EPYC 7763 VM
reporting four logical CPUs on two cores. Five alternating runtime pairs per
configuration followed a schedule warmup. All measured outputs matched their
serial checksum with zero errors.

For 1048576 iterations, 4096 maximum recurrence rounds and four workers,
elapsed milliseconds were median (full range):

| Kernel | LOMP FAC2 | LLVM guided | LOMP dynamic, chunk 1 |
| --- | ---: | ---: | ---: |
| Balanced | 169.13 (168.98..171.27) | 169.02 (168.91..170.45) | 184.42 (184.17..185.10) |
| Irregular | 89.21 (89.15..89.27) | 89.23 (89.15..89.29) | 105.41 (105.29..105.56) |
| Clustered heavy prefix | 87.48 (87.21..87.53) | 86.55 (86.44..86.79) | 42.68 (42.48..42.81) |

FAC2 has no measurable advantage over LLVM guided on balanced or irregular
work here. Its balanced one/two/four-worker medians were 654.75/337.55/169.13 ms;
the corresponding serial control was about 651 ms. These are logical-worker
results on this VM, not four physical-core scaling guarantees.

The clustered case exposes a schedule tradeoff. FAC2's first chunk contains
the entire heavy prefix, leaving one worker with 99.829% of useful recurrence
work. Guided shows the same imbalance. The existing dynamic schedule is faster
for this case. Changing FAC2's initial batching to remove that imbalance would
change its documented algorithm. The complete matrix preserves that result
alongside the smaller overhead controls; 210 of 405 summary groups include a
sample below 10 ms. No general runtime or scheduling speedup is claimed.

Reproduce the full matrix, separate syscall profiles and owned cleanup with:

```sh
LOMP_VERIFY_RUN_ID=fresh-metrics LOMP_METRICS=1 bash scripts/verify-openmp-runtime.sh all
```

[All compact summaries and raw hashes](docs/evidence/linux-39729269942/) include
serial controls, per-worker work, resources, environment and profile limits.
Kernel timing includes parallel-region startup/barriers and counters, excludes
allocation and serial reference construction, and is separate from process
CPU/RSS. Peak process RSS ranged from 7988 to 21344 KiB.

## Scope and limitations

This runtime implements a subset of OpenMP through the LLVM-compatible ABI. It
does not claim full OpenMP compliance, nested parallelism, or changing a team's
size inside a parallel region. The FAC2 claim step uses one descriptor-local
mutex acquisition per chunk. No performance result is claimed without native,
interleaved measurements against equivalent work and inputs.

## Coursework and research

- The course project reference is the [CMU `omp0` final report](https://vnnm404.github.io/webpage/final.html),
  which motivates implementing the worksharing runtime and comparing loop
  schedules.
- The FAC2 mechanism follows the simplified batching formulation in
  [Buder, Ciorba, and Iwainsky, “OpenMP Loop Scheduling Revisited: Making a Case
  for More Schedules”](https://arxiv.org/abs/1809.03188) and the [author's thesis,
  section 2.2.2.5](https://hpc.dmi.unibas.ch/wp-content/uploads/sites/87/2019/11/2018_patrick_buder_ma_thesisjanuary2018.pdf).
