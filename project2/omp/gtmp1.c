#include <omp.h>
#include <stdbool.h>

#include "gtmp.h"

int count;
bool sense;

void gtmp_init(int num_threads) {
  count = num_threads;
  sense = false;
}

void gtmp_barrier() {
  bool my_sense = !sense;
  if (__sync_fetch_and_sub(&count, 1) ==
      1) {  // last thread to reach this barrier
    count = omp_get_num_threads();
    sense = my_sense;
  } else {  // wait for other threads
    while (sense != my_sense);
  }
}

void gtmp_finalize() {}
