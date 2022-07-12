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
#include <deque>
#include <atomic>
#include <condition_variable>
#include <algorithm>
#include "misc/stacksafe.h"

using namespace std;

CNDFS::CNDFS(ModelCheckBaseMT *mcl, const spot::twa_graph_ptr &af, const uint16_t &nbTh) : mMcl(mcl), mAa(af),
                                                                                           mNbTh(nbTh) {
    getInitialState();
    spawnThreads();
}

CNDFS::~CNDFS() {
    for (int i = 0; i < mNbTh; ++i) {
        mlThread[i]->join();
        delete mlThread[i];
    }
    // Liberate dynamic allocated memory for synchropnized product
    for (const auto & elt : mlBuiltStates)
        delete elt;
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
    vector<myState_t *> Rp;
    dfsBlue(mInitStatePtr, Rp, idThread);
}

/*
 * Build initial state of the product automata
 */
void CNDFS::getInitialState() {
    mInitStatePtr = new myState_t;
    mInitStatePtr->left = mMcl->getInitialMetaState();
    mInitStatePtr->right = mAa->get_init_state();
    mInitStatePtr->isAcceptance = mAa->state_is_accepting(mAa->get_init_state());
    mInitStatePtr->isConstructed = true;
    mlBuiltStates.push_back(mInitStatePtr);
}


//this function is to build a state with list of successor initially null
myState_t *CNDFS::buildState(LDDState *left, spot::state *right, bool acc, bool constructed, bool cyan) {
    myState_t *buildStatePtr = new myState_t;
    buildStatePtr->left = left;
    buildStatePtr->right = (spot::twa_graph_state *) right;
    buildStatePtr->isAcceptance = acc;
    buildStatePtr->isConstructed = constructed;
    return buildStatePtr;
}

std::ostream &operator<<(std::ostream &Str, myState_t *state) {
    Str << "({ Sog state= " << state->left << ", BA state= " << state->right << ", acceptance= " << state->isAcceptance
        << ", constructed= " << state->isConstructed << ", red= " << state->red << ", blue= " << state->blue << " }"
        << endl;
    int i = 0;
    for (const auto &ii: state->new_successors) {
        cout << "succ num " << i << ii.first << " with transition " << ii.second << endl;
        i++;
    }
    return Str;
}

void CNDFS::dfsRed(myState_t *state, vector<myState_t *> &Rp, uint8_t idThread) {
    cout << "dfsRed" << endl;
    Rp.push_back(state);
    computeSuccessors(state);
    for (const auto &succ: state->new_successors) {
        if (succ.first->cyan[idThread]) {
            cout << "cycle detected" << endl;
            exit(0);
        }
        // unvisited and not red state
        if ((find(Rp.begin(), Rp.end(), state) != Rp.end()) && !succ.first->red)
            dfsRed(succ.first, Rp, idThread);
    }
}

/*
 * Check whether a product state exists or not
 */
myState_t *CNDFS::isStateBuilt(LDDState *sogState, spot::twa_graph_state *spotState) {
    auto compare = [sogState, spotState](myState_t *state) {
        return (state->left == sogState && state->right == spotState);
    };
    auto result = find_if(begin(mlBuiltStates), end(mlBuiltStates), compare);
    return result == end(mlBuiltStates) ? nullptr : *result;
}

//compute new successors of a product state
void CNDFS::computeSuccessors(myState_t *state) {
    if (state->succState == SuccState::built) return;
    std::unique_lock lk(mMutexStatus);
    if (state->succState == SuccState::beingbuilt) {
        mDataCondWait.wait(lk, [state] { return state->succState == SuccState::built; });
        mMutexStatus.unlock();
        return;
    }
    state->succState = SuccState::beingbuilt;
    lk.unlock();

    auto sog_current_state = state->left;
    const spot::twa_graph_state *ba_current_state = state->right;
    while (!sog_current_state->isCompletedSucc());
    auto p = mAa->get_dict(); //avoir le dictionnaire bdd,proposition atomique
    //fetch the state's atomic proposition
    /* for (const auto & vv: sog_current_state->getMarkedPlaces(mMcl->getPlaceProposition()))
     {
         auto name = string(mMcl->getPlace(vv));
         auto f = spot::formula::ap(name);
         transitionNames.push(f);

     }*/
    //iterate over the successors of a current aggregate
    for (const auto &elt: sog_current_state->Successors) {
        auto transition = elt.second; // je récupère le numéro du transition
        auto name = string(mMcl->getTransition(transition)); // récuprer le nom de la transition
        auto f = spot::formula::ap(name);// récuperer la proposition atomique qui correspond à la transition

        //iterate over the successors of a BA state
        auto ii = mAa->succ_iter(ba_current_state);
        if (ii->first())
            do {
                if (p->var_map.find(f) != p->var_map.end()) {
                    //fetch the transition of BA that have the same AP as the SOG transition
                    const bdd result = bdd_ithvar((p->var_map.find(f))->second);
                    if ((ii->cond() & result) != bddfalse) {
                        std::unique_lock lk(mMutex);
                        auto result = isStateBuilt(elt.first, (spot::twa_graph_state *) ii->dst());
                        if (result) {
                            state->new_successors.push_back(make_pair(result, transition));
                        } else {
                            myState_t *nd = buildState(elt.first,
                                                       const_cast<spot::state *>(ii->dst()),
                                                       mAa->state_is_accepting(ii->dst()), true, false);
                            state->new_successors.push_back(make_pair(nd, transition));
                        }
                    }
                }
            } while (ii->next());
        mAa->release_iter(ii);
    }
    state->succState = SuccState::built;
    mDataCondWait.notify_all();
}


//Perform the dfsBlue
void CNDFS::dfsBlue(myState_t *state, vector<myState_t *> &Rp, uint8_t idThread) {
    state->cyan[idThread] = true;
    computeSuccessors(state);
    for (const auto &succ: state->new_successors) {
        if (!succ.first->blue && !succ.first->cyan[idThread]) {
            dfsBlue(succ.first, Rp, idThread);
        }
    }
    state->blue = true;
    if (state->isAcceptance) {
        cout << "Acceptance state detected " << endl;
        if (state->left->isDeadLock() || state->left->isDiv()) {
            cout << "cycle detected: an aggregate with deal_lock or live_lock" << endl;
            exit(0);
        } else {
            Rp.clear();
            dfsRed(state, Rp, idThread); //looking for an accepting cycle
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

            for (const auto &qu: Rp) // prune other dfsRed
            {
                qu->red = true;
            }
        }
        cout << "no cycle detected" << endl;
    }
    state->cyan[idThread] = false;
}

spot::bdd_dict_ptr *CNDFS::m_dict_ptr;

