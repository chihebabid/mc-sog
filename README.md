# PMC-SOG

(P)arallel (M)odel (C)hecking using the (S)ymbolic (O)bservation (G)raph

A [Symbolic Observation Graph](https://www.researchgate.net/profile/Kais_Klai/publication/48445044_Design_and_Evaluation_of_a_Symbolic_and_Abstraction-Based_Model_Checker/links/00463514319a181966000000.pdf) software tool has been implemented in C/C++.
The user can choose between sequential or parallel construction.

The [BuDDy](http://buddy.sourceforge.net/manual/main.html) BDD package and [Sylvan](https://trolando.github.io/sylvan/) package are exploited in order to represent aggregates compactly.

BuDDy used for sequential version and Sylvan for both sequential and parallel construction.



It is necessary to install [Spot](https://spot.lrde.epita.fr/install.html).

## Description
This repository hosts the experiments and results for the Hybrid (MPI/pthreads) approach for the SOG construction and provides a short guide on how to install the tools and reproduce the results.

  ** MPI : The Message Passing Interface ([OpenMPI](https://www.open-mpi.org/) implementation). The goal of the MPI is to establish a portable, efficient, and flexible standard for message passing between processess (nodes).

  ** Pthread : POSIX thread librarie.  Mutexes (Pthread lock functionality) are used to prevent data inconsistencies due to race conditions.




## Building


- `git clone --recursive https://depot.lipn.univ-paris13.fr/PMC-SOG/mc-sog.git`

- `mkdir build`

- `cd build`

- `cmake ..`

- `cmake --build .`


## Testing
SOG building from an LTL formula (philo3.net example)

```
Multi-threading execution

mpirun -n 1 hybrid-sog arg1 arg 2 arg3 arg4
arg1: specifies method of creating threads or/and specifies if modelchecking should be performed on the fly. It can be set with one of the following values:
     * p : using pthread library
     * pc : using pthread library and applying canonization on nodes
     * l : using lace framework
     * lc : using lace framework and applying canonization on nodes
     * otfL : perform modelchecking on the fly using laceframework
     * otfP : perform modelchecking on the fly using pthread
arg2: specifies the number of threads/workers to be created
arg3: specifies the net to build its SOG
arg4: specifies the LTL formula file

Distributed execution
mpirun -n arg0 hybrid-sog arg1 arg 2 arg3 arg4
arg0 : specifies the number of processes must be >1
arg1: specifies whether modelchecking should be performed on the fly. It can be set with one of the following values:
     * otf : Construction performing Model checking on the fly
     * otherwise : construction without Model checking
arg2: specifies the number of threads/workers to be created
arg3: specifies the net to build its SOG
arg4: specifies the LTL formula file

```
