//===-- test_fac2_environment.cc - OMP_SCHEDULE FAC2 check ----*- C++ -*-===//
//
// Part of the LOMP project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <omp.h>

#include <cstdio>
#include <vector>

int main() {
  omp_sched_t schedule;
  int chunk;
  omp_get_schedule(&schedule, &chunk);
  if (schedule != lomp_sched_fac2 || chunk != 3) {
    std::fprintf(stderr, "OMP_SCHEDULE FAC2 selection was not retained\n");
    return 1;
  }
  std::vector<int> seen(97, 0);
#pragma omp parallel for schedule(runtime)
  for (int i = 0; i < static_cast<int>(seen.size()); ++i) {
#pragma omp atomic update
    ++seen[i];
  }
  for (int count : seen) {
    if (count != 1) {
      std::fprintf(stderr, "OMP_SCHEDULE FAC2 did not cover the loop\n");
      return 1;
    }
  }
  return 0;
}
