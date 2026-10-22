# Parallel synchronization

## Sub-features

Existing barriers, critical sections, locks, and reductions used by scheduled
loops.

## How to get to it (user POV)

Compile and run the existing synchronization and reduction examples/tests
against this repository's `libomp`.

## Driving it with the native CTest and consumer harness

Run the existing `locks`, `critical`, `parallel_reduction`, and `for_reduction`
CTest cases at the configured team sizes, plus the scheduled-loop consumer.
The end state is successful completion and values matching their serial
oracles.

## Gotchas

The checkout rejects Windows and requires native Linux or macOS for runtime
execution.
