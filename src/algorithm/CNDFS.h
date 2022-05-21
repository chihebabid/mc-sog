//
// Created by ghofrane on 5/4/22.
//

#ifndef PMC_SOG_CNDFS_H
#define PMC_SOG_CNDFS_H
#include "../ModelCheckBaseMT.h"
//#include "../SogKripkeTh.h"
#include <spot/tl/apcollect.hh>
#include <cstdint>
#include <thread>
class CNDFS {

private:
    static constexpr uint8_t MAX_THREADS=64;
    ModelCheckBaseMT * mMcl;
    spot::twa_graph_ptr mAa;
    uint16_t mNbTh;
    atomic<uint8_t> mIdThread;
    static void threadHandler(void *context);
    std::thread* mlThread[MAX_THREADS];
    std::mutex mMutex;
    void spawnThreads();
public:
    CNDFS(ModelCheckBaseMT *mcl,const spot::twa_graph_ptr &af,const uint16_t& nbTh);
//    CNDFS(shared_ptr<SogKripkeTh> k,const spot::twa_graph_ptr &af,const uint16_t& nbTh);
    virtual ~CNDFS();
    //static void DfsBlue();
    void computeProduct();
    static spot::bdd_dict_ptr* m_dict_ptr;
};


#endif //PMC_SOG_CNDFS_H
