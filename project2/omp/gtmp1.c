#include <omp.h>
#include <stdbool.h>

#include "gtmp.h"

int count;
bool sense;
int num_threads;

void gtmp_init(int _num_threads) {
  count = _num_threads;
  num_threads = _num_threads;
  sense = false;
}

void gtmp_barrier() {
  bool my_sense = !sense;
  if (__sync_fetch_and_sub(&count, 1) == 1) {  // last thread to reach this barrier
    count = num_threads;
    sense = my_sense;
  } else {  // wait for other threads
    while (sense != my_sense);
  }
}

void gtmp_finalize() {}
