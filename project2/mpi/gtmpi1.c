#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "gtmpi.h"

int num_rounds;
int num_process;

void gtmpi_init(int num_processes) {
  num_rounds = ceil(log2(num_processes));
  num_process = num_processes;
}

void gtmpi_barrier() {
  // dissemination barrier
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // is this called by every process?

  for (int round = 0; round < num_rounds; round++) {
    int send_to = (rank + (1 << round)) % num_process;
    int recv_from = (rank - (1 << round) + num_process) % num_process;

    // printf("Rank %d sending to %d and receiving from %d\n", rank, send_to,
    //        recv_from);

    MPI_Request request;
    MPI_Isend(NULL, 0, MPI_INT, send_to, 1, MPI_COMM_WORLD, &request);

    MPI_Recv(NULL, 0, MPI_INT, recv_from, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }
}

void gtmpi_finalize() {}
