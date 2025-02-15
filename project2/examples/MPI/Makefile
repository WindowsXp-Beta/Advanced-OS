OMPFLAGS = -fopenmp
OMPLIBS = -lgomp

CC = gcc
CPPFLAGS = -g -Wall
LDFLAGS = -g -Wall
LDLIBS = $(OMPLIBS)

MPICC = mpicc.mpich
MPICH = /usr/lib/mpich
CFLAGS = -I $(MPICH)/include

all: hello_world communicate_world openmp_mpi

hello_world: hello_world.o
	$(MPICC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

communicate_world: communicate_world.o
	$(MPICC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

openmp_mpi: openmp_mpi.o
	$(MPICC) -o $@ $(LDFLAGS) $^ $(LDLIBS) $(OMPLIBS)

openmp_mpi.o: openmp_mpi.c
	$(MPICC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $(OMPFLAGS) $^

%.o: %.c
	$(MPICC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o hello_world communicate_world openmp_mpi
