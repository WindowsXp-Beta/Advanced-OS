#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "gtmpi.h"

int rank, num_processes;

void gtmpi_init(int _num_processes) {
  num_processes = _num_processes;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
}

void gtmpi_barrier() {
  int first_child = rank * 4 + 1;
  int num_children = num_processes - first_child;
  if (num_children < 0) {
    num_children = 0;
  } else if (num_children > 4) {
    num_children = 4;
  }

  int finished_children = 0;
  MPI_Status status;
  while (finished_children < num_children) {
    MPI_Recv(NULL, 0, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    assert(status.MPI_SOURCE >= first_child &&
           status.MPI_SOURCE <= first_child + num_children);
    finished_children++;
  }

  MPI_Request req;
  if (rank != 0) {
    MPI_Isend(NULL, 0, MPI_INT, (rank - 1) / 4, 0, MPI_COMM_WORLD, &req);

    MPI_Recv(NULL, 0, MPI_INT, (rank - 1) / 2, 0, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
  }

  for (int i = rank * 2 + 1, k = 0; i < num_processes && k < 2; i++, k++) {
    MPI_Isend(NULL, 0, MPI_INT, i, 0, MPI_COMM_WORLD, &req);
  }
}

void gtmpi_finalize() {}
