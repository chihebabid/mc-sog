//
// Created by ghofrane on 7/21/22.
//

#ifndef PMC_SOG_CNDFSV2_H
#define PMC_SOG_CNDFSV2_H

#include "../ModelCheckBaseMT.h"
#include "CNDFS.h"
#include <spot/tl/apcollect.hh>
#include <cstdint>
#include <spot/twa/twagraph.hh>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "misc/SafeDequeue.h"
#include <spot/tl/contain.hh>
#include <random>
#include <algorithm>

using namespace std;
typedef pair<struct myState_t *, int> coupleSuccessor;

using namespace std;


class CndfsV2 {

private:
    ModelCheckBaseMT *mMcl;
    spot::twa_graph_ptr mAa;
    uint16_t mNbTh;
    atomic<uint8_t> mIdThread;
    std::thread *mlThread[MAX_THREADS];
    mutex mMutex,mMutexStatus;
    std::condition_variable mDataCondWait;
    condition_variable cv;
    myState_t *mInitStatePtr;
    vector<myState_t *> mlBuiltStates;
    SafeDequeue<struct myState_t *> mSharedPoolTemp;
    static spot::bdd_dict_ptr *m_dict_ptr;
    spot::language_containment_checker c;
    std::random_device rd;

    void getInitialState();

    void spawnThreads();

    static void threadHandler(void *context);

    void threadRun();

//    void computeSuccessors(myState_t *state, vector<spot::formula> ap_sog);
    vector<spot::formula> fireable(myState_t *state, vector<spot::formula> ap_sog, vector <spot::formula> transition);

    myState_t *buildState(myState_t *state, spot::formula tr);
    myState_t* isStateBuilt(LDDState *sogState,const spot::twa_graph_state *spotState);
public:
    CndfsV2(ModelCheckBaseMT *mcl, const spot::twa_graph_ptr &af, const uint16_t &nbTh);

    virtual ~CndfsV2();

    void dfsBlue(myState_t *state, vector<myState_t *> &Rp, uint8_t idThread,vector<spot::formula> ap_sog, vector <spot::formula> transition);

    void dfsRed(myState_t *state, vector<myState_t *> &Rp, uint8_t idThread);

};

#endif //PMC_SOG_CNDFSV2_H
