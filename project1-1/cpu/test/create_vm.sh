#!/bin/bash

NUM_VMS=8

# Loop to create the VMs
for i in $(seq 1 $NUM_VMS); do
    VM_NAME="aos_vm$i"
    echo "Creating VM: $VM_NAME"
    uvt-kvm create $VM_NAME release=bionic --memory 512
done