import subprocess


def submit_mp_hello_jobs():
    """Submit SLURM jobs for the first task (mp_hello)"""
    for it in range(10):
        for cpus in range(2, 9, 2):  # Iterate from 2 to 8 in steps of 2
            output_file = f"mp2_{cpus}_{it}.out"
            wrap_command = f"./mp_hello {cpus}"

            # fmt: off
            command = [
                "sbatch",
                "-J", "cs6210-proj2-mphello",
                "-N", "1",
                f"--cpus-per-task={cpus}",
                "--mem-per-cpu=1G",
                "-t", "5",
                "-q", "coc-ice",
                "-o", output_file,
                f"--wrap={wrap_command}"
            ]
            # fmt: on

            try:
                result = subprocess.run(command, check=True, capture_output=True, text=True)
                print(f"SLURM job submitted successfully with {cpus} CPUs!")
                print("Output:", result.stdout)
            except subprocess.CalledProcessError as e:
                print(f"Error submitting SLURM job with {cpus} CPUs!")
                print("Error message:", e.stderr)


def submit_combined_hello_jobs():
    """Submit SLURM jobs for the second task (combined_hello)"""
    for it in range(10):
        for N in range(2, 9, 2):  # Iterate N from 2 to 8
            for cpus in range(2, 13, 2):  # Iterate CPUs from 2 to 12 in steps of 2
                output_file = f"combined_hello_{N}_{cpus}_{it}.out"
                wrap_command = f"srun combined_hello {cpus}"

                # fmt: off
                command = [
                    "sbatch",
                    "-J", "cs6210-proj2-combinedhello",
                    "-N", str(N),
                    "--ntasks-per-node=1",
                    f"--cpus-per-task={cpus}",
                    "--mem-per-cpu=1G",
                    "-t", "5",
                    "-q", "coc-ice",
                    "-o", output_file,
                    f"--wrap={wrap_command}"
                ]
                # fmt: on

                try:
                    result = subprocess.run(
                        command, check=True, capture_output=True, text=True
                    )
                    print(f"SLURM job submitted successfully with N={N} and CPUs={cpus}!")
                    print("Output:", result.stdout)
                except subprocess.CalledProcessError as e:
                    print(f"Error submitting SLURM job with N={N} and CPUs={cpus}!")
                    print("Error message:", e.stderr)


def submit_mpi_hello_jobs():
    """Submit SLURM jobs for the third task (mpi_hello)"""
    for it in range(10):
        for N in range(2, 13, 2):  # Iterate N from 2 to 12
            output_file = f"mpi_hello_{N}_{it}.out"
            wrap_command = "srun mpi_hello"

            # fmt: off
            command = [
                "sbatch",
                "-J", "cs6210-proj2-mpihello",
                "-N", str(N),
                "--ntasks-per-node=1",
                "--mem-per-cpu=1G",
                "-t", "5",
                "-q", "coc-ice",
                "-o", output_file,
                f"--wrap={wrap_command}"
            ]
            # fmt: on

            try:
                result = subprocess.run(command, check=True, capture_output=True, text=True)
                print(f"SLURM job submitted successfully with N={N}.")
                print("Output:", result.stdout)
            except subprocess.CalledProcessError as e:
                print(f"Error submitting SLURM job with N={N}.")
                print("Error message:", e.stderr)

def submit_mpi_comp_jobs():
    """Submit SLURM jobs for the third task (mpi_hello)"""
    for it in range(10):
        for N in range(2, 9, 2):  # Iterate N from 2 to 12
            for t_local in range(2, 13, 2):
                output_file = f"mpi2_comp_{N}_{t_local}_{it}.out"
                wrap_command = "srun mpi_hello"

                # fmt: off
                command = [
                    "sbatch",
                    "-J", "cs6210-proj2-mpicomp",
                    "-N", str(N),
                    f"--ntasks-per-node={t_local}",
                    "--mem-per-cpu=1G",
                    "-t", "5",
                    "-q", "coc-ice",
                    "-o", output_file,
                    f"--wrap={wrap_command}"
                ]
                # fmt: on

                try:
                    result = subprocess.run(command, check=True, capture_output=True, text=True)
                    print(f"SLURM job submitted successfully with N={N}.")
                    print("Output:", result.stdout)
                except subprocess.CalledProcessError as e:
                    print(f"Error submitting SLURM job with N={N}.")
                    print("Error message:", e.stderr)

if __name__ == "__main__":
    # submit_mp_hello_jobs()
    submit_combined_hello_jobs()
    # submit_mpi_hello_jobs()
    # submit_mpi_comp_jobs()
