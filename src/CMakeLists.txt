# set minimum cmake version
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

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
add_executable(hybrid-sog main.cpp
  NewNet.cpp
  NewNet.h
  CommonSOG.cpp
  CommonSOG.h
  DistributedSOG.cpp
  DistributedSOG.h
  HybridSOG.cpp
  HybridSOG.h
  threadSOG.cpp
  threadSOG.h
  LDDGraph.cpp
  LDDGraph.h
  LDDState.cpp
  LDDState.h
  TransSylvan.cpp
  sylvan_sog.c
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
  ModelCheckLace.cpp
  ModelCheckLace.h
  SogKripkeTh.cpp
  SogKripkeTh.h
  SogKripkeStateTh.cpp
  SogKripkeStateTh.h
  SogKripkeIteratorTh.cpp
  SogKripkeIteratorTh.h
  ModelCheckerTh.cpp
  ModelCheckerTh.h
  MCHybridSOG.cpp
  MCHybridSOG.h
  HybridKripke.cpp
  HybridKripke.h
  HybridKripkeIterator.cpp
  HybridKripkeIterator.h
  HybridKripkeState.cpp
  HybridKripkeState.h
  )

target_link_libraries(hybrid-sog
  bddx
  spot
  sylvan
  RdP
  ${MPI_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)
