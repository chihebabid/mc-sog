# set minimum cmake version
cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

# include MPI
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

# include openssl
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# include spot and bddx library
add_library(spot SHARED IMPORTED)
add_library(bddx SHARED IMPORTED)

if(NOT SPOT_LIBRARY)
  set_target_properties(spot PROPERTIES IMPORTED_LOCATION "${SPOT_DIR}/lib/libspot.so")
  set_target_properties(bddx PROPERTIES IMPORTED_LOCATION "${SPOT_DIR}/lib/libbddx.so")
else()
  set_target_properties(spot PROPERTIES IMPORTED_LOCATION ${BDDX_LIBRARY})
  set_target_properties(bddx PROPERTIES IMPORTED_LOCATION ${SPOT_LIBRARY})
endif()

# Hybrid SOG executable
add_executable(mc-sog main.cpp
  NewNet.cpp
  NewNet.h
  CommonSOG.cpp
  CommonSOG.h
  LDDGraph.cpp
  LDDGraph.h
  LDDState.cpp
  LDDState.h
  TransSylvan.cpp

  SpotSogState.cpp
  SpotSogState.h
  SpotSogIterator.cpp
  SpotSogIterator.h
  SogTwa.cpp
  SogTwa.h
  SogKripkeState.cpp
  SogKripkeState.h
  SogKripkeIterator.cpp
  SogKripkeIterator.h
  SogKripke.cpp
  SogKripke.h
  ModelCheckBaseMT.cpp
  ModelCheckBaseMT.h

  SogKripkeTh.cpp
  SogKripkeTh.h
  SogKripkeStateTh.cpp
  SogKripkeStateTh.h
  SogKripkeIteratorTh.cpp
  SogKripkeIteratorTh.h

  stacksafe.cpp
  stacksafe.h
  SafeDequeue.cpp
  SafeDequeue.h
  HybridKripke.cpp
  HybridKripke.h
  HybridKripkeIterator.cpp
  HybridKripkeIterator.h
  HybridKripkeState.cpp
  HybridKripkeState.h
        SylvanWrapper.cpp
        SylvanWrapper.h
        SylvanCacheWrapper.cpp SylvanCacheWrapper.h
        ModelCheckerCPPThread.cpp ModelCheckerCPPThread.h
        ModelCheckerTh.cpp ModelCheckerTh.h
        threadSOG.cpp threadSOG.h
        HybridSOG.cpp HybridSOG.h
        MCHybridSOG.cpp MCHybridSOG.h)

target_link_libraries(mc-sog
  bddx
  spot
  RdP
  ${MPI_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)
