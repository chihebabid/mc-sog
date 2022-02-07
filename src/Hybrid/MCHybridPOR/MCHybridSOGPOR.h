//
// Created by chiheb on 06/02/2022.
//

#ifndef PMC_SOG_MCHYBRIDSOGPOR_H
#define PMC_SOG_MCHYBRIDSOGPOR_H
#include <mpi.h>
#include <openssl/md5.h>
#include "misc/md5_hash.h"
#include "NewNet.h"
#include "LDDGraph.h"
#include "../MCHybrid/MCHybridSOG.h"

class MCHybridSOGPOR : public MCHybridSOG {
public:
    MCHybridSOGPOR(const NewNet &R, MPI_Comm &comm_world, bool init = false);
    ~MCHybridSOGPOR() override;
    void computeDSOG(LDDGraph &g) override;
    static void *threadHandlerPOR(void *context);
    void *doCompute() override;
};


#endif //PMC_SOG_MCHYBRIDSOGPOR_H
