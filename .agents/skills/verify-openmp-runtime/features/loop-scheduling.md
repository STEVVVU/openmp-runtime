# Loop scheduling

## Sub-features

Runtime schedule selection, canonical loop dispatch, guided scheduling, and
the FAC2 extension through `OMP_SCHEDULE`, `omp_set_schedule`, and
`schedule(runtime)`. The native environment matrix covers static, dynamic, and
FAC2 startup selections at a nonzero chunk.

## How to get to it (user POV)

Compile a C/C++ consumer with Clang's OpenMP support and link it to this
repository's `libomp`; select a runtime schedule before entering a parallel
loop.

## Driving it with the native CTest and consumer harness

Run `tests/test_fac2.cc` from the native workflow. The end state must show each
canonical iteration exactly once, a correct serial-oracle result, and the
consumer resolving this build's `libomp.so`.

## Gotchas

LOMP intentionally does not support nested parallelism or changing team size
inside a parallel region. A Windows helper test does not exercise the runtime.
