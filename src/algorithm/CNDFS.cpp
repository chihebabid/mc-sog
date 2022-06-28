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
    _state* state = ((CNDFS *) context)->getInitialState();
    ((CNDFS *) context)->dfsBlue(state);
}

//get initial state of the product automata
_state* CNDFS::getInitialState(){
    mMutex.lock();
    _state * initStatePtr  = static_cast<_state *>(malloc(sizeof(_state)));
    initStatePtr->left = mMcl->getInitialMetaState();
    initStatePtr->right = mAa->get_init_state();
    initStatePtr->new_successors =  static_cast<vector<pair<_state *, int>>>(NULL);
    initStatePtr->isAcceptance = mAa->state_is_accepting(mAa->get_init_state());
    initStatePtr->isConstructed = true;
    mMutex.unlock();
//    sharedPool.push_back(make_pair(initStatePtr->left,initStatePtr->right));
    return initStatePtr;
}


//this function is to build a state with list of successor initially null
_state* CNDFS::buildState(LDDState* left, spot::state* right, bool acc, bool constructed, bool cyan){
    bool _cyan{false};
    mMutex.lock();
    _state * buildStatePtr  = static_cast<_state *>(malloc(sizeof(_state)));
    _cyan = cyan;
    buildStatePtr->left = left;
    buildStatePtr->right = dynamic_cast<const spot::twa_graph_state *>(right);
    buildStatePtr->new_successors =  static_cast<vector<pair<_state *, int>>>(NULL);
    buildStatePtr->isAcceptance = acc;
    buildStatePtr->isConstructed = constructed;
    buildStatePtr->cyan= _cyan;
    mMutex.unlock();
    return buildStatePtr;
}

std::ostream & operator<<(std::ostream & Str,_state* state) {
     Str << "({ Sog state= " << state->left << ", BA state= " << state->right << ", acceptance= "  << state->isAcceptance <<  ", constructed= "  << state->isConstructed <<  ", cyan= "  << state->cyan << ", red= "  << state->red << ", blue= "  <<  state->blue <<  " }" << endl;
     int i = 0;
     for (auto ii : state->new_successors)
     {
         cout << "succ num " << i << ii.first << " with transition " << ii.second<<  endl;
         i++;
     }
    return Str;
}

//all visited accepting nodes (non seed, non red) should be red
atomic_bool CNDFS::awaitCondition(_state* state,deque<_state*> localDeque)
{
    vector<bool> localList ;
    for (auto qu: localDeque)
    {
        if ((qu->isAcceptance) && (qu!=state))
            localList.push_back(qu->red); // add all the red values
//            return(qu->red == true);
    }
    //test if all elements are true
    if (all_of(localList.begin(),localList.end(),[] (bool cond) {return cond ==true; }))
    {
        return true;
    }
    return  false;
}

//block all threads while awaitCondition is false
//void  CNDFS::WaitForTestCompleted(_state* state) {
//    while ( awaitCondition(state) == false) ;
//}

void CNDFS::dfsRed(_state* state,deque<_state*> localDeque)
{
    cout << "dfsRed" << endl;
    localDeque.push_back(state);
    computeSuccessors(state);
    for (auto p:state->new_successors) {
      if (p.first->cyan)
      {
          cout << "cycle detected"  << endl;
          exit(0);
      }
      // unvisited and not red state
      if ((find(localDeque.begin(), localDeque.end(), state) != localDeque.end()) && ! p.first->red )
          dfsRed(p.first, localDeque);
    }
}

//compute new successors of a product state
void CNDFS::computeSuccessors(_state *state)
{
    LDDState* sog_current_state = state->left;
    const spot::twa_graph_state* ba_current_state = state->right;
    while (!sog_current_state);
    while (!sog_current_state->isCompletedSucc());
    auto p = mAa->get_dict(); //avoir le dictionnaire bdd,proposition atomique
    //fetch the state's atomic proposition
    for (auto vv: sog_current_state->getMarkedPlaces(mMcl->getPlaceProposition()))
    {
        string name = string(mMcl->getPlace(vv));
        auto f = spot::formula::ap(name);
        transitionNames.push(f);
        cv.notify_one();
    }
    //iterate over the successors of a current aggregate
    for (int i = 0; i < sog_current_state->Successors.size(); i++)
    {
        int transition = sog_current_state->Successors.at(i).second; // je récupère le numéro du transition
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
            spot::twa_succ_iterator *ii = mAa->succ_iter(ba_current_state);
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
                                SafeDequeue<_state*> tempSharedPool = sharedPoolTemp;
                                while (!tempSharedPool.empty())
                                {
                                   std::lock_guard<std::mutex> guard(mMutex);
//                                    if ((tempSharedPool.front().first == sog_current_state->Successors.at(i).first) &&
//                                      (tempSharedPool.front().second == const_cast<spot::state *>(ii->dst())))
                                   if ((tempSharedPool.front()->left == sog_current_state->Successors.at(i).first) && (tempSharedPool.front()->right == const_cast<spot::state *>(ii->dst())))
                                    {
//                                        nd->cyan = true;
                                        state->new_successors.push_back(make_pair(tempSharedPool.front(), transition));
                                    } else
                                    {
                                        //new terminal state without successors
                                        //state_is_accepting() should only be called on automata with state-based acceptance
                                        _state *nd = buildState(sog_current_state->Successors.at(i).first,
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

int i = 1;

//Perform the dfsBlue
void CNDFS::dfsBlue(_state *state) {
    deque<_state*> localDeque;
    uint16_t idThread = mIdThread++;
    cout<< "appel recursif " << i  << endl;
    i++;
    state->cyan = true;
//    sharedPool.push(myCouple (state->left,state->right));
    sharedPoolTemp.push(state);
    cv.notify_one();
//    new_successor.push(coupleSuccessor(state,2));
    computeSuccessors(state);
    cout << " \n current state " << state << endl;
//    cout << "nbr of successors of the current state "<<  state->new_successors.size() << " with thread "<< idThread<< endl;
    for (auto p:state->new_successors)
    {
//        cout << "state " << p.first << endl;
        while ((!p.first->cyan) && (!p.first->blue))
          {
              transitionNames.try_pop(transitionNames.front());
              dfsBlue(p.first);
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
            dfsRed(state, localDeque); //looking for an accepting cycle
    //        the thread processed the current state waits for all visited accepting nodes (non seed, non red) to turn red
    //        the late red coloring forces the other acceptance states to be processed in post-order by the other threads
            std::unique_lock<std::mutex> lk(mMutex);
            while (!awaitCondition(state, localDeque)) cv.wait(lk);
//         cv.wait(lk,[this] {return !awaitCondition(state, localDeque)});
            lk.unlock();
    //        notify once we have unlocked - this is important to avoid a pessimization.
            awaitCondition(state,localDeque)=true;
            cv.notify_all();
            for (auto qu: localDeque) // prune other dfsRed
            {
                qu->red = true;
            }
        }
        cout << "no cycle detected" << endl;
    }
    state->cyan = false;
}
spot::bdd_dict_ptr *CNDFS::m_dict_ptr;

