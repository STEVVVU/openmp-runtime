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

The optional hosted `metrics` input sets `LOMP_METRICS=1` on the same maintained
driver. It sweeps balanced/irregular/clustered kernels, 262144/1048576 iterations,
32/256 maximum recurrence rounds and 1/2/4 workers capped at available CPUs.
The first hosted matrix showed 207 of 324 summary groups below 10ms. A separate
4096-round confirmation tier at 1048576 iterations preserves the original cells
and adds longer useful-work samples. It uses the same five alternating process
pairs; separate clustered profiles also use 4096 rounds.
Each configuration has five alternating LOMP/LLVM process pairs. Each process
runs an untimed-output serial control, one schedule warmup (`trial=-1`) and one
measured trial. Serial rows contain their own elapsed time. Per-worker iteration
and recurrence counts measure imbalance. Counters use separate cache lines.
`/usr/bin/time -v` records process CPU and peak RSS separately from kernel time.
Environment, source SHA, errors, checksums and raw rows survive owned cleanup.
Short samples establish overhead only. Inspect median/range and separate profile
evidence before making a performance claim. No locality guarantee is implied.

The benchmark does not report exact per-worker chunk counts or establish a
speedup. Do not compare a run whose workload checksum or actual team size
failed.
