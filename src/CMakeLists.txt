# set minimum cmake version
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# include MPI
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

# include openssl
find_package(OpenSSL)
include_directories(${OPENSSL_INCLUDE_DIR})

include_directories("${CMAKE_SOURCE_DIR}/third-party/sylvan/src")

# Hybrid SOG executable
add_executable(hybrid-sog main.cpp
  DistributedSOG.cpp
  DistributedSOG.h
  HybridSOG.cpp
  HybridSOG.h
  threadSOG.cpp
  threadSOG.h
  LDDGraph.cpp
  LDDState.cpp
  TransSylvan.cpp
  sylvan_sog.c
  )

target_link_libraries(hybrid-sog
  sylvan
  bdd
  RdP
  ${MPI_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)
