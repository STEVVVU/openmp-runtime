//===-- bench_fac2.cc - Compare runtime loop schedulers --------*- C++ -*-===//
//
// Part of the LOMP project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <omp.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <vector>

namespace {
constexpr int Iterations = 32768;
constexpr int MinimumTrials = 5;

struct scheduleCase {
  char const * name;
  omp_sched_t schedule;
  int chunk;
};

uint64_t work(int index) {
  uint64_t value = uint64_t(index) + 1;
  uint32_t rounds = 8 + (uint32_t(index) * 17 + 31) % 57;
  for (uint32_t i = 0; i < rounds; ++i) {
    value = value * 6364136223846793005ULL + 1442695040888963407ULL;
  }
  return value;
}

bool run(scheduleCase const & selected, int workers, int trial,
         std::vector<uint64_t> const & expected) {
  std::vector<uint64_t> output(Iterations, 0);
  std::vector<uint64_t> workerIterations(workers, 0);
  int actualWorkers = 0;
  omp_set_schedule(selected.schedule, selected.chunk);
  auto start = std::chrono::steady_clock::now();
#pragma omp parallel num_threads(workers)
  {
    int thread = omp_get_thread_num();
    if (thread == 0) {
      actualWorkers = omp_get_num_threads();
    }
#pragma omp for schedule(runtime)
    for (int i = 0; i < Iterations; ++i) {
      output[i] = work(i);
      ++workerIterations[thread];
    }
  }
  auto end = std::chrono::steady_clock::now();

  if (actualWorkers != workers || output != expected ||
      std::accumulate(workerIterations.begin(), workerIterations.end(),
                      uint64_t(0)) != Iterations) {
    std::fprintf(stderr, "correctness or team-size check failed for %s\n",
                 selected.name);
    return false;
  }
  auto elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  std::printf("{\"trial\":%d,\"schedule\":\"%s\",\"threads\":%d,"
              "\"elapsed_ns\":%lld,\"iterations\":%d,"
              "\"worker_iterations\":[",
              trial, selected.name, workers, static_cast<long long>(elapsed),
              Iterations);
  for (int i = 0; i < workers; ++i) {
    std::printf("%s%llu", i ? "," : "",
                static_cast<unsigned long long>(workerIterations[i]));
  }
  std::puts("]}");
  return true;
}
} // namespace

int main(int argc, char ** argv) {
  if (argc < 3 || argc > 4) {
    std::fprintf(stderr,
                 "usage: %s <threads:2|4> <trials:5+> [--standard-only]\n",
                 argv[0]);
    return 2;
  }
  int workers = std::atoi(argv[1]);
  int trials = std::atoi(argv[2]);
  if ((workers != 2 && workers != 4) || trials < MinimumTrials ||
      (argc == 4 && std::strcmp(argv[3], "--standard-only") != 0)) {
    std::fprintf(stderr, "invalid benchmark arguments\n");
    return 2;
  }

  scheduleCase schedules[] = {{"static", omp_sched_static, 0},
                              {"dynamic", omp_sched_dynamic, 1},
                              {"guided", omp_sched_guided, 1},
                              {"fac2", lomp_sched_fac2, 1}};
  size_t scheduleCount = argc == 4 ? 3 : 4;
  std::vector<uint64_t> expected(Iterations);
  for (int i = 0; i < Iterations; ++i) {
    expected[i] = work(i);
  }
  omp_set_num_threads(workers);

  for (size_t i = 0; i < scheduleCount; ++i) {
    if (!run(schedules[i], workers, -1, expected)) {
      return 1;
    }
  }
  for (int trial = 0; trial < trials; ++trial) {
    for (size_t offset = 0; offset < scheduleCount; ++offset) {
      size_t selected = (offset + trial) % scheduleCount;
      if (!run(schedules[selected], workers, trial, expected)) {
        return 1;
      }
    }
  }
  return 0;
}
