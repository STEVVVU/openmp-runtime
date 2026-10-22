//===-- test_fac2.cc - FAC2 OpenMP runtime checks --------------*- C++ -*-===//
//
// Part of the LOMP project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <omp.h>

#include <atomic>
#include <cstdio>
#include <cstdint>
#include <thread>
#include <vector>

static bool check(bool condition, char const * message) {
  if (!condition) {
    std::fprintf(stderr, "%s\n", message);
  }
  return condition;
}

int main() {
  omp_set_num_threads(4);
  omp_set_schedule(lomp_sched_fac2, 1);
  bool passed = true;
  omp_sched_t selected;
  int selectedChunk = 0;
  omp_get_schedule(&selected, &selectedChunk);
  passed &= check(selected == lomp_sched_fac2 && selectedChunk == 1,
                  "omp_get_schedule did not return FAC2 selection");

  for (int repeat = 0; repeat < 100; ++repeat) {
    std::vector<int> seen(1000, 0);
    long long sum = 0;
#pragma omp parallel for schedule(runtime) reduction(+ : sum)
    for (int i = -10; i < 2990; i += 3) {
      int index = (i + 10) / 3;
#pragma omp atomic update
      ++seen[index];
      sum += i;
    }
    long long expected = 0;
    for (int i = -10; i < 2990; i += 3) {
      expected += i;
    }
    passed &=
        check(sum == expected, "FAC2 reduction differs from serial oracle");
    for (int count : seen) {
      passed &= check(count == 1, "FAC2 lost or duplicated an iteration");
    }
  }

  std::vector<int> first(257, 0);
  std::vector<int> second(193, 0);
#pragma omp parallel num_threads(4)
  {
#pragma omp for schedule(runtime) nowait
    for (int i = 0; i < static_cast<int>(first.size()); ++i) {
#pragma omp atomic update
      ++first[i];
    }
#pragma omp for schedule(runtime)
    for (int i = 0; i < static_cast<int>(second.size()); ++i) {
#pragma omp atomic update
      ++second[i];
    }
  }
  for (int count : first) {
    passed &= check(count == 1, "FAC2 nowait loop lost or duplicated work");
  }
  for (int count : second) {
    passed &= check(count == 1, "FAC2 overlapping loop state leaked");
  }

  std::atomic<int> secondEntered{0};
  std::atomic<int> firstThreadsDone{0};
#pragma omp parallel num_threads(4)
  {
#pragma omp for schedule(runtime) nowait
    for (int i = 0; i < 257; ++i) {
      if (i == 0) {
        while (secondEntered.load(std::memory_order_acquire) == 0) {
          std::this_thread::yield();
        }
      }
#pragma omp atomic update
      ++first[i];
    }
    firstThreadsDone.fetch_add(1, std::memory_order_relaxed);
#pragma omp for schedule(runtime)
    for (int i = 0; i < 193; ++i) {
      if (i == 0) {
        secondEntered.fetch_add(1, std::memory_order_release);
      }
#pragma omp atomic update
      ++second[i];
    }
  }
  passed &= check(firstThreadsDone.load(std::memory_order_relaxed) == 4,
                  "FAC2 nowait producer did not complete all threads");
  for (int count : first) {
    passed &= check(count == 2, "FAC2 repeated nowait loop was incomplete");
  }
  for (int count : second) {
    passed &= check(count == 2, "FAC2 repeated loop descriptor leaked state");
  }

  std::vector<int> signed64(500, 0);
#pragma omp parallel for schedule(runtime)
  for (int64_t i = 7000; i > 2000; i -= 10) {
    int index = static_cast<int>((7000 - i) / 10);
#pragma omp atomic update
    ++signed64[index];
  }
  for (int count : signed64) {
    passed &= check(count == 1, "FAC2 signed 64-bit loop coverage mismatch");
  }

  std::vector<int> unsigned32(250, 0);
#pragma omp parallel for schedule(runtime)
  for (uint32_t i = 5; i < 1005; i += 4) {
    int index = static_cast<int>((i - 5) / 4);
#pragma omp atomic update
    ++unsigned32[index];
  }
  for (int count : unsigned32) {
    passed &= check(count == 1, "FAC2 unsigned 32-bit loop coverage mismatch");
  }

  std::vector<int> unsigned64(200, 0);
#pragma omp parallel for schedule(runtime)
  for (uint64_t i = 11; i < 2011; i += 10) {
    int index = static_cast<int>((i - 11) / 10);
#pragma omp atomic update
    ++unsigned64[index];
  }
  for (int count : unsigned64) {
    passed &= check(count == 1, "FAC2 unsigned 64-bit loop coverage mismatch");
  }

  omp_set_schedule(lomp_sched_fac2, 11);
  int tail = 0;
#pragma omp parallel for schedule(runtime) reduction(+ : tail)
  for (int i = 0; i < 7; ++i) {
    ++tail;
  }
  passed &= check(tail == 7, "FAC2 minimum chunk tail is incomplete");

  if (!passed) {
    return 1;
  }
  std::puts("FAC2 runtime checks passed");
  return 0;
}
