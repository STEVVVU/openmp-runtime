# Native scheduling campaign

[Run 39729269942](https://github.com/STEVVVU/openmp-runtime/actions/runs/39729269942)
executed source `88108a219e4ccbea20c591d0ff337acb1cd49826`. Ubuntu 24.04,
Linux 7.0 Azure, Clang 18.1.3 and system LLVM OpenMP 18.1.3; Release builds.
AMD EPYC 7763 VM reports four logical CPUs, two cores and SMT. Runtime linkage
logs resolve the LOMP consumer to its just-built libomp.so.1 and the comparison
to system libomp.so.5. All 25 native CTest entries passed. Normal receipt has
passed/cleanup_succeeded true; separate injected receipt has passed false,
cleanup_succeeded/injected_failure true. Original receipts remain in the full
workflow artifact `openmp-runtime-39729269942-1`.

[receipt.json](receipt.json) is the compact normal receipt plus explicit source
and validated row counts. [summary.csv](summary.csv) preserves all 405 summary
groups with five samples each. [raw-sha256.txt](raw-sha256.txt) hashes all 450
measurement logs, original summary, validation/profile logs, environment,
CTest, linkage and terminal receipt (460 files). The maintained summarizer
independently reproduced the original summary exactly from all raw logs.
Every schedule output matches its serial reference with zero errors; iteration
counts cover the requested range. Warmup rows are excluded from summaries.

The frozen original matrix covers balanced/irregular/clustered work,
262144/1048576 iterations, 32/256 recurrence limits and 1/2/4 workers. A longer
1048576-iteration/4096-round tier was added after the preserved first run showed
mostly short samples. It retains all original configurations. Each cell has
five process pairs alternating LOMP/LLVM order. Each process times its serial
control, warms all supported schedules once, then measures static, dynamic,
guided and (LOMP only) FAC2. Dynamic and guided use chunk 1; no comparison
against dynamic here establishes that chunk 1 is universally tuned.

Long four-worker balanced FAC2 median 169.134 ms (168.981..171.269) overlaps
LLVM guided 169.024 ms (168.909..170.446). Irregular FAC2 89.209 ms
(89.148..89.272) overlaps guided 89.226 ms (89.154..89.288). No measurable
advantage in those cells. Balanced FAC2 one/two/four-worker medians are
654.746/337.546/169.134 ms; serial controls about 651 ms. All longer-tier rows
exceed 10 ms. Across the whole matrix, 210 groups contain a sub-10 ms sample
and remain overhead controls.

Clustered work places all 4096-round iterations in the first eighth of the
range and uses one round elsewhere. FAC2's first four-worker chunk is exactly
one eighth of the range, so one worker receives 536870912 of 537788416 useful
rounds (99.829%). The raw per-worker counters confirm this in each measured
trial. Guided shares the same dominant-work fraction. LOMP dynamic's median
heaviest-worker share is 44.97%, and it takes 42.676 ms (42.483..42.811), versus
FAC2 87.483 ms (87.214..87.525). This directly explains an imbalance limit in
this adversarial ordering. The existing dynamic schedule addresses this case;
altering FAC2's chunk rule would change its public algorithm, so no hidden
runtime semantic change was made to improve the benchmark.

Separate unreported two-worker clustered profiles have 118 syscalls/0.003108
seconds total traced syscall time for LOMP and 6556/0.049529 seconds for LLVM;
LLVM includes 6370 sched_yield calls. Those are whole-process diagnostics, not
kernel timings or instruction-level CPU attribution. They do not establish
the precise dynamic dispatch CPU cost. Counter overhead and parallel-region
startup/barriers remain in kernel timing. Allocation, serial reference and
process startup are excluded there; /usr/bin/time -v includes them in CPU/RSS.
Peak RSS across measurement processes is 7988..21344 KiB. No cache/NUMA,
general performance, full OpenMP compliance or physical-core scaling claim.

```sh
LOMP_VERIFY_RUN_ID=fresh-metrics LOMP_METRICS=1 bash scripts/verify-openmp-runtime.sh all
```
