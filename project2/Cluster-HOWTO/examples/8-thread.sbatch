#!/bin/bash

#SBATCH -J cs6210-proj2-mphello
#SBATCH -N 1 --cpus-per-task=8
#SBATCH --mem-per-cpu=1G
#SBATCH -t 5
#SBATCH -q coc-ice
#SBATCH -o mp1_8.out

echo "Started on `/bin/hostname`"


module load gcc/12.3.0 mvapich2/2.3.7-1

gcc -fopenmp mp_hello.c ../../omp/gtmp1.c -o mp_hello -I../../omp

./mp_hello 8
