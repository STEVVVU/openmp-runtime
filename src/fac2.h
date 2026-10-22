//===-- fac2.h - Factoring-2 loop claims -----------------------*- C++ -*-===//
//
// Part of the LOMP project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef FAC2_H_INCLUDED
#define FAC2_H_INCLUDED

#include <algorithm>
#include <cstdint>
#include <mutex>

namespace lomp {

// Claims canonical iteration indices in FAC2 batches. Every claim advances
// next_ and consumes one batch slot under the same descriptor-local mutex.
class fac2Scheduler {
  std::mutex mutex_;
  uint64_t total_ = 0;
  uint64_t next_ = 0;
  uint64_t minimumChunk_ = 1;
  uint64_t batchChunk_ = 0;
  uint32_t workers_ = 1;
  uint32_t claimsLeft_ = 0;

public:
  void reset(uint64_t total, uint64_t minimumChunk, uint32_t workers) {
    std::lock_guard<std::mutex> guard(mutex_);
    total_ = total;
    next_ = 0;
    minimumChunk_ = std::max<uint64_t>(minimumChunk, 1);
    workers_ = std::max<uint32_t>(workers, 1);
    batchChunk_ = 0;
    claimsLeft_ = 0;
  }

  bool next(uint64_t * begin, uint64_t * count) {
    std::lock_guard<std::mutex> guard(mutex_);
    if (next_ == total_) {
      return false;
    }
    if (claimsLeft_ == 0) {
      uint64_t remaining = total_ - next_;
      uint64_t divisor = uint64_t(workers_) * 2;
      uint64_t factored = remaining / divisor + (remaining % divisor != 0);
      batchChunk_ = std::max(minimumChunk_, factored);
      claimsLeft_ = workers_;
    }
    *begin = next_;
    *count = std::min(batchChunk_, total_ - next_);
    next_ += *count;
    --claimsLeft_;
    return true;
  }
};

} // namespace lomp

#endif
