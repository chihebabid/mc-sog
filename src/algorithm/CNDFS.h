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
typedef pair<struct myState*, int> coupleSuccessor;

struct myState{
    LDDState *left;
    const spot::twa_graph_state* right;
    vector<pair<struct myState*, int>> new_successors;
    atomic_bool isAcceptance {false};
    atomic_bool isConstructed {false};
    bool cyan {false};
    atomic_bool blue {false};
    atomic_bool red {false};
};
typedef struct myState _state; // @alias

class CNDFS {

private:
    static constexpr uint8_t MAX_THREADS=64;
    ModelCheckBaseMT * mMcl;
    spot::twa_graph_ptr mAa;
    uint16_t mNbTh;
    atomic<uint8_t> mIdThread;
    static void threadHandler(void *context);
    std::thread* mlThread[MAX_THREADS];
    mutex mMutex;
    condition_variable cv;
    void spawnThreads();

public:

//    typedef myState _state;
     SafeDequeue<struct myState*> sharedPoolTemp;
//    SafeDequeue<myCouple> sharedPool;
     SafeDequeue<spot::formula> transitionNames;
     SafeDequeue<coupleSuccessor> new_successor;
    CNDFS(ModelCheckBaseMT *mcl,const spot::twa_graph_ptr &af,const uint16_t& nbTh);
    virtual ~CNDFS();
    void computeSuccessors(_state *state);
    void dfsBlue(_state *state);
    _state* getInitialState();
    void dfsRed(_state* state, deque<_state*> mydeque);
    void WaitForTestCompleted(_state* state);
    atomic_bool awaitCondition(_state* state,deque<_state*> mydeque);
    _state* buildState(LDDState* left, spot::state* right, bool acc, bool constructed,bool cyan);
    static spot::bdd_dict_ptr* m_dict_ptr;
};

#endif //PMC_SOG_CNDFS_H
