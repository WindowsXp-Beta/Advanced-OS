/******************************************************************************
 * FILE: omp_hello.c
 * DESCRIPTION:
 *   OpenMP Example - Hello World - C/C++ Version
 *   In this simple example, the master thread forks a parallel region.
 *   All threads in the team obtain their unique thread number and print it.
 *   The master thread only prints the total number of threads.  Two OpenMP
 *   library routines are used to obtain the number of threads and each
 *   thread's number.
 * AUTHOR: Blaise Barney  5/99
 * LAST REVISED: 04/06/05
 ******************************************************************************/
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "gtmp.h"

int main(int argc, char** argv) {
  int num_threads, num_iter = 1000000;

  if (argc < 2) {
    fprintf(stderr, "Usage: ./harness [NUM_THREADS]\n");
    exit(EXIT_FAILURE);
  }
  num_threads = strtol(argv[1], NULL, 10);

  omp_set_dynamic(0);
  if (omp_get_dynamic())
    printf("Warning: dynamic adjustment of threads has been set\n");

  omp_set_num_threads(num_threads);

  gtmp_init(num_threads);

#pragma omp parallel shared(num_threads)
  {
    int thread_id = omp_get_thread_num();
    double start_time = omp_get_wtime();
    for (int i = 0; i < num_iter; i++) {
      gtmp_barrier();
    }
    double end_time = omp_get_wtime();
    printf("Thread %d: Time %f\n", thread_id, end_time - start_time);
  }

  gtmp_finalize();

  return 0;
}
