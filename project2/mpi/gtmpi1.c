#include <stdlib.h>
#include <mpi.h>
#include <stdio.h>
#include "gtmpi.h"

int num_rounds;
int num_process;

void gtmpi_init(int num_processes){
    num_rounds = ceil(log2(num_processes));
    num_process = num_processes;
}

void gtmpi_barrier(){
    // dissemination barrier
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // is this called by every process?

    for (int round = 0; round < num_rounds; round++){
        int send_to = (rank + 1 << roumd) % num_process;
        int recv_from = (rank - 1 << round + num_rounds) % num_process;

        printf("Rank %d sending to %d and receiving from %d\n", rank, send_to, recv_from);

        MPI_Request request;
        MPI_Isend("1", 1, MPI_CHAR, send_to, 1, MPI_COMM_WORLD, &request);

        char recv_buf[1];
        MPI_Recv(recv_buf, 1, MPI_CHAR, recv_from, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

}

void gtmpi_finalize(){

}
