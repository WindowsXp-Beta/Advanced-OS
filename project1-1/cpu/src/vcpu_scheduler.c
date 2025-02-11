#include <float.h>
#include <libvirt/libvirt.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define MIN(a, b) ((a) < (b) ? a : b)
#define MAX(a, b) ((a) > (b) ? a : b)

#define DEBUG

#ifdef DEBUG
#define debug(fmt, ...) printf("%s:%d: " fmt, __FILE__, __LINE__, __VA_ARGS__);
#else
#define debug(fmt, ...)
#endif

const int MAX_VM = 32;
unsigned long long *cpu_times = NULL;
const int BALANCE_THRESHOLD = 5;

int is_exit = 0;  // DO NOT MODIFY THIS VARIABLE
int n_host_cpus = 0;
int n_vms = 0;
int cpu_map_len = 0;

typedef struct {
  double cpu_usage;
  int vm_id;
} VM_CPU_Usage;

void error(const char *str, int if_exit) {
  fprintf(stderr, "%s\n", str);
  if (if_exit) exit(1);
  return;
}

void CPUScheduler(virConnectPtr conn, int interval);

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
void signal_callback_handler() {
  printf("Caught Signal");
  is_exit = 1;
}

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
int main(int argc, char *argv[]) {
  virConnectPtr conn;

  if (argc != 2) {
    printf("Incorrect number of arguments\n");
    return 0;
  }

  // Gets the interval passes as a command line argument and sets it as the
  // STATS_PERIOD for collection of balloon memory statistics of the domains
  int interval = atoi(argv[1]);

  conn = virConnectOpen("qemu:///system");
  if (conn == NULL) {
    fprintf(stderr, "Failed to open connection\n");
    return 1;
  }

  // Get the total number of pCpus in the host
  signal(SIGINT, signal_callback_handler);

  virNodeInfo node_info;

  if (virNodeGetInfo(conn, &node_info) < 0) {
    error("Failed to get node info", 1);
  }
  n_host_cpus = VIR_NODEINFO_MAXCPUS(node_info);
  cpu_map_len = VIR_CPU_MAPLEN(n_host_cpus);
  cpu_times = calloc(MAX_VM, sizeof(long long));
  // printf("%d\t%d\n%s\n", n_host_cpus, cpu_map_len,
  //        virConnectGetCapabilities(conn));

  while (!is_exit)
  // Run the CpuScheduler function that checks the CPU Usage and sets the pin at
  // an interval of "interval" seconds
  {
    CPUScheduler(conn, interval);
    sleep(interval);
  }

  // Closing the connection
  virConnectClose(conn);
  free(cpu_times);
  return 0;
}

/* COMPLETE THE IMPLEMENTATION */
int get_active_vm(virConnectPtr conn, virDomainPtr **domains) {
  return virConnectListAllDomains(
      conn, domains,
      VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_RUNNING);
}

void get_vm_cpu_times(unsigned long long current_cpu_times[],
                      virDomainPtr *domains, int interval) {
  for (int i = 0; i < n_vms; i++) {
    virTypedParameterPtr params;
    int n_params = virDomainGetCPUStats(domains[i], NULL, 0, -1, 1, 0);
    if (n_params < 0) {
      error("Get n_params failed", 1);
    }
    params = calloc(n_params, sizeof(virTypedParameter));
    if (virDomainGetCPUStats(domains[i], params, n_params, -1, 1, 0) < 0) {
      error("Get CPU Stats failed", 1);
    }
    unsigned long long cpu_time = 0;
    virTypedParamsGetULLong(params, n_params, VIR_DOMAIN_CPU_STATS_CPUTIME,
                            &cpu_time);
    // TODO: figure out how to use cpu_times
    current_cpu_times[i] = cpu_time;
  }
}

void calculate_vm_cpu_usages(VM_CPU_Usage usages[],
                             unsigned long long current_cpu_times[],
                             int interval) {
  for (int i = 0; i < n_vms; i++) {
    printf("VM %d prev time %llu, cur time %llu, diff %llu\n", i, cpu_times[i],
           current_cpu_times[i], current_cpu_times[i] - cpu_times[i]);
    usages[i].cpu_usage = 100 * (current_cpu_times[i] - cpu_times[i]) /
                          (double)(interval * 1000000000);
    usages[i].vm_id = i;
  }
}

