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

using namespace std ;
typedef pair<struct myState_t*, int> coupleSuccessor;

struct myState_t {
    LDDState *left;
    const spot::twa_graph_state* right;
    vector<pair<struct myState_t*, int>> new_successors;
    bool isAcceptance {false};
    bool isConstructed {false};
    bool cyan {false};
    atomic<bool> blue {false};
    atomic<bool> red {false};
};

class CNDFS {
private:
    static constexpr uint8_t MAX_THREADS=64;
    ModelCheckBaseMT * mMcl;
    spot::twa_graph_ptr mAa;
    uint16_t mNbTh;
    atomic<uint8_t> mIdThread;
    std::thread* mlThread[MAX_THREADS];
    mutex mMutex;
    condition_variable cv;
    void spawnThreads();
    myState_t * mInitStatePtr;
    SafeDequeue<struct myState_t*> mSharedPoolTemp;
    static spot::bdd_dict_ptr* m_dict_ptr;
    void getInitialState();
    static void threadHandler(void *context);
public:
//    SafeDequeue<myCouple> sharedPool;
     SafeDequeue<spot::formula> transitionNames;
     SafeDequeue<coupleSuccessor> new_successor;
    CNDFS(ModelCheckBaseMT *mcl,const spot::twa_graph_ptr &af,const uint16_t& nbTh);
    virtual ~CNDFS();
    void computeSuccessors(myState_t *state);
    void dfsBlue(myState_t *state);
    void dfsRed(myState_t* state, deque<myState_t*> mydeque);
    void WaitForTestCompleted(myState_t* state);
    atomic_bool awaitCondition(myState_t* state,deque<myState_t*> mydeque);
    myState_t* buildState(LDDState* left, spot::state* right, bool acc, bool constructed,bool cyan);

};

#endif //PMC_SOG_CNDFS_H
