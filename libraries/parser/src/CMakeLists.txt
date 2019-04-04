# set minimum cmake version
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# compilator flags
project(pn-parser-src LANGUAGES CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall")

# dependencies
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# execute bison
BISON_TARGET(
  RdPMonteurParser
  RdPMonteur.y ${CMAKE_CURRENT_BINARY_DIR}/RdPMonteur.tab.cpp
)

# execute flex
FLEX_TARGET(
  RdPMonteurLexer
  RdPMonteur.l ${CMAKE_CURRENT_BINARY_DIR}/RdPMonteur.lex.cpp
)
ADD_FLEX_BISON_DEPENDENCY(RdPMonteurLexer RdPMonteurParser)

# generate a the library from sources
add_library(RdP STATIC
  ${BISON_RdPMonteurParser_OUTPUTS}
  ${FLEX_RdPMonteurLexer_OUTPUTS}
  Net.hpp
  Net.cpp
  RdPMonteur.hpp
)
