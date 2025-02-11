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
#define LL unsigned long long
#define DEBUG

#ifdef DEBUG
#define debug(fmt, ...) printf("%s:%d: " fmt, __FILE__, __LINE__, __VA_ARGS__);
#else
#define debug(fmt, ...)
#endif

int is_exit = 0;  // DO NOT MODIFY THE VARIABLE
int unused_mem_low = 100;
int unused_mem_high = 300;
int mem_alloc_unit = 100;
int host_mem_low = 200;

void MemoryScheduler(virConnectPtr conn, int interval);

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
void signal_callback_handler() {
  printf("Caught Signal");
  is_exit = 1;
}

void error(const char *str, int if_exit) {
  fprintf(stderr, "%s\n", str);
  if (if_exit) exit(1);
  return;
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

  signal(SIGINT, signal_callback_handler);

  while (!is_exit) {
    // Calls the MemoryScheduler function after every 'interval' seconds
    MemoryScheduler(conn, interval);
    sleep(interval);
  }

  // Close the connection
  virConnectClose(conn);
  return 0;
}

/*
COMPLETE THE IMPLEMENTATION
*/

int get_active_vm(virConnectPtr conn, virDomainPtr **domains) {
  return virConnectListAllDomains(
      conn, domains,
      VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_RUNNING);
}

void get_mem_stats(LL *unused_mems, LL *available_mems, LL *balloon_mems,
                   int n_vms, virDomainPtr *domains) {
  virDomainMemoryStatStruct stats[VIR_DOMAIN_MEMORY_STAT_NR];
  for (int i = 0; i < n_vms; i++) {
    int stats_len =
        virDomainMemoryStats(domains[i], stats, VIR_DOMAIN_MEMORY_STAT_NR, 0);
    if (stats_len < 0) {
      error("Failed to query memory stats", 1);
    }

    for (int j = 0; j < stats_len; j++) {
      virDomainMemoryStatStruct stat = stats[j];
      if (stat.tag == VIR_DOMAIN_MEMORY_STAT_UNUSED) {
        unused_mems[i] = stat.val / 1024;
      } else if (stat.tag == VIR_DOMAIN_MEMORY_STAT_AVAILABLE) {
        available_mems[i] = stat.val / 1024;
      } else if (stat.tag == VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON) {
        balloon_mems[i] = stat.val / 1024;
      }
    }
    debug(
        "Mem Stats for VM %s is unused: %llu MB, available: %llu MB, balloon: "
        "%llu MB\n",
        virDomainGetName(domains[i]), unused_mems[i], available_mems[i],
        balloon_mems[i]);
  }
}

LL get_host_mem(virConnectPtr conn) {
  LL ret = virNodeGetFreeMemory(conn) / 1024 / 1024;
  if (ret == 0) {
    error("Failed to query host free memory", 1);
  }
  return ret;
}

void MemoryScheduler(virConnectPtr conn, int interval) {
  virDomainPtr *domains;
  int n_vms = get_active_vm(conn, &domains);
  LL *unused_mems = calloc(n_vms, sizeof(unsigned long long));
  LL *available_mems = calloc(n_vms, sizeof(unsigned long long));
  LL *balloon_mems = calloc(n_vms, sizeof(unsigned long long));

  get_mem_stats(unused_mems, available_mems, balloon_mems, n_vms, domains);
  LL host_mem_avail = get_host_mem(conn);
  debug("Host free mem: %llu\n", host_mem_avail);

  int vm_status[n_vms];  // 0: have extra memory, 1: lack memory, 2: just enough
                         // memory
  int vm_lack_mem_cnt = 0;
  LL mem_needed = 0, mem_avail = 0;
  for (int i = 0; i < n_vms; i++) {
    if (unused_mems[i] < unused_mem_low) {
      LL max_mem = virDomainGetMaxMemory(domains[i]) / 1024;
      debug("Max mem for VM %s is %llu\n", virDomainGetName(domains[i]),
            max_mem);
      if (max_mem < balloon_mems[i] + mem_alloc_unit) {
        debug("VM %d reaches its max memory\n", i);
        vm_status[i] = 2;
        continue;
      }
      vm_status[i] = 1;
      mem_needed += mem_alloc_unit;
      vm_lack_mem_cnt++;
    } else if (unused_mems[i] > unused_mem_high) {
      vm_status[i] = 0;
      mem_avail += mem_alloc_unit;
    } else {
      vm_status[i] = 2;
    }
  }
  debug("Mem needed %llu, mem available %llu\n", mem_needed, mem_avail);

  if (mem_needed > mem_avail) {
    if (host_mem_avail - (mem_needed - mem_avail) >= host_mem_low) {
      mem_avail = mem_needed;
    }
  }

  debug("[UPDATED] Mem needed %llu, mem available %llu\n", mem_needed,
        mem_avail);
  if (mem_avail >= mem_needed) {
    for (int i = 0; i < n_vms; i++) {
      if (vm_status[i] == 0) {
        virDomainSetMemory(domains[i],
                           (balloon_mems[i] - mem_alloc_unit) * 1024);
      } else if (vm_status[i] == 1) {
        int ret = virDomainSetMemory(domains[i],
                                     (balloon_mems[i] + mem_alloc_unit) * 1024);
        debug("Inflate for VM %s: %llu, ret: %d\n",
              virDomainGetName(domains[i]), balloon_mems[i] + mem_alloc_unit,
              ret);
      }
    }
  } else if (vm_lack_mem_cnt > 0) {
    for (int i = 0; i < n_vms; i++) {
      if (vm_status[i] == 0) {
        virDomainSetMemory(domains[i],
                           (balloon_mems[i] - mem_alloc_unit) * 1024);
      } else if (vm_status[i] == 1) {
        if (mem_avail > 0) {
          virDomainSetMemory(domains[i],
                             (balloon_mems[i] + mem_alloc_unit) * 1024);
          mem_avail -= mem_alloc_unit;
        }
      }
    }
  }

  free(unused_mems);
  free(available_mems);
  free(balloon_mems);
  for (int i = 0; i < n_vms; i++) {
    virDomainFree(domains[i]);
  }
  free(domains);
  printf("\nFinished One Iteration\n");
}