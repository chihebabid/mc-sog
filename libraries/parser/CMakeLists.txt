# set minimum cmake version
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# project name and language
project(pn-parser LANGUAGES CXX)

# variables
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(EXAMPLES_DIR "${PROJECT_SOURCE_DIR}/examples")

# add source folder
include_directories (${SRC_DIR})
add_subdirectory(src)

# add tests
enable_testing()
add_subdirectory(tests)
