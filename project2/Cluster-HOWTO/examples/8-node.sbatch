#!/bin/bash

#SBATCH -J cs6210-proj2-mpihello
#SBATCH -N 8 --ntasks-per-node=1
#SBATCH --mem-per-cpu=1G
#SBATCH -t 5
#SBATCH -q coc-ice
#SBATCH -o mpi_hello.out

echo "Started on `/bin/hostname`"


cd ~/examples

module load gcc/12.3.0 mvapich2/2.3.7-1
mpicc -Wall mpi_hello.c -o mpi_hello

srun mpi_hello
