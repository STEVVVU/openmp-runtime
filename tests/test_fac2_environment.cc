//===-- test_fac2_environment.cc - OMP_SCHEDULE FAC2 check ----*- C++ -*-===//
//
// Part of the LOMP project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <omp.h>

#include <cstdio>
#include <cstring>
#include <vector>

int main(int argc, char ** argv) {
  if (argc != 2) {
    std::fprintf(stderr, "expected schedule name argument\n");
    return 2;
  }
  omp_sched_t expected;
  if (std::strcmp(argv[1], "fac2") == 0) {
    expected = lomp_sched_fac2;
  } else if (std::strcmp(argv[1], "dynamic") == 0) {
    expected = omp_sched_dynamic;
  } else if (std::strcmp(argv[1], "static") == 0) {
    expected = omp_sched_static;
  } else {
    std::fprintf(stderr, "unknown expected schedule: %s\n", argv[1]);
    return 2;
  }

  omp_sched_t schedule;
  int chunk;
  omp_get_schedule(&schedule, &chunk);
  if (schedule != expected || chunk != 3) {
    std::fprintf(stderr, "OMP_SCHEDULE=%s,3 selection was not retained\n",
                 argv[1]);
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
