# PMC-SOG

(P)arallel (M)odel (C)hecking using the (S)ymbolic (O)bservation (G)raph.

A [Symbolic Observation Graph](https://www.researchgate.net/profile/Kais_Klai/publication/48445044_Design_and_Evaluation_of_a_Symbolic_and_Abstraction-Based_Model_Checker/links/00463514319a181966000000.pdf)
(SOG) software tool has been implemented in C/C++. The user can choose between
sequential or parallel construction.

## Description

This repository hosts the code source for the Hybrid (MPI/pthreads) approach for
the SOG construction and its use for model-checking.

- **MPI:** The Message Passing Interface ([OpenMPI](https://www.open-mpi.org/)
  implementation). The goal of the MPI is to establish a portable, efficient, and
  flexible standard for message passing between processess (nodes).

- **Pthread:** POSIX thread library. Mutexes (Pthread lock functionality) are
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
                              Use the work-stealing framework Lace. Available only in multi-core (default: false)
  --canonization,--no-canonization{false} Needs: --explicit
                              Apply canonization to the SOG. Available only in multi-core (default: false)
  --only-sog Needs: --explicit
                              Only builds the SOG. Available only in multi-core


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

- Cou99
  - shy
  - poprem
  - group
- GC04
- CVWY90
  - bsh : set the size of a hash-table
- SE05
  - bsh : set the size of a hash-table
- Tau03
- Tau03_opt

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

- K. Klai, C. A. Abid, J. Arias, and S. Evangelista, "[Hybrid Parallel Model Checking of Hybrid LTL on Hybrid State Space Representation](https://link.springer.com/chapter/10.1007/978-3-030-98850-0_3)", in 14th International Conference on Verification and Evaluation of Computer and Communication Systems, VECoS 2021, Beijing, China, November 22-23, 2021, 2022, vol. 13187, pp. 27–42. doi: 10.1007/978-3-030-98850-0_3.


- C. A. Abid, K. Klai, J. Arias, and H. Ouni, “[SOG-Based Multi-Core LTL Model Checking](https://ieeexplore.ieee.org/document/9443795),” in IEEE International Conference on Parallel & Distributed Processing with Applications, Big Data & Cloud Computing, Sustainable Computing & Communications, Social Computing & Networking, ISPA/BDCloud/SocialCom/SustainCom 2020, Exeter, United Kingdom, December 17-19, 2020, 2020, pp. 9–17. doi: 10.1109/ISPA-BDCloud-SocialCom-SustainCom51426.2020.00028.

- H. Ouni, K. Klai, C. A. Abid, and B. Zouari, “[Towards Parallel Verification of Concurrent Systems using the Symbolic Observation Graph](https://ieeexplore.ieee.org/document/8843636),” in 19th International Conference on Application of Concurrency to System Design, ACSD 2019, Aachen, Germany, June 23-28, 2019, 2019, pp. 23–32. doi: 10.1109/ACSD.2019.00007.

- H. Ouni, K. Klai, and B. Zouari, “[Parallel construction of the Symbolic Observation Graph](https://ieeexplore.ieee.org/document/9188053),” in 17th International Conference on High Performance Computing & Simulation, HPCS 2019, Dublin, Ireland, July 15-19, 2019, 2019, pp. 1011–1013. doi: 10.1109/HPCS48598.2019.9188053.

- H. Ouni, K. Klai, C. A. Abid, and B. Zouari, “[Reducing Time and/or Memory Consumption of the SOG Construction in a Parallel Context](https://ieeexplore.ieee.org/document/8672359),” in IEEE International Conference on Parallel & Distributed Processing with Applications, Ubiquitous Computing & Communications, Big Data & Cloud Computing, Social Computing & Networking, Sustainable Computing & Communications, ISPA/IUCC/BDCloud/SocialCom/SustainCom 2018, Melbourne, Australia, December 11-13, 2018, 2018, pp. 147–154. doi: 10.1109/BDCloud.2018.00034.

- H. Ouni, K. Klai, C. A. Abid, and B. Zouari, “[A Parallel Construction of the Symbolic Observation Graph: the Basis for Efficient Model Checking of Concurrent Systems](https://easychair.org/publications/open/9pR),” in SCSS 2017, The 8th International Symposium on Symbolic Computation in Software Science 2017, April 6-9, 2017, Gammarth, Tunisia, 2017, vol. 45, pp. 107–119. doi: 10.29007/7b44.

- H. Ouni, K. Klai, C. A. Abid, and B. Zouari, “[Parallel Symbolic Observation Graph](https://ieeexplore.ieee.org/document/8367348),” in 2017 IEEE International Symposium on Parallel and Distributed Processing with Applications and 2017 IEEE International Conference on Ubiquitous Computing and Communications (ISPA/IUCC), Guangzhou, China, December 12-15, 2017, 2017, pp. 770–777. doi: 10.1109/ISPA/IUCC.2017.00118.
