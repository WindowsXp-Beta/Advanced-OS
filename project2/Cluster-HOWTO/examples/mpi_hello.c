#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "gtmpi.h"

int main(int argc, char** argv) {
  int num_processes;
  int num_rounds = 10000;

  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

  gtmpi_init(num_processes);

  double start_time = MPI_Wtime();
  for (int k = 0; k < num_rounds; k++) {
    gtmpi_barrier();
  }
  double end_time = MPI_Wtime();
  printf("Rk %d: Time %f\n", (end_time - start_time) / num_rounds);

  gtmpi_finalize();

  MPI_Finalize();

  return 0;
}
