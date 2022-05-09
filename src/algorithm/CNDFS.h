//
// Created by ghofrane on 5/4/22.
//

#ifndef PMC_SOG_CNDFS_H
#define PMC_SOG_CNDFS_H
#include "../ModelCheckBaseMT.h"

#include <spot/tl/apcollect.hh>
class CNDFS {

private:
    ModelCheckBaseMT *mMcl;
    shared_ptr<spot::twa_graph> mAa;
    uint16_t mNbTh;
public:
    //CNDFS(ModelCheckBaseMT *mMcl, const shared_ptr<spot::twa_graph> &mAa);
    CNDFS(ModelCheckBaseMT &mcl,shared_ptr<spot::twa_graph> af,const uint16_t& nbTh) {
        mMcl=&mcl;
        mAa=af;
        mNbTh=nbTh; // nulber of threads to be created
    }
    virtual ~CNDFS();
    //static void DfsBlue();
    void DfsBlue();
   // static void spawnThreads(int n, ModelCheckBaseMT &mcl, shared_ptr<spot::twa_graph> af);
};


#endif //PMC_SOG_CNDFS_H
