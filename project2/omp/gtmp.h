#ifndef GTMP_H
#define GTMP_H
#include <stdint.h>

typedef struct {
  volatile int8_t* parent_ptr;  // points to the CN array in the parent node
  volatie int* wakeup_ptrs[2];
  int sense;
  volatile int parent_sense;
  union {
    volatile int value;
    volatile int8_t array[4];
  } hc;  // have_children
  union {
    volatile int value;
    volatile int8_t array[4];
  } cn;  // children_not_ready
} mp_mcs_barrier_node;

typedef struct {
  int num_threads;
  mp_mcs_barrier_node* nodes;
} mp_mcs_barrier;

void gtmp_init(int num_threads);
void gtmp_barrier();
void gtmp_finalize();
#endif
