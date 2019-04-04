cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(sylvan C CXX)

set(PROJECT_DESCRIPTION "Sylvan, a parallel decision diagram library")
set(PROJECT_URL "https://github.com/trolando/sylvan")

include_directories(.)

add_library(sylvan
    avl.h
    lace.h
    lace.c
    sha2.h
    sha2.c
    sylvan.h
    sylvan_bdd.h
    sylvan_bdd.c
    sylvan_cache.h
    sylvan_cache.c
    sylvan_config.h
    sylvan_common.h
    sylvan_common.c
    sylvan_gmp.h
    sylvan_gmp.c
    sylvan_int.h
    sylvan_ldd.h
    sylvan_ldd.c
    sylvan_mt.h
    sylvan_mt.c
    sylvan_mtbdd.h
    sylvan_mtbdd.c
    sylvan_mtbdd_int.h
    sylvan_obj.hpp
    sylvan_obj.cpp
    sylvan_refs.h
    sylvan_refs.c
    sylvan_seq.h
    sylvan_seq.c
    sylvan_sl.h
    sylvan_sl.c
    sylvan_table.h
    sylvan_table.c
    sylvan_stats.h
    sylvan_stats.c
    tls.h
)

target_link_libraries(sylvan m pthread)

if(UNIX AND NOT APPLE)
    target_link_libraries(sylvan rt)
endif()
