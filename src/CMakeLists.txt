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
  PMCSOGConfig.h
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

        misc/stacksafe.cpp
        misc/stacksafe.h
        misc/SafeDequeue.cpp
        misc/SafeDequeue.h
        Hybrid/HybridKripke.cpp
        Hybrid/HybridKripke.h
        Hybrid/HybridKripkeIterator.cpp
        Hybrid/HybridKripkeIterator.h
        Hybrid/HybridKripkeState.cpp
        Hybrid/HybridKripkeState.h
        SylvanWrapper.cpp
        SylvanWrapper.h
        SylvanCacheWrapper.cpp SylvanCacheWrapper.h
        MCMultiCore/ModelCheckerCPPThread.cpp MCMultiCore/ModelCheckerCPPThread.h
        MCMultiCore/ModelCheckerTh.cpp MCMultiCore/ModelCheckerTh.h
        MCMultiCore/MCThReqPor.cpp MCMultiCore/MCThReqPor.h
        MCMultiCore/MCCPPThPor.cpp MCMultiCore/MCCPPThPor.h
        threadSOG.cpp threadSOG.h
        HybridSOG.cpp HybridSOG.h
        Hybrid/MCHybrid/MCHybridSOG.cpp Hybrid/MCHybrid/MCHybridSOG.h
        MCMultiCore/ModelCheckThReq.cpp MCMultiCore/ModelCheckThReq.h
        Hybrid/MCHybridReq/MCHybridSOGReq.cpp
        Hybrid/MCHybridReq/MCHybridSOGReq.h
        Hybrid/MCHybridReqPOR/MCHybridSOGReqPOR.cpp
        Hybrid/MCHybridReqPOR/MCHybridSOGReqPOR.h
        misc/md5_hash.h Hybrid/MCHybridPOR/MCHybridSOGPOR.cpp Hybrid/MCHybridPOR/MCHybridSOGPOR.h)


target_include_directories(mc-sog PUBLIC  "${PROJECT_BINARY_DIR}/src")

target_link_libraries(mc-sog
  bddx
  spot
  RdP
  ${MPI_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)
