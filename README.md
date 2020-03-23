# PMC-SOG

(P)arallel (M)odel (C)hecking using the (S)ymbolic (O)bservation (G)raph.

A [Symbolic Observation Graph](https://www.researchgate.net/profile/Kais_Klai/publication/48445044_Design_and_Evaluation_of_a_Symbolic_and_Abstraction-Based_Model_Checker/links/00463514319a181966000000.pdf) software tool has been implemented in C/C++.
The user can choose between sequential or parallel construction.

The [BuDDy](http://buddy.sourceforge.net/manual/main.html) BDD package and [Sylvan](https://trolando.github.io/sylvan/) package are exploited in order to represent aggregates compactly.

BuDDy used for sequential version and Sylvan for both sequential and parallel construction.

It is necessary to install [Spot](https://spot.lrde.epita.fr/install.html).

## Description

This repository hosts the experiments and results for the Hybrid (MPI/pthreads) approach for the SOG construction and provides a short guide on how to install the tools and reproduce the results.

* **MPI:** The Message Passing Interface ([OpenMPI](https://www.open-mpi.org/) implementation). The goal of the MPI is to establish a portable, efficient, and flexible standard for message passing between processess (nodes).

* **Pthread:** POSIX thread library.  Mutexes (Pthread lock functionality) are used to prevent data inconsistencies due to race conditions.

## Dependencies

- [Cmake](https://cmake.org/)
- [Bison](https://www.gnu.org/software/bison/)
- [Flex](https://github.com/westes/flex)
- [OpenMPI](https://www.open-mpi.org/) development libraries
- [OpenSSL](https://www.openssl.org/) development libraries
- [GMP](https://gmplib.org/) development libraries

## Building

- `git clone --recursive https://depot.lipn.univ-paris13.fr/PMC-SOG/mc-sog.git`

- `cd mc-sog && mkdir build && cd build`

- `cmake ..`

- `cmake --build .`


## Testing
SOG building from an LTL formula (philo3.net example)

```
Multi-threading execution

mpirun -n 1 hybrid-sog arg1 arg2 arg3 arg4 [arg5]
arg1: specifies method of creating threads or/and specifies if modelchecking should be performed on the fly. It can be set with one of the following values:
     * p : using pthread library
     * pc : using pthread library and applying canonization on nodes
     * l : using lace framework
     * lc : using lace framework and applying canonization on nodes
     * otfL : perform modelchecking on the fly using laceframework
     * otfP : perform modelchecking on the fly using pthread
arg2: specify the number of threads/workers to be created
arg3: specify the net to build its SOG
arg4: specify the LTL formula file
arg5: (Optional) select an algorithm for the emptiness check provided by spot, see https://spot.lrde.epita.fr/doxygen/group__emptiness__check.html 
     * Cou99 
	+ shy
	+ poprem
	+ group 
     * GC04
     * CVWY90
	+ bsh : set the size of a hash-table
     * SE05
	+ bsh : set the size of a hash-table
     * Tau03
     * Tau03_opt
      

Distributed execution
mpirun -n arg0 hybrid-sog arg1 arg 2 arg3 arg4
arg0 : specifies the number of processes must be >1
arg1: specifies whether modelchecking should be performed on the fly. It can be set with one of the following values:
     * otf : Construction performing Model checking on the fly
     * otherwise : construction without Model checking
arg2: specifies the number of threads/workers to be created
arg3: specifies the net to build its SOG
arg4: specifies the LTL formula file
arg5: (Optional) select an algorithm for the emptiness check provided by spot, see https://spot.lrde.epita.fr/doxygen/group__emptiness__check>
     * Cou99 
        + shy
        + poprem
        + group
     * GC04
     * CVWY90
        + bsh : set the size of a hash-table
     * SE05
        + bsh : set the size of a hash-table
     * Tau03
     * Tau03_opt

```
## Publications
1. [A Parallel Construction of the Symbolic Observation Graph: the Basis for Efficient Model Checking of Concurrent Systems 2017](https://www.researchgate.net/publication/315840512_A_Parallel_Construction_of_the_Symbolic_Observation_Graph_the_Basis_for_Efficient_Model_Checking_of_Concurrent_Systems)

2. [Parallel Symbolic Observation Graph 2017](https://ieeexplore.ieee.org/document/8367348)

3. [Reducing Time and/or Memory Consumption of the SOG Construction in a Parallel Context](https://ieeexplore.ieee.org/abstract/document/8672359)

4. [Towards Parallel Verification of Concurrent Systems using the Symbolic Observation Graph](https://ieeexplore.ieee.org/abstract/document/8843636)



