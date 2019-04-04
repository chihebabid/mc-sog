# PMC-SOG

(P)arallel (M)odel (C)hecking using the (S)ymbolic (O)bservation (G)raph

A [Symbolic Observation Graph](https://www.researchgate.net/profile/Kais_Klai/publication/48445044_Design_and_Evaluation_of_a_Symbolic_and_Abstraction-Based_Model_Checker/links/00463514319a181966000000.pdf) software tool has been implemented in C/C++.
The user can choose between sequential or parallel construction.

The [BuDDy](http://buddy.sourceforge.net/manual/main.html) BDD package and [Sylvan](https://trolando.github.io/sylvan/) package are exploited in order to represent aggregates compactly.

BuDDy used for sequential version and Sylvan for both sequential and parallel construction.

# Hybrid-SOG
## Description
This repository hosts the experiments and results for the Hybrid (MPI/pthreads) approach for the SOG construction and provides a short guide on how to install the tools and reproduce the results.

  ** MPI : The Message Passing Interface ([OpenMPI](https://www.open-mpi.org/) implementation). The goal of the MPI is to establish a portable, efficient, and flexible standard for message passing between processess (nodes).

  ** Pthread : POSIX thread librarie.  Mutexes (Pthread lock functionality) are used to prevent data inconsistencies due to race conditions.



All experiments were performed on a [Magi cluster](http://www.univ-paris13.fr/calcul/wiki/) of Paris 13 university. Some results are reported in [Resutls directory](https://depot.lipn.univ-paris13.fr/PMC-SOG/hybrid-sog/blob/master/Results/Experiments-Hybrid.pdf).


## Building


- `git clone --recursive https://depot.lipn.univ-paris13.fr/PMC-SOG/hybrid-sog.git`

- `mkdir build`

- `cd build`

- `cmake ..`

- `cmake --build .`


## Testing
Interactive submission of the job  (philo5 example) using 2 processes * 12 threads:

```
mpirun -n 2 ./hybrid-sog arg1 arg 2 arg3 arg4
arg1: specifies method of creating threads. It can be set with one of the following values:
     * p : using pthread library
     * pc : using pthread library and applying canonization on nodes
     * l : using lace framework
     * lc : using lace framework and applying canonization on nodes
arg2: specifies the number of threads/workers to be created
arg3: specifies the net to build its SOG
arg4: specifies the set of observable transitions


mpirun -n 2 ./hybrid-sog pc 12 philo5.net Obs5_philo
```

A sample job submission script for our home cluster using 2 nodes * 12 cores (for the same example in batch mode) :

```
#!/bin/bash
#SBATCH --job-name=philo5
#SBATCH --output=philo5.out
#SBATCH --error=philo5.err
#SBATCH --ntasks=2
#SBATCH --cpus-per-task=12
srun ./hybrid-sog pc 12 philo5.net Obs5_philo 32
```
