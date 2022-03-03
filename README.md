# PMC-SOG

(P)arallel (M)odel (C)hecking using the (S)ymbolic (O)bservation (G)raph.

A [Symbolic Observation Graph](https://www.researchgate.net/profile/Kais_Klai/publication/48445044_Design_and_Evaluation_of_a_Symbolic_and_Abstraction-Based_Model_Checker/links/00463514319a181966000000.pdf)
(SOG) software tool has been implemented in C/C++. The user can choose between
sequential or parallel construction.

## Description

This repository hosts the code source for the Hybrid (MPI/pthreads) approach for
the SOG construction and its use for model-checking.

* **MPI:** The Message Passing Interface ([OpenMPI](https://www.open-mpi.org/)
implementation). The goal of the MPI is to establish a portable, efficient, and
flexible standard for message passing between processess (nodes).

* **Pthread:** POSIX thread library.  Mutexes (Pthread lock functionality) are
used to prevent data inconsistencies due to race conditions.

## Dependencies

The following dependencies are necessary to build the tool.

- [Bison](https://www.gnu.org/software/bison/)
- [Cmake](https://cmake.org/)
- [Flex](https://github.com/westes/flex)
- [OpenMPI](https://www.open-mpi.org/) development libraries
- [OpenSSL](https://www.openssl.org/) development libraries
- [Spot](https://spot.lrde.epita.fr/install.html) library

## Build

The steps to build the tool are the following:

- `git clone --recursive https://depot.lipn.univ-paris13.fr/PMC-SOG/mc-sog.git`

- `cd mc-sog`

- `cmake -S . -B build`

- `cmake --build build --target install`

The compiled binary `pmc-sog` can be found in the folder `assets`.

## CLI

```
PMC-SOG : Parallel Model Checking tool based on Symbolic Observation Graphs
Usage: ./assets/pmc-sog [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  --n Integer:POSITIVE        Number of threads to be created (default: 1)
  --net Path:FILE REQUIRED    Petri net file
  --ltl Path:FILE REQUIRED    LTL property file
  --thread TEXT:{posix,c++}   Thread library (default: c++)
  --por,--no-por{false}       Apply partial order reductions (default: false)


Explicit MC:
  --explicit Excludes: --progressive --emptiness-check
                              Run explicit model-checking
  --lace,--no-lace{false} Needs: --explicit
                              Use the work-stealing framework Lace (default: false)
  --canonization,--no-canonization{false} Needs: --explicit
                              Apply canonization to the SOG (default: false)
  --only-sog Needs: --explicit
                              Only builds the SOG


On-the-fly MC:
  --progressive,--no-progressive{false} Excludes: --explicit
                              Use a progressive construction of the SOG (default: false)
  --emptiness-check TEXT Excludes: --explicit
                              Spot emptiness-check algorithm


Print:
  --dot-sog Needs: --explicit Save the SOG in a dot file
  --dot-formula               Save the automata of the negated formula in a dot file
  --counter-example Needs: --explicit
                              Save the counter example in a dot file
```

### Emptiness-Check algorithms
The emptiness-check algorithms are provided by spot. For more information,
please see https://spot.lrde.epita.fr/doxygen/group__emptiness__check.html.
Here, some of examples:

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

## Examples

### Multi-Core Model-Checking

In the following example, the model `train12.net` is verified using the
multi-core on-the-fly model-checking (progressive) with 4 posix threads and no
partial order reduction (POR). The emptiness-check algorithm used is the
Couvreur's algorithm.

```
mpirun -n 1 ./pmc-sog \
     --n 4 \
     --net build/hybrid/models/train/train12.net \
     --ltl build/hybrid/formulas/train/train12/train12-1.ltl.reduced \
     --thread posix \
     --no-por \
     --progressive \
     --emptiness-check "Cou99(poprem)"
```

In the following example, the model `train12.net` is verified using the explicit
multi-core model-checking with 4 posix threads and no partial order reduction
(POR). Moreover, the SOG is canonized and the lace framework is used. Finally,
the tool generates the `.dot` files of the SOG, the automaton of negated
formula, and a counter-example if the property is violated.

```
mpirun -n 1 ./pmc-sog \
     --n 4 \
     --net build/hybrid/models/train/train12.net \
     --ltl build/hybrid/formulas/train/train12/train12-1.ltl.reduced \
     --thread posix \
     --no-por \
     --explicit \
     --lace \
     --canonization \
     --dot-sog \
     --dot-formula \
     --counter-example
```

### Hybrid Model-Checking

**Note:** To run the hybrid version, the minimum number of MPI processes `n` is 2.

In the following example, the model `train12.net` is verified using the hybrid
on-the-fly model-checking (progressive) with 2 processes, each process with 4
C++ threads and no partial order reduction (POR). The emptiness-check algorithm
used is the Couvreur's algorithm.


```
mpirun -n 2 ./pmc-sog \
     --n 4 \
     --net build/hybrid/models/train/train12.net \
     --ltl build/hybrid/formulas/train/train12/train12-1.ltl.reduced \
     --no-por \
     --progressive \
     --emptiness-check "Cou99(poprem)"
```


## Publications

1. [A Parallel Construction of the Symbolic Observation Graph: the Basis for Efficient Model Checking of Concurrent Systems 2017](https://www.researchgate.net/publication/315840512_A_Parallel_Construction_of_the_Symbolic_Observation_Graph_the_Basis_for_Efficient_Model_Checking_of_Concurrent_Systems)

2. [Parallel Symbolic Observation Graph 2017](https://ieeexplore.ieee.org/document/8367348)

3. [Reducing Time and/or Memory Consumption of the SOG Construction in a Parallel Context](https://ieeexplore.ieee.org/abstract/document/8672359)

4. [Towards Parallel Verification of Concurrent Systems using the Symbolic Observation Graph](https://ieeexplore.ieee.org/abstract/document/8843636)
