# Use Ubuntu 22.04 as the base image
FROM ubuntu:22.04

# Set non-interactive mode for apt-get to avoid prompts
ENV DEBIAN_FRONTEND=noninteractive

# Update and install necessary packages
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        git \
        vim \
        python3 \
        python3-pip \
        python-is-python3 \
        cmake \
        clang \
        tmux \
        valgrind \
        zip \
        unzip \
        gdb \
        mpich \
	libomp-dev && \
    rm -rf /var/lib/apt/lists/*

# Set the working directory in the container
WORKDIR /home

# Default command: start an interactive shell
CMD ["/bin/bash"]