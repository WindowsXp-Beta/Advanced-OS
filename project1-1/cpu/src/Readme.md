# VCPU Scheduler

> Please write your algorithm and logic for the VCPU scheduler in this readme. Include details about how the algorithm is designed and explain how your implementation fulfills the project requirements. Use pseudocode or simple explanations where necessary to make it easy to understand.

The CPUScheduler follows the below logic in each iteration:
1. Get active domains(VMs) using `virConnectListAllDomains`.
2. Query the current cumulative CPU time of each vCPU using `virDomainGetCPUStats`. Since we assume that each VM can only have one vCPU in this project, the CPU time of each vCPU can represent the CPU time of its VM.
3. Convert each cumulative CPU time into a percentage workload using $workload = \frac{cur\_cpu\_time - prev\_cpu\_time}{interval}$
4. Get the mapping between vCPU and pCPU by using `virDomainGetVcpus` to obtain the `virVcpuInfo` struct and accessing its `cpu` field.
5. Calculate each pCPU's workload by summing workloads of all VMs that run on this pCPU.
6. Check if the workloads are balanced by checking if the standard deviation of VMs' workloads is smaller than a threshold, return True.
7. If the workloads are balanced, just return the `CPUScheduler`.
8. If the workloads are imbalanced, follow the below steps to balance it.
    1. Sort VMs in descending order based on their workloads.
    2. Maintain a `double workloads[n_host_cpus]` array that record the workload of each pCPU so far. After determining the pCPU of a VM, add the workload of that VM to the corresponding array entry.
    3. Allocate the first `n_host_cpus` largest VMs, each of which on a different pCPU.
    4. Iterate through remaining VMs, assign each of them to the pCPU with the smallest workload so far.