# Parallel synchronization

## Sub-features

Existing barriers, critical sections, locks, reductions used by scheduled
loops, and task completion at parallel, explicit-barrier, and single barriers.

## How to get to it (user POV)

Compile and run the existing synchronization and reduction examples/tests
against this repository's `libomp`.

## Driving it with the native CTest and consumer harness

Run the existing `locks`, `critical`, `parallel_reduction`, `for_reduction`,
`task_forkjoin`, `task_barrier`, and `task_single` CTest cases at the configured
team sizes, plus the scheduled-loop consumer. Task tests assert each generated
task completes exactly once and that barrier paths observe all tasks complete;
they do not assert a particular thread distribution.

## Gotchas

The checkout rejects Windows and requires native Linux or macOS for runtime
execution. OpenMP leaves task scheduling choices unspecified and requires all
team tasks to complete before a barrier releases; see the official [task
scheduling rules](https://www.openmp.org/spec-html/5.2/openmpse77.html) and
[barrier semantics](https://www.openmp.org/spec-html/5.2/openmpsu4.html).
