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
constexpr int MinimumTrials = 1;
int iterations = Iterations;
int rounds = 64;
char const * kernel = "irregular";

int cost(int index) {
  if (std::strcmp(kernel, "balanced") == 0) return rounds;
  if (std::strcmp(kernel, "clustered") == 0)
    return index < iterations / 8 ? rounds : 1;
  return 1 + (uint32_t(index) * 17 + 31) % rounds;
}

struct alignas(64) workerCount {
  uint64_t iterations = 0;
  uint64_t work = 0;
};

struct scheduleCase {
  char const * name;
  omp_sched_t schedule;
  int chunk;
};

uint64_t work(int index) {
  uint64_t value = uint64_t(index) + 1;
  int count = cost(index);
  for (int i = 0; i < count; ++i) {
    value = value * 6364136223846793005ULL + 1442695040888963407ULL;
  }
  return value;
}

bool run(scheduleCase const & selected, int workers, int trial,
         std::vector<uint64_t> const & expected) {
  std::vector<uint64_t> output(iterations, 0);
  std::vector<workerCount> counts(workers);
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
    for (int i = 0; i < iterations; ++i) {
      output[i] = work(i);
      ++counts[thread].iterations;
      counts[thread].work += cost(i);
    }
  }
  auto end = std::chrono::steady_clock::now();

  if (actualWorkers != workers || output != expected ||
      std::accumulate(counts.begin(), counts.end(), uint64_t(0),
                      [](uint64_t sum, workerCount const & c) {
                        return sum + c.iterations;
                      }) != uint64_t(iterations)) {
    std::fprintf(stderr, "correctness or team-size check failed for %s\n",
                 selected.name);
    return false;
  }
  auto elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  std::printf("{\"trial\":%d,\"schedule\":\"%s\",\"threads\":%d,"
              "\"elapsed_ns\":%lld,\"iterations\":%d,\"kernel\":\"%s\","
              "\"rounds\":%d,\"errors\":0,\"checksum\":%llu,"
              "\"worker_iterations\":[",
              trial, selected.name, workers, static_cast<long long>(elapsed),
              iterations, kernel, rounds,
              static_cast<unsigned long long>(std::accumulate(output.begin(), output.end(), uint64_t(0))));
  for (int i = 0; i < workers; ++i) {
    std::printf("%s%llu", i ? "," : "",
                static_cast<unsigned long long>(counts[i].iterations));
  }
  std::printf("],\"worker_work\":[");
  for (int i = 0; i < workers; ++i)
    std::printf("%s%llu", i ? "," : "", static_cast<unsigned long long>(counts[i].work));
  std::puts("]}");
  return true;
}
} // namespace

int main(int argc, char ** argv) {
  if (argc < 3) {
    std::fprintf(stderr,
                 "usage: %s <threads:1..64> <trials:5+> [--standard-only] [--kernel balanced|irregular|clustered] [--iterations N] [--rounds N]\n",
                 argv[0]);
    return 2;
  }
  int workers = std::atoi(argv[1]);
  int trials = std::atoi(argv[2]);
  bool standardOnly = false;
  for (int i = 3; i < argc; ++i) {
    if (std::strcmp(argv[i], "--standard-only") == 0) standardOnly = true;
    else if (i + 1 < argc && std::strcmp(argv[i], "--kernel") == 0) kernel = argv[++i];
    else if (i + 1 < argc && std::strcmp(argv[i], "--iterations") == 0) iterations = std::atoi(argv[++i]);
    else if (i + 1 < argc && std::strcmp(argv[i], "--rounds") == 0) rounds = std::atoi(argv[++i]);
    else return 2;
  }
  if (workers < 1 || workers > 64 || trials < MinimumTrials || trials > 100 ||
      iterations < 1 || iterations > 4194304 || rounds < 1 || rounds > 4096 ||
      (std::strcmp(kernel, "balanced") && std::strcmp(kernel, "irregular") && std::strcmp(kernel, "clustered"))) {
    std::fprintf(stderr, "invalid benchmark arguments\n");
    return 2;
  }

  scheduleCase schedules[] = {{"static", omp_sched_static, 0},
                              {"dynamic", omp_sched_dynamic, 1},
                              {"guided", omp_sched_guided, 1},
                              {"fac2", lomp_sched_fac2, 1}};
  size_t scheduleCount = standardOnly ? 3 : 4;
  std::vector<uint64_t> expected(iterations);
  auto serialStart = std::chrono::steady_clock::now();
  for (int i = 0; i < iterations; ++i) {
    expected[i] = work(i);
  }
  auto serialNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now() - serialStart).count();
  std::printf("{\"schedule\":\"serial\",\"threads\":1,\"trial\":0,\"kernel\":\"%s\",\"iterations\":%d,\"rounds\":%d,\"elapsed_ns\":%lld,\"errors\":0,\"checksum\":%llu}\n",
      kernel, iterations, rounds, static_cast<long long>(serialNs),
      static_cast<unsigned long long>(std::accumulate(expected.begin(), expected.end(), uint64_t(0))));
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