void get_cpu_mapping(int mapping[], virDomainPtr *domains) {
  for (int i = 0; i < n_vms; i++) {
    virVcpuInfoPtr cpu_info = (virVcpuInfoPtr)malloc(sizeof(virVcpuInfo));
    unsigned char cpumaps[cpu_map_len];
    memset(cpumaps, 0, cpu_map_len * sizeof(char));
    if (virDomainGetVcpus(domains[i], cpu_info, 1, cpumaps, cpu_map_len) < 0) {
      error("Failed to get Vcpus", 1);
    }
    mapping[i] = cpu_info->cpu;
    // TODO: figure which p cpu to use
    // Check if cpu_info matches cpumaps
    int p_cpu = cpu_info->cpu;
    // printf("%d maps to %d\n", i, p_cpu);
    for (int k = 0; k < cpu_map_len; k++) {
      if (p_cpu < 8) {
        for (int j = 0; j < 8; j++) {
          if (((cpumaps[k] >> j) & 1) != (j == p_cpu)) {
            debug("p cpu mismatch for VM %d with cpumap %d: %x\n", i, k,
                  cpumaps[k]);
          }
        }
      } else {
        if (cpumaps[k] != 0) {
          debug("p cpu mismatch for VM %d with cpumap %d: %x\n", i, k,
                cpumaps[k]);
        }
        p_cpu -= 8;
      }
    }
    free(cpu_info);
  }
}

void calculate_pcpu_workload(double workloads[], VM_CPU_Usage usages[],
                             int mapping[]) {
  for (int i = 0; i < n_vms; i++) {
    workloads[mapping[usages[i].vm_id]] += usages[i].cpu_usage;
  }
}

int cmp_vm_usage(const void *vm1, const void *vm2) {
  double vm1_usage = ((VM_CPU_Usage *)vm1)->cpu_usage,
         vm2_usage = ((VM_CPU_Usage *)vm2)->cpu_usage;
  if (vm1_usage > vm2_usage) {
    return -1;
  } else if (vm1_usage == vm2_usage) {
    return 0;
  } else {
    return 1;
  }
}

double calculate_std_dev(double workloads[]) {
  double sum = 0.0;
  for (int i = 0; i < n_host_cpus; i++) {
    sum += workloads[i];
  }
  double mean = sum / n_host_cpus;

  double sum_squared_diff = 0.0;

  for (int i = 0; i < n_host_cpus; i++) {
    double diff = workloads[i] - mean;
    sum_squared_diff += diff * diff;
  }

  double variance = sum_squared_diff / n_host_cpus;
  return sqrt(variance);
}

int check_balance(double workloads[], int mapping[]) {
  double max_wl = DBL_MIN, min_wl = DBL_MAX;
  int max_pcpu_id = 0;

  for (int i = 0; i < n_host_cpus; i++) {
    double wl = workloads[i];
    if (wl > max_wl) {
      max_wl = wl;
      max_pcpu_id = i;
    }
    if (wl < min_wl) {
      min_wl = wl;
    }
  }
  if ((max_wl - min_wl) <= BALANCE_THRESHOLD) {
    return 1;
  } else {
    // check if the max_cpu only has one VM
    int cnt = 0;
    for (int i = 0; i < n_vms; i++) {
      if (mapping[i] == max_pcpu_id) {
        cnt++;
      }
    }
    return cnt == 1;
  }
}

void pin_vm_to_pcpu(virDomainPtr domain, int p_cpu) {
  unsigned char cpumaps[cpu_map_len];
  memset(cpumaps, 0, cpu_map_len * sizeof(char));
  cpumaps[p_cpu / 8] = (1 << (p_cpu % 8));
  if (virDomainPinVcpu(domain, 0, cpumaps, cpu_map_len) < 0) {
    error("Failed to pin vpu", 1);
  }
}

