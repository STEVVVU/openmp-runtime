//===-- test_task_forkjoin.c - Test task execution at join --------*- C -*-===//
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
  for (int i = 0; i < NUM_TASKS; ++i) {
    atomic_init(&task_counts[i], 0);
  }
  atomic_init(&completed, 0);
  atomic_init(&invalid_tasks, 0);

#pragma omp parallel shared(task_counts, completed, invalid_tasks)
  {
#pragma omp single nowait
    {
      for (int i = 0; i < NUM_TASKS; ++i) {
        printf("Create task %d\n", i);
#pragma omp task firstprivate(i)
        {
          if (i < 0 || i >= NUM_TASKS) {
            atomic_fetch_add(&invalid_tasks, 1);
          } else {
            atomic_fetch_add(&task_counts[i], 1);
          }
          sleep(SLEEPTIME);
          atomic_fetch_add(&completed, 1);
          printf("Executed task %d in thread %d\n", i,
                 omp_get_thread_num());
        }
      }
    }
  }

  // OpenMP 5.2 leaves task selection unspecified; test completion and exactly
  // once execution, not that tasks run on multiple threads. The parallel
  // region's barrier must wait for every generated task.
  if (atomic_load(&completed) != NUM_TASKS ||
      atomic_load(&invalid_tasks) != 0) {
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
