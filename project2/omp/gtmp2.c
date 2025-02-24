#include <omp.h>
#include <stdio.h>

#include "gtmp.h"

mp_mcs_barrier barrier;

void gtmp_init(int num_threads) {
  barrier.num_threads = num_threads;
  int ret = posix_memalign(&(barrier.nodes), 64,
                           sizeof(mp_mcs_barrier) * num_threads);

  if (ret != 0) {
    fprintf(stderr, "%s:%s Failed to alloc mcs_nodes for mp_mcs_barrier %d\n",
            __FILE__, __LINE__);
    exit(ret)
  }

  for (int i = 0; i < num_threads; i++) {
    mp_mcs_barrier_node* node = &(barrier.nodes[i]);
    node->sense = 1;
    node->parent_sense = 0;
    node->hc.value = 0;
    node->cn.value = 0;

    // initialize wait tree
    for (int j = i * 4 + 1, k = 0; j < num_threads && k < 4; j++, k++) {
      node->hc.array[k] = 1;
      node->cn.array[k] = 1;
    }
    if (i != 0) {
      node->parent_ptr = &(barrier.nodes[(i - 4) / 4].cn.array[(i - 4) % 4]);
    } else {
      node->parent_ptr = NULL;
    }

    // initialize wakeup tree
    int k = 0;
    for (int j = i * 2 + 1; j < num_threads && k < 2; j++, k++) {
      node->wakeup_ptrs[k] = &(barrier.nodes[j].parent_sense);
    }
    for (; k < 2; k++) {
      node->wakeup_ptrs[k] = NULL;
    }
  }
}

void gtmp_barrier() {
  int id = omp_get_thread_num();
  mp_mcs_barrier_node* node = &(barrier.nodes[id]);
  while (node->cn.value);
  node->cn.value = node->hc.value;

  if (node->parent_ptr) {
    *(node->parent_ptr) = 0;
    while (node->parent_sense != node->sense);
  }

  for (int j = 0; j < 2; j++) {
    if (node->wakeup_ptrs[j]) {
      *(node->wakeup_ptrs[j]) = node->sense;
    }
  }
  node->sense = !node->sense;
}

void gtmp_finalize() { free(barrier.nodes); }