void CPUScheduler(virConnectPtr conn, int interval) {
  virDomainPtr *domains;
  n_vms = get_active_vm(conn, &domains);

  unsigned long long current_cpu_times[n_vms];
  memset(current_cpu_times, 0, n_vms * sizeof(long long));
  get_vm_cpu_times(current_cpu_times, domains, interval);

  // If cpu_times hasn't been initialized, we populate the array and return
  if (cpu_times[0] == 0) {
    for (int i = 0; i < n_vms; i++) {
      cpu_times[i] = current_cpu_times[i];
    }
    return;
  }

  VM_CPU_Usage usages[n_vms];
  calculate_vm_cpu_usages(usages, current_cpu_times, interval);
  for (int i = 0; i < n_vms; i++) {
    debug("VM %d spends %.2f\n", i, usages[i].cpu_usage);
  }

  for (int i = 0; i < n_vms; i++) {
    cpu_times[i] = current_cpu_times[i];
  }

  int mapping[n_vms];  // vm -> p_cpu
  memset(mapping, 0, n_vms * sizeof(int));
  get_cpu_mapping(mapping, domains);
  for (int i = 0; i < n_vms; i++) {
    debug("VM %d runs on p_cpu %d\n", i, mapping[i]);
  }

  double workloads[n_host_cpus];
  memset(workloads, 0, n_host_cpus * sizeof(double));
  calculate_pcpu_workload(workloads, usages, mapping);
  for (int i = 0; i < n_host_cpus; i++) {
    debug("p cpu %d wl is %.2f\n", i, workloads[i]);
  }
  printf("std dev is %f\n", calculate_std_dev(workloads));

  if (calculate_std_dev(workloads) > BALANCE_THRESHOLD) {
    // if (!check_balance(workloads, mapping)) {
    printf("Unbalance Workload\n");

    qsort(usages, n_vms, sizeof(VM_CPU_Usage), cmp_vm_usage);

    int new_mapping[n_host_cpus];  // p_cpu -> vm
    memset(new_mapping, 0, n_host_cpus * sizeof(int));

    if (n_vms <= n_host_cpus) {  // We just assign one VM per physical CPU
      for (int i = 0; i < n_vms; i++) {
        if (new_mapping[mapping[i]] == 0) {
          new_mapping[mapping[i]] = i;
        } else {
          for (int j = 0; j < n_host_cpus; j++) {
            if (new_mapping[j] == 0) {
              new_mapping[j] = i;
              pin_vm_to_pcpu(domains[i], j);
            }
          }
        }
      }
    } else {
      for (int i = 0; i < n_host_cpus; i++) {
        workloads[i] = 0.0;
      }
      for (int i = 0; i < n_host_cpus; i++) {
        double cpu_usage = usages[i].cpu_usage;
        int vm_id = usages[i].vm_id;
        if (new_mapping[mapping[vm_id]] == 0) {
          new_mapping[mapping[vm_id]] = vm_id;
          workloads[mapping[vm_id]] = cpu_usage;
          pin_vm_to_pcpu(
              domains[vm_id],
              mapping[vm_id]);  // We still need this function otherwise the VM
                                // will shift to other pcpus.
        } else {
          for (int j = 0; j < n_host_cpus; j++) {
            if (new_mapping[j] == 0) {
              new_mapping[j] = vm_id;
              workloads[j] = cpu_usage;
              pin_vm_to_pcpu(domains[vm_id], j);
              debug("Move VM %d to p_cpu %d\n", vm_id, j);
              break;
            }
          }
        }
      }
      printf("After allocating n_host_cpus VMs\n");
      for (int i = 0; i < n_host_cpus; i++) {
        debug("p cpu %d workload is %f\n", i, workloads[i]);
      }

      for (int i = n_host_cpus; i < n_vms; i++) {
        double min_wl = DBL_MAX;
        int min_wl_p_cpu = 0;
        for (int j = 0; j < n_host_cpus; j++) {
          if (workloads[j] < min_wl) {
            min_wl_p_cpu = j;
            min_wl = workloads[j];
          }
        }
        double cpu_usage = usages[i].cpu_usage;
        int vm_id = usages[i].vm_id;
        pin_vm_to_pcpu(domains[vm_id], min_wl_p_cpu);
        debug("Move VM %d to p_cpu %d\n", vm_id, min_wl_p_cpu);
        workloads[min_wl_p_cpu] += cpu_usage;
      }
    }
  }

  // do some cleanup
  for (int i = 0; i < n_vms; i++) {
    virDomainFree(domains[i]);
  }
  free(domains);
  printf("\nFinished One Iteration\n");
}
