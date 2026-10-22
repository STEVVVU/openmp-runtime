//===-- test_fac2_arithmetic.cc - FAC2 arithmetic checks -------*- C++ -*-===//
//
// Part of the LOMP project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "../src/fac2.h"

#include <cstdio>
#include <cstddef>
#include <atomic>
#include <limits>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

static std::vector<std::pair<uint64_t, uint64_t>>
claims(uint64_t total, uint64_t minimum, uint32_t workers) {
  lomp::fac2Scheduler schedule;
  schedule.reset(total, minimum, workers);
  std::vector<std::pair<uint64_t, uint64_t>> result;
  uint64_t begin;
  uint64_t count;
  while (schedule.next(&begin, &count)) {
    result.emplace_back(begin, count);
  }
  return result;
}

int main() {
  const std::vector<std::pair<uint64_t, uint64_t>> expected = {
      {0, 13}, {13, 13}, {26, 13}, {39, 13}, {52, 6}, {58, 6}, {64, 6},
      {70, 6}, {76, 3},  {79, 3},  {82, 3},  {85, 3}, {88, 2}, {90, 2},
      {92, 2}, {94, 2},  {96, 1},  {97, 1},  {98, 1}, {99, 1}};
  if (claims(100, 1, 4) != expected) {
    std::fprintf(stderr, "FAC2 batch sequence mismatch\n");
    return 1;
  }
  const std::vector<std::pair<uint64_t, uint64_t>> guidedPrefix = {
      {0, 13}, {13, 11}, {24, 10}, {34, 9}};
  bool matchesGuided = true;
  for (std::size_t i = 0; i < guidedPrefix.size(); ++i) {
    matchesGuided &= expected[i] == guidedPrefix[i];
  }
  if (matchesGuided) {
    std::fprintf(stderr, "FAC2 batch sequence matches guided prefix\n");
    return 1;
  }
  if (!claims(0, 1, 4).empty()) {
    std::fprintf(stderr, "empty FAC2 range produced work\n");
    return 1;
  }
  if (claims(3, 9, 4) != std::vector<std::pair<uint64_t, uint64_t>>{{0, 3}}) {
    std::fprintf(stderr, "FAC2 minimum chunk tail mismatch\n");
    return 1;
  }
  if (claims(3, 0, 4) !=
      std::vector<std::pair<uint64_t, uint64_t>>{{0, 1}, {1, 1}, {2, 1}}) {
    std::fprintf(stderr, "FAC2 zero minimum chunk normalization mismatch\n");
    return 1;
  }
  const auto maximum = std::numeric_limits<uint64_t>::max();
  auto boundary = claims(maximum, 1, 512);
  if (boundary.empty() || boundary.front().first != 0 ||
      boundary.front().second == 0 ||
      boundary.back().first + boundary.back().second != maximum) {
    std::fprintf(stderr, "FAC2 uint64 boundary mismatch\n");
    return 1;
  }
  constexpr uint64_t concurrentTotal = 100003;
  std::unique_ptr<std::atomic<uint32_t>[]> coverage(
      new std::atomic<uint32_t>[concurrentTotal]);
  for (uint64_t i = 0; i < concurrentTotal; ++i) {
    coverage[i].store(0, std::memory_order_relaxed);
  }
  lomp::fac2Scheduler concurrent;
  concurrent.reset(concurrentTotal, 7, 8);
  std::vector<std::thread> workers;
  for (int i = 0; i < 8; ++i) {
    workers.emplace_back([&] {
      uint64_t begin;
      uint64_t count;
      while (concurrent.next(&begin, &count)) {
        for (uint64_t index = begin; index < begin + count; ++index) {
          coverage[index].fetch_add(1, std::memory_order_relaxed);
        }
      }
    });
  }
  for (auto & worker : workers) {
    worker.join();
  }
  for (uint64_t i = 0; i < concurrentTotal; ++i) {
    if (coverage[i].load(std::memory_order_relaxed) != 1) {
      std::fprintf(stderr, "concurrent FAC2 claims lost or duplicated work\n");
      return 1;
    }
  }
  std::puts("FAC2 arithmetic checks passed");
}
