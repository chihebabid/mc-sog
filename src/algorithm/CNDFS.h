//
// Created by ghofrane on 5/4/22.
//
#ifndef PMC_SOG_CNDFS_H
#define PMC_SOG_CNDFS_H

#include "../ModelCheckBaseMT.h"
#include <spot/tl/apcollect.hh>
#include <cstdint>
#include <spot/twa/twagraph.hh>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "misc/SafeDequeue.h"
#include <spot/tl/contain.hh>

using namespace std;
typedef pair<struct myState_t *, int> coupleSuccessor;
static constexpr uint8_t MAX_THREADS = 64;

enum class SuccState {notyet,beingbuilt,built};
struct myState_t {
    LDDState *left;
    const spot::twa_graph_state *right;
    vector<pair<struct myState_t *, int>> new_successors;
    bool isAcceptance {false};
    bool isConstructed {false};
    array<bool, MAX_THREADS> cyan {false};
    atomic<bool> blue{false};
    atomic<bool> red{false};
    SuccState succState {SuccState::notyet};
};

class CNDFS {
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

    void getInitialState();

    void spawnThreads();

    static void threadHandler(void *context);

    void threadRun();

    void computeSuccessors(myState_t *state, vector<spot::formula> ap_sog);

    myState_t *buildState(LDDState *left, spot::state *right, bool acc, bool constructed, bool cyan);
    myState_t* isStateBuilt(LDDState *sogState,spot::twa_graph_state *spotState);
public:
    CNDFS(ModelCheckBaseMT *mcl, const spot::twa_graph_ptr &af, const uint16_t &nbTh);

    virtual ~CNDFS();

    void dfsBlue(myState_t *state, vector<myState_t *> &Rp, uint8_t idThread,vector<spot::formula> ap_sog);

    void dfsRed(myState_t *state, vector<myState_t *> &Rp, uint8_t idThread);

};

#endif //PMC_SOG_CNDFS_H
