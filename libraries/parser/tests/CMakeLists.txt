# set minimum cmake version
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# project name and language
project(pn-parser-test LANGUAGES CXX)

# testing library
add_executable(RdPtest test.cpp)
target_link_libraries(RdPtest RdP)

# Add test
add_test(
  NAME tests
  WORKING_DIRECTORY ${EXAMPLES_DIR}
  COMMAND RdPtest
)
