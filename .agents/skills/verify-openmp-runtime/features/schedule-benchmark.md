# Schedule benchmark

## Sub-features

The benchmark compares a deterministic irregular loop under LOMP static,
dynamic, guided, and FAC2 scheduling. It also runs the same source and standard
schedules with Clang's installed OpenMP runtime.

## How to get to it (user POV)

Run the native verification driver on Ubuntu. It builds both binaries and
records every sample in the retained run artifact.

## Driving it with the native verification driver

Run `bash scripts/verify-openmp-runtime.sh all`. Each binary warms up all of
its schedules, then rotates schedule order across five trials. The driver runs
five paired processes at two and four threads. The benchmark checks the output
against the same serial oracle and records elapsed nanoseconds and iterations
per worker.

## Gotchas

The benchmark does not report exact per-worker chunk counts or establish a
speedup. Do not compare a run whose workload checksum or actual team size
failed.
