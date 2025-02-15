# Cluster-HOWTO

## Cluster Access
You will run jobs on the COC-ICE PACE cluster. All enrolled students must be able to access the cluster via ssh and use GT credentials. You **must** be on VPN or campus network for this to work. You can login to PACE using the following command (replace username):

```bash
ssh gburdell3@login-ice.pace.gatech.edu
```

## Managing source files

Use `scp` or `rsync` to copy files from local to the pace cluster. The [`examples/`](./examples) folder contains sample mpi and omp files along with SLURM scripts so you may follow along the HOWTO doc. Command to copy source files to PACE: 
```bash
scp -r examples gburdell3@login-ice.pace.gatech.edu:~/
```

## Execution

To run jobs on the COC-ICE PACE cluster, you will need SLURM scripts. As mentioned before, these have been provided under the `examples/` folder which we copied via `scp/rsync`

A brief description of the files within examples are as follows:
- 8-node.sbatch: SLURM config file for running an OpenMPI job across 8 nodes
- 8-thread.sbatch: SLURM config file for running an OpenMP job across 8 threads
- 2-node-8-thread.sbatch: SLURM config file for running a combined OpenMP/MPI job across 16 threads across 2 nodes (8 on each)
- mpi_hello.c: A Hello, world! OpenMPI implementation you can use to test SLRUM configs for OpenMPI 
- mp_hello.c: A Hello, world! OpenMP implementation you can use to test SLRUM configs for OpenMP
- combined_hello.c: A Hello, world! combined OpenMP/MPI implementation you can use to test SLRUM configs for combined OpenMP/MPI programs

### Run And Monitor
A sample workflow which schedules the binary built from `combined_hello.c` on the PACE cluster is as follows: 

To schedule SLURM scripts:
```bash
cd examples
sbatch 2-node-8-thread.sbatch
```
To monitor your spot on queue:
```bash
squeue
```
### SLURM config: details

As an example, line-by-line descriptions of the [`2-node-8-thread.sbatch`](./examples/2-node-8-thread.sbatch) is as follows:

```bash
#!/bin/bash                           #<-- mandatory hashbang for SLURM scripts
#SBATCH -J cs6210-proj2-combinedhello  #<-- name of the job
#SBATCH -N 2 --ntasks-per-node=1 --cpus-per-task=8     #<-- -N is the node count; --ntasks-per-node is assigned tasks/processes per node; --cpus-per-task is the number of cpus per task/process
#SBATCH --mem-per-cpu=1G              #<-- memory per core (default should suffice for all)
#SBATCH -t 5                           #<-- how long is the job expected to run (5 minutes)
#SBATCH -q coc-ice                    #<-- SLURM QoS queue. Default suffices.
#SBATCH -o combined_hello.out          #<-- console output of your program is piped to this file

echo "Started on `/bin/hostname`"


cd ~/examples

module load gcc/12.3.0 mvapich2/2.3.7-1    #<-- load gcc (w/ omp) & mpi environment
export OMP_NUM_THREADS=8              #<-- specify num threads for OMP to use

# we MUST compile in the cluster environment before execution
mpicc -Wall -lm -fopenmp -lgomp combined_hello.c -o combined_hello 

# use SLURM to run the binary
srun combined_hello
```



