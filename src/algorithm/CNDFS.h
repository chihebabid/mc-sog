//
// Created by ghofrane on 5/4/22.
//

#ifndef PMC_SOG_CNDFS_H
#define PMC_SOG_CNDFS_H
#include "ModelCheckBaseMT.h"
#include <spot/tl/apcollect.hh>
class CNDFS {
public:
    virtual ~CNDFS();
    static void DfsBlue(ModelCheckBaseMT &mcl, shared_ptr<spot::twa_graph> af);
    static void spawnThreads(int n, ModelCheckBaseMT &mcl, shared_ptr<spot::twa_graph> af);
};


#endif //PMC_SOG_CNDFS_H
