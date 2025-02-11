#!/bin/bash

NUM_VMS=0
MAX_VM=4

for ((idx=MAX_VM; idx>NUM_VMS; idx--)); do
  VM_NAME="aos_vm$idx"
  echo "Destroying VM: $VM_NAME"
  uvt-kvm destroy "$VM_NAME"
done
