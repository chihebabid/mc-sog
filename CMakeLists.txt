# set minimum cmake version
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# project name and language
project(pmc-sog C CXX)

# compiler flags
if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
endif()

# add pn parser
set(PARSER_DIR "${CMAKE_SOURCE_DIR}/libraries/parser")
message(STATUS "Building Petri Net parser ...")
include_directories("${PARSER_DIR}/src")
add_subdirectory(${PARSER_DIR})

# add sylvan
message(STATUS "Building Sylvan ...")
set(SYLVAN_DIR "${CMAKE_SOURCE_DIR}/third-party/sylvan")
include_directories(${SYLVAN_DIR})
add_subdirectory(${SYLVAN_DIR})

# build spot if it is not installed on the system
find_library(SPOT_LIBRARY "spot")
find_library(BDDX_LIBRARY "bddx")

if(NOT SPOT_LIBRARY)
  include(ExternalProject)

  message(STATUS "Building Spot ...")
  set(SPOT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third-party/spot")
  set(SPOT_DIR "${CMAKE_BINARY_DIR}/third-party/spot")

  ExternalProject_Add(SpotLibrary
    PREFIX            ${SPOT_DIR}
    SOURCE_DIR        ${SPOT_SOURCE_DIR}
    CONFIGURE_COMMAND autoreconf -if ${SPOT_SOURCE_DIR} && ${SPOT_SOURCE_DIR}/configure --disable-python --prefix=<INSTALL_DIR>
    BUILD_COMMAND     make -j
  )

  include_directories("${SPOT_DIR}/include")
else ()
  message(STATUS "Found spot: ${SPOT_LIBRARY}")
  message(STATUS "Found bddx: ${BDDX_LIBRARY}")
endif()

# add source folder
include_directories(src)
add_subdirectory(src)

# add tests
# enable_testing()
# add_subdirectory(tests)

