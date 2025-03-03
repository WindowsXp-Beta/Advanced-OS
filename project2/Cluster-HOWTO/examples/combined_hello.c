#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "combined.h"

int main(int argc, char** argv) {
  int num_iter = 10;
  int num_processes, num_threads;

  MPI_Init(&argc, &argv);

  if (argc < 2) {
    fprintf(stderr, "Usage: ./combined [NUM_THREADS]\n");
    exit(EXIT_FAILURE);
  }

  // get num process and num threads
  MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
  num_threads = strtol(argv[1], NULL, 10);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    printf("Test setting: procs %d, threads %d and iters %d\n", num_processes,
           num_threads, num_iter);
  }

  omp_set_dynamic(0);
  if (omp_get_dynamic())
    printf("Warning: dynamic adjustment of threads has been set\n");

  omp_set_num_threads(num_threads);

  combined_init(num_processes, num_threads);

  double start_time = MPI_Wtime();
#pragma omp parallel shared(num_threads)
  {
    int i;
    for (i = 0; i < num_iter; i++) {
      combined_barrier();
    }
    printf("rank %d thread %d: finished\n", rank, omp_get_thread_num());
  }
  double end_time = MPI_Wtime();
  printf("Rk %d: Time %f\n", (end_time - start_time) / num_iter);

  combined_finalize();

  MPI_Finalize();

  return 0;
}