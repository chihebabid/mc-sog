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
    std::mutex mMutex;
    void spawnThreads();

public:
    typedef struct _state{
        LDDState *left;
        const spot::twa_graph_state* right;
        bool isAcceptance;
        bool isConstructed = false;
    } _state;

    vector<pair<_state , int>> new_successors;
    list<spot::formula> transitionNames;
    vector<bdd>temp;
    CNDFS(ModelCheckBaseMT *mcl,const spot::twa_graph_ptr &af,const uint16_t& nbTh);
    virtual ~CNDFS();
    void computeSuccessors(LDDState* sog_current_state,const spot::twa_graph_state* ba_current_state);
    void dfsBlue(_state *state);
    _state* getInitialState();
    static spot::bdd_dict_ptr* m_dict_ptr;
};


#endif //PMC_SOG_CNDFS_H
