# set minimum cmake version
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# project name and language
project(hybrid-sog C CXX)

# compiler flags
if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
endif()

# add pn parser
set(PARSER_DIR "${CMAKE_SOURCE_DIR}/libraries/parser")
message(STATUS "Building Petri Net parser ...")
include_directories("${PARSER_DIR}/src")
add_subdirectory(${PARSER_DIR})

# add buddy
set(BUDDY_DIR "${CMAKE_SOURCE_DIR}/third-party/buddy")
message(STATUS "Building BuDDy library ...")
include_directories("${BUDDY_DIR}/src")
add_subdirectory(third-party/buddy)

# add sylvan
message(STATUS "Building Sylvan ...")
set(SYLVAN_DIR "${CMAKE_SOURCE_DIR}/third-party/sylvan")
include_directories(${SYLVAN_DIR})
add_subdirectory(${SYLVAN_DIR})

# add source folder
include_directories(src)
add_subdirectory(src)

# add tests
# enable_testing()
# add_subdirectory(tests)
