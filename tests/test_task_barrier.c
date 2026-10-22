//===-- test_task_barrier.c - Test tasks at a barrier -------------*- C -*-===//
//
// Part of the LOMP project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//
// This file has been modified from the file
//    openmp/runtime/test/tasking/omp_task.c
// of the LLVM project (https://github.com/llvm/llvm-project)
// under the Apache License v2.0 with LLVM Exceptions.
//
//===----------------------------------------------------------------------===//

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <unistd.h>

#include <omp.h>

#include "tests.h"

static int test_omp_task(void) {
  atomic_int task_counts[NUM_TASKS];
  atomic_int completed;
  atomic_int invalid_tasks;
  atomic_int barrier_failures;
  for (int i = 0; i < NUM_TASKS; ++i) {
    atomic_init(&task_counts[i], 0);
  }
  atomic_init(&completed, 0);
  atomic_init(&invalid_tasks, 0);
  atomic_init(&barrier_failures, 0);

#pragma omp parallel shared(task_counts, completed, invalid_tasks, barrier_failures)
  {
#pragma omp single nowait
    {
      for (int i = 0; i < NUM_TASKS; ++i) {
        printf("Create task %d\n", i);
#pragma omp task firstprivate(i)
        {
          sleep(SLEEPTIME);
          if (i < 0 || i >= NUM_TASKS) {
            atomic_fetch_add(&invalid_tasks, 1);
          } else {
            atomic_fetch_add(&task_counts[i], 1);
          }
          atomic_fetch_add(&completed, 1);
          printf("Executed task %d in thread %d\n", i,
                 omp_get_thread_num());
        }
      }
    }

    printf("Before barrier\n");
#pragma omp barrier
    // OpenMP requires all team tasks to finish before any thread leaves this
    // barrier; task-to-thread distribution is intentionally not asserted.
    if (atomic_load(&completed) != NUM_TASKS) {
      atomic_fetch_add(&barrier_failures, 1);
    }
    printf("After barrier\n");
  }

  if (atomic_load(&completed) != NUM_TASKS ||
      atomic_load(&invalid_tasks) != 0 ||
      atomic_load(&barrier_failures) != 0) {
    return 1;
  }
  for (int i = 0; i < NUM_TASKS; ++i) {
    if (atomic_load(&task_counts[i]) != 1) {
      return 1;
    }
  }
  return 0;
}

int main(void) {
  int failures = 0;
  for (int repetition = 0; repetition < REPETITIONS; ++repetition) {
    failures += test_omp_task();
  }
  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
