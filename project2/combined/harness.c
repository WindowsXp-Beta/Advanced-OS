#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>
#include "combined.h"

int main(int argc, char** argv)
{
    int num_processes, num_threads, num_iter = 1;

    MPI_Init(&argc, &argv);

    if (argc < 2){
        fprintf(stderr, "Usage: ./combined [NUM_THREADS]\n");
        exit(EXIT_FAILURE);
    }

    // get num process and num threads
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    num_threads = strtol(argv[1], NULL, 10);

    omp_set_dynamic(0);
    if (omp_get_dynamic())
        printf("Warning: dynamic adjustment of threads has been set\n");

    omp_set_num_threads(num_threads);

    combined_init(num_processes, num_threads);

    #pragma omp parallel shared(num_threads)
    {
        int i;
        for(i = 0; i < num_iter; i++){
            combined_barrier();
        }
    }

    combined_finalize();

    MPI_Finalize();

    return 0;
}
