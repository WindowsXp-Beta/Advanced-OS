#include "combined.h"

#include <math.h>
#include <mpi.h>
#include <omp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int num_rounds;
int num_processes;
int num_threads;
volatile int count;
volatile bool sense;

void combined_init(int _num_processes, int _num_threads) {
  num_rounds = ceil(log2(_num_processes));
  num_processes = _num_processes;
  count = _num_threads;
  num_threads = _num_threads;
  sense = false;
}

void combined_barrier() {
  // called on a single process
  // check if all threads have reached the barrier
  // if so, send signal to other processes; if no, wait
  bool my_sense = !sense;
  if (__sync_fetch_and_sub(&count, 1) == 1) {  // last thread reaches

    // send signal to other processes
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    for (int round = 0; round < num_rounds; round++) {
      int send_to = (rank + (1 << round)) % num_processes;
      int recv_from = (rank - (1 << round) + num_processes) % num_processes;

      // printf("Rank %d sending to %d and receiving from %d\n", rank, send_to,
      //        recv_from);

      MPI_Request request;
      MPI_Isend(NULL, 0, MPI_INT, send_to, 1, MPI_COMM_WORLD, &request);

      MPI_Recv(NULL, 0, MPI_INT, recv_from, 1, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
    }
    count = num_threads;
    sense = my_sense;
  } else {
    while (sense != my_sense);
  }
}

void combined_finalize() {}
