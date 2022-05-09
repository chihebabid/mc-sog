//
// Created by ghofrane on 5/4/22.
//

#ifndef PMC_SOG_CNDFS_H
#define PMC_SOG_CNDFS_H
#include "ModelCheckBaseMT.h"
#include "SogKripkeTh.h"
#include <spot/tl/apcollect.hh>
class CNDFS {

private:
    static ModelCheckBaseMT *mMcl;
    shared_ptr<spot::twa_graph> mAa;
public:
    //CNDFS(ModelCheckBaseMT *mMcl, const shared_ptr<spot::twa_graph> &mAa);

    virtual ~CNDFS();
    //static void DfsBlue();
    static void DfsBlue(ModelCheckBaseMT &mcl, shared_ptr<spot::twa_graph> af);
   // static void spawnThreads(int n, ModelCheckBaseMT &mcl, shared_ptr<spot::twa_graph> af);
};


#endif //PMC_SOG_CNDFS_H
