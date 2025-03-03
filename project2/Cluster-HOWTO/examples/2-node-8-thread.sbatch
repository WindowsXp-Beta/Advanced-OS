#!/bin/bash

#SBATCH -J cs6210-proj2-combinedhello
#SBATCH -N 2 --ntasks-per-node=1 --cpus-per-task=8
#SBATCH --mem-per-cpu=1G
#SBATCH -t 5
#SBATCH -q coc-ice
#SBATCH -o combined_hello.out

echo "Started on `/bin/hostname`"


module load gcc/12.3.0 mvapich2/2.3.7-1

mpicc -Wall -lm -fopenmp -lgomp combined_hello.c ../../combined/combined.c -o combined_hello -I../../combined

srun combined_hello 8
