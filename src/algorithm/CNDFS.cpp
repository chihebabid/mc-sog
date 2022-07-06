//
// Created by ghofrane on 5/4/22.
//

#include "CNDFS.h"
#include "ModelCheckBaseMT.h"
#include <iostream>
#include <spot/twa/twagraph.hh>
#include <thread>
#include <vector>
#include <utility>
#include <spot/twa/twa.hh>
#include <bddx.h>
#include <cstddef>
#include <deque>
#include <atomic>
#include <condition_variable>
#include "misc/SafeDequeue.h"
#include "misc/stacksafe.h"

using namespace std;

CNDFS::CNDFS(ModelCheckBaseMT *mcl, const spot::twa_graph_ptr &af, const uint16_t &nbTh) : mMcl(mcl), mAa(af), mNbTh(nbTh)
{
    getInitialState();
    spawnThreads();
}

CNDFS::~CNDFS() {
    for (int i = 0; i < mNbTh; ++i) {
        mlThread[i]->join();
        delete mlThread[i];
    }
}

// Create threads
void CNDFS::spawnThreads() {
    for (int i = 0; i < mNbTh; ++i) {
        mlThread[i] = new thread(threadHandler, this);
        if (mlThread[i] == nullptr) {
            cout << "error: pthread creation failed. " << endl;
        }
    }
}

void CNDFS::threadHandler(void *context) {
    ((CNDFS *) context)->threadRun();
}
void CNDFS::threadRun() {
    uint16_t idThread = mIdThread++;
    vector<myState_t*> Rp;
    dfsBlue(mInitStatePtr,Rp,idThread);
}

//get initial state of the product automata
void CNDFS::getInitialState(){
    mInitStatePtr  = new myState_t; //static_cast<_state *>(malloc(sizeof(_state)));
    mInitStatePtr->left = mMcl->getInitialMetaState();
    mInitStatePtr->right = mAa->get_init_state();
    //mInitStatePtr->new_successors = nullptr;
    mInitStatePtr->isAcceptance = mAa->state_is_accepting(mAa->get_init_state());
    mInitStatePtr->isConstructed = true;
}


//this function is to build a state with list of successor initially null
myState_t* CNDFS::buildState(LDDState* left, spot::state* right, bool acc, bool constructed, bool cyan){
    myState_t * buildStatePtr  = new myState_t;
    buildStatePtr->left = left;
    buildStatePtr->right = dynamic_cast<const spot::twa_graph_state *>(right);
    buildStatePtr->isAcceptance = acc;
    buildStatePtr->isConstructed = constructed;

    return buildStatePtr;
}

std::ostream & operator<<(std::ostream & Str,myState_t* state) {
     Str << "({ Sog state= " << state->left << ", BA state= " << state->right << ", acceptance= "  << state->isAcceptance <<  ", constructed= "  << state->isConstructed << ", red= "  << state->red << ", blue= "  <<  state->blue <<  " }" << endl;
     int i = 0;
     for (const auto & ii : state->new_successors)
     {
         cout << "succ num " << i << ii.first << " with transition " << ii.second<<  endl;
         i++;
     }
    return Str;
}



//block all threads while awaitCondition is false
//void  CNDFS::WaitForTestCompleted(_state* state) {
//    while ( awaitCondition(state) == false) ;
//}

void CNDFS::dfsRed(myState_t *state, vector<myState_t*>& Rp,uint8_t idThread)
{
    cout << "dfsRed" << endl;
    Rp.push_back(state);
    computeSuccessors(state);
    for (const auto& succ:state->new_successors) {
      if (succ.first->cyan[idThread])
      {
          cout << "cycle detected"  << endl;
          exit(0);
      }
      // unvisited and not red state
      if ((find(Rp.begin(), Rp.end(), state) != Rp.end()) && ! succ.first->red )
          dfsRed(succ.first, Rp,idThread);
    }
}

