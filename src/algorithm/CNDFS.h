//
// Created by ghofrane on 5/4/22.
//
#ifndef PMC_SOG_CNDFS_H
#define PMC_SOG_CNDFS_H
#include "../ModelCheckBaseMT.h"
#include <spot/tl/apcollect.hh>
#include <cstdint>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <spot/twa/twagraph.hh>

class CNDFS {

private:
    static constexpr uint8_t MAX_THREADS=64;
    ModelCheckBaseMT * mMcl;
    spot::twa_graph_ptr mAa;
    uint16_t mNbTh;
    atomic<uint8_t> mIdThread;
    static void threadHandler(void *context);
    std::thread* mlThread[MAX_THREADS];
    mutex *mMutex;
    condition_variable cv;
    void spawnThreads();

public:
    typedef struct _state{
        LDDState *left;
        const spot::twa_graph_state* right;
        vector<pair<_state* , int>> new_successors ;
        bool isAcceptance = false;
        bool isConstructed = false;
        inline static thread_local atomic_bool cyan = {false};
        bool blue = false;
        bool red = false;
    } _state;


    list<spot::formula> transitionNames;
    CNDFS(ModelCheckBaseMT *mcl,const spot::twa_graph_ptr &af,const uint16_t& nbTh);
    virtual ~CNDFS();
    void computeSuccessors(_state *state);
    void dfsBlue(_state *state);
    _state* getInitialState();
    void dfsRed(_state* state);
    void WaitForTestCompleted(_state* state);
    atomic_bool awaitCondition(_state* state);
    _state* buildState(LDDState* left, spot::state* right, vector<pair<_state *, int>> succ, bool acc, bool constructed);
    static spot::bdd_dict_ptr* m_dict_ptr;
};


#endif //PMC_SOG_CNDFS_H
