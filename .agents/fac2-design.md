# FAC2 scheduling design

The shared per-loop data is the remaining canonical iteration count, current
batch chunk size, and number of claims left in that batch. The `dynamicLoop`
descriptor owns these values because the compiler ABI already shares that
descriptor among threads for one dynamic loop and keeps separate descriptors
for overlapping `nowait` loops.

FAC2 claims are serialized by a per-descriptor mutex. At a batch boundary the
runtime computes `ceil(remaining / (2 * team_size))`, raises it to the explicit
minimum chunk, and caps each claim at remaining work. It retains that chunk for
at most one claim per team thread, then recomputes from the remaining work.
Unsigned division and remainder avoid overflow in the ceiling calculation;
the denominator is bounded by the runtime's team limit. This is the paper's
bounded FAC2 mechanism, with integer rounding and the OpenMP minimum chunk
preserved. It is not the paper's experimental implementation or a performance
claim.

The explicit ABI boundary is the normal `__kmpc_dispatch_init_*` / dispatch
next path. The public extension is selected through `OMP_SCHEDULE=fac2[,chunk]`
or `omp_set_schedule(lomp_sched_fac2, chunk)`; standard schedule meanings and
`auto` remain unchanged. Setup resets batch state whenever a descriptor is
reused.

One short mutex-protected claim per chunk is accepted. It makes the whole FAC2
state transition coherent and avoids a split atomic protocol for next-index,
batch-size, and batch-count. A native test must check the deterministic chunk
sequence, exact coverage, reduction, signed canonical loops, and independent
loop descriptors before calling the runtime complete.
