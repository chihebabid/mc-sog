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
#include <spot/twa/formula2bdd.hh>
#include <spot/tl/formula.hh>
#include <random>

using namespace std;

CNDFS::CNDFS(ModelCheckBaseMT *mcl, const spot::twa_graph_ptr &af, const uint16_t &nbTh) : mMcl(mcl), mAa(af),
                                                                                           mNbTh(nbTh) {
    dict_ba = mAa->get_dict();
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
    vector<spot::formula> ap_sog;
    dfsBlue(mInitStatePtr, Rp, idThread, ap_sog);
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
    mlBuiltStates.push_back(buildStatePtr);
    return buildStatePtr;
}

std::ostream &operator<<(std::ostream &Str, myState_t *state) {
    Str << "({ Sog state= " << state->left << ", BA state= " << state->right << ", acceptance= " << state->isAcceptance
        << ", constructed= " << state->isConstructed << ", red= " << state->red << ", blue= " << state->blue << " }"
        << endl;
    int i = 0;
    for (const auto &ii: state->new_successors) {
        Str << "succ num " << i << ii.first << " with transition " << ii.second << endl;
        i++;
    }
    return Str;
}

void CNDFS::dfsRed(myState_t *state, vector<myState_t *> &Rp, uint8_t idThread) {
//    cout << "dfsRed" << endl;
    Rp.push_back(state);
//    computeSuccessors(state);
    for (const auto &succ: state->new_successors) {
//        cout << "dfs red 1 "  << succ.first->cyan[idThread]<< endl;
        if (succ.first->cyan[idThread]) {
            cout << "cycle detected with the thread " << unsigned(idThread) << endl;
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
void CNDFS::computeSuccessors(myState_t *state, vector<spot::formula> ap_sog) {
    if (state->succState == SuccState::done) return;
    std::unique_lock lk(mMutexStatus);
    if (state->succState == SuccState::doing) {
        mDataCondWait.wait(lk, [state] { return state->succState == SuccState::done; });
        mMutexStatus.unlock();
        return;
    }
    state->succState = SuccState::doing;
    lk.unlock();
    auto sog_current_state = state->left;
    const spot::twa_graph_state *ba_current_state = state->right;
    while (!sog_current_state->isCompletedSucc());
    //fetch the state's atomic proposition
    for (const auto & vv: sog_current_state->getMarkedPlaces(mMcl->getPlaceProposition()))
     {
         auto name = string(mMcl->getPlace(vv));
         auto ap_state = spot::formula::ap(name);
         if (dict_ba->var_map.find(ap_state) != dict_ba->var_map.end()) {
             ap_sog.push_back(ap_state);
             for( auto n: dict_ba->var_map)
             {
                 if (n.first != ap_state)
                 {
                     ap_sog.push_back(spot::formula::Not(n.first));
                 }
             }
         }
     }
    //iterate over the successors of a current aggregate
    for (const auto &elt: sog_current_state->Successors) {
        auto transition = elt.second; // je récupère le numéro du transition
        auto name = string(mMcl->getTransition(transition)); // récuprer le nom de la transition
        auto ap_edge = spot::formula::ap(name);// récuperer la proposition atomique qui correspond à la transition
        if (dict_ba->var_map.find(ap_edge) != dict_ba->var_map.end()) {
            ap_sog.push_back(ap_edge);
        }

        spot::formula pa_sog_result = spot::formula::And(ap_sog);
//        cout << "formula sog: " << pa_sog_result << endl;
        //iterate over the successors of a BA state
        auto ii = mAa->succ_iter(ba_current_state);
         if (ii->first())
                do {
                   auto pa_ba_result = spot::bdd_to_formula(ii->cond(), dict_ba); // from bdd to formula
                   if (c.contained(pa_sog_result, pa_ba_result) || c.contained(pa_ba_result, pa_sog_result))
                   {
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
                } while (ii->next());
         mAa->release_iter(ii);
        }
    state->succState = SuccState::done;
    mDataCondWait.notify_all();
}


//Perform the dfsBlue
void CNDFS::dfsBlue(myState_t *state, vector<myState_t *> &Rp, uint8_t idThread, vector<spot::formula> ap_sog) {
    state->cyan[idThread] = true;
    computeSuccessors(state, ap_sog);
    std::mt19937 g(rd());
    std::shuffle(state->new_successors.begin(), state->new_successors.end(), g);
    for (const auto &succ: state->new_successors) {
//        cout << "cyan " << succ.first->cyan[idThread] << endl;
        if (!succ.first->blue && !succ.first->cyan[idThread]) {
            dfsBlue(succ.first, Rp, idThread, ap_sog);
        }
    }
    state->blue = true;
    if (state->isAcceptance) {
//        cout << "Acceptance state detected " << endl;
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