//compute new successors of a product state
void CNDFS::computeSuccessors(myState_t *state)
{
    auto sog_current_state = state->left;
    const spot::twa_graph_state* ba_current_state = state->right;
    while (!sog_current_state);
    while (!sog_current_state->isCompletedSucc());
    auto p = mAa->get_dict(); //avoir le dictionnaire bdd,proposition atomique
    //fetch the state's atomic proposition
    for (const auto & vv: sog_current_state->getMarkedPlaces(mMcl->getPlaceProposition()))
    {
        auto name = string(mMcl->getPlace(vv));
        auto f = spot::formula::ap(name);
        transitionNames.push(f);
        cv.notify_one();
    }
    //iterate over the successors of a current aggregate
    for (const auto &elt : sog_current_state->Successors)
    {
        auto transition = elt.second; // je récupère le numéro du transition
        auto name = string(mMcl->getTransition(transition)); // récuprer le nom de la transition
        auto f = spot::formula::ap(name);// récuperer la proposition atomique qui correspond à la transition
        transitionNames.push(f); // push state'S AP to edge'S AP
        cv.notify_one();
//        cout << " AP in state ";
//        for (auto tn: transitionNames) {
//            cout << tn << " ";
//        }
        //make a copy that we can iterate
        SafeDequeue<spot::formula> tempTransitionNames = transitionNames;
        while (!tempTransitionNames.empty())
        {
            //iterate over the successors of a BA state
            auto ii = mAa->succ_iter(ba_current_state);
            if (ii->first())
                do {
                    if (p->var_map.find(tempTransitionNames.front()) !=p->var_map.end())
                        {
                            //fetch the transition of BA that have the same AP as the SOG transition
                            const bdd result = bdd_ithvar((p->var_map.find(tempTransitionNames.front()))->second);
                            if ((ii->cond() & result) != bddfalse)
                            {
                                //make a copy that we can iterate
//                              SafeDequeue<myCouple> tempSharedPool = sharedPool;
                                SafeDequeue<myState_t*> tempSharedPool = mSharedPoolTemp;
                                while (!tempSharedPool.empty())
                                {
                                   std::lock_guard<std::mutex> guard(mMutex);
//                                    if ((tempSharedPool.front().first == sog_current_state->Successors.at(i).first) &&
//                                      (tempSharedPool.front().second == const_cast<spot::state *>(ii->dst())))
                                   if ((tempSharedPool.front()->left == elt.first) && (tempSharedPool.front()->right == const_cast<spot::state *>(ii->dst())))
                                    {
//                                        nd->cyan = true;
                                        state->new_successors.push_back(make_pair(tempSharedPool.front(), transition));
                                    } else
                                    {
                                        //new terminal state without successors
                                        //state_is_accepting() should only be called on automata with state-based acceptance
                                        myState_t *nd = buildState(elt.first,
                                                                const_cast<spot::state *>(ii->dst()),
                                                                mAa->state_is_accepting(ii->dst()), true, false);
                                        state->new_successors.push_back(make_pair(nd, transition));
                                    }
//                                    mMutex.unlock();
                                    tempSharedPool.try_pop(tempSharedPool.front());

                                }
                            }
                        }
                    }
                while (ii->next());
            mAa->release_iter(ii);
            tempTransitionNames.try_pop(tempTransitionNames.front());
        }
    transitionNames.try_pop(transitionNames.front());
    }
}



//Perform the dfsBlue
void CNDFS::dfsBlue(myState_t *state, vector<myState_t*>& Rp,uint8_t idThread) {

    for (const auto & succ:state->new_successors)
    {
        if (!succ.first->blue && !succ.first->cyan[idThread])
          {
              dfsBlue(succ.first,Rp,idThread);
          }
    }
    state->blue = true;
    if (state->isAcceptance)
    {
        cout << "Acceptance state detected " << endl;
        if (state->left->isDeadLock() || state->left->isDiv())
        {
            cout << "cycle detected: an aggregate with deal_lock or live_lock"  << endl;
            exit(0);
        } else
        {
            Rp.clear();
            dfsRed(state, Rp,idThread); //looking for an accepting cycle
    //        the thread processed the current state waits for all visited accepting nodes (non seed, non red) to turn red
    //        the late red coloring forces the other acceptance states to be processed in post-order by the other threads
            bool cond;
            do {
                cond = true;
                for (auto iter = Rp.begin(); iter != Rp.end() && cond; ++iter) {
                    if ((*iter)->isAcceptance && (*iter != state)) {
                        if (!(*iter)->red) cond = false;
                    }
                }
            } while (!cond);

            for (const auto& qu: Rp) // prune other dfsRed
            {
                qu->red = true;
            }
        }
        cout << "no cycle detected" << endl;
    }
    state->cyan[idThread] = false;
}
spot::bdd_dict_ptr *CNDFS::m_dict_ptr;

