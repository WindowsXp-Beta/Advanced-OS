#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>
#include "combined.h"
#include <time.h>

int main(int argc, char** argv)
{
    int num_processes, num_threads, num_iter = 10;
    struct timespec start, end;

    MPI_Init(&argc, &argv);

    if (argc < 2){
        fprintf(stderr, "Usage: ./combined [NUM_THREADS]\n");
        exit(EXIT_FAILURE);
    }

    // get num process and num threads
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    num_threads = strtol(argv[1], NULL, 10);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    omp_set_dynamic(0);
    if (omp_get_dynamic())
        printf("Warning: dynamic adjustment of threads has been set\n");

    omp_set_num_threads(num_threads);

    combined_init(num_processes, num_threads);

    clock_gettime(CLOCK_MONOTONIC, &start);

    #pragma omp parallel shared(num_threads)
    {
        int i;
        for(i = 0; i < num_iter; i++){
            combined_barrier();
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    float elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    float start_time = start.tv_sec + start.tv_nsec / 1e9;
    float end_time = end.tv_sec + end.tv_nsec / 1e9;

    combined_finalize();

    MPI_Finalize();
    
    printf("%d rank: %d processes and %d threads: start time is %f, end time is %f\n", rank, num_processes, num_threads, start_time, end_time);

    return 0;
}
