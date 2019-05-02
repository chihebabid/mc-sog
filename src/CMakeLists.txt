# set minimum cmake version
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# include MPI
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

# include openssl
find_package(OpenSSL)
include_directories(${OPENSSL_INCLUDE_DIR})

include_directories("${CMAKE_SOURCE_DIR}/third-party/sylvan/src")


add_library(spot SHARED IMPORTED) # or STATIC instead of SHARED
set_target_properties(spot PROPERTIES
  IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libspot.so"
  INTERFACE_INCLUDE_DIRECTORIES "/usr/include/spot/"
)

add_library(bddx SHARED IMPORTED) # or STATIC instead of SHARED
set_target_properties(bddx PROPERTIES
  IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libbddx.so"
  INTERFACE_INCLUDE_DIRECTORIES "/usr/include/spot/"
)





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
  )

target_link_libraries(hybrid-sog
  bddx
  spot
  sylvan
  RdP


  ${MPI_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)
