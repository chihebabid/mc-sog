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
#include <condition_variable>
using namespace std;


thread_local deque<CNDFS::_state*> mydeque ;
thread_local list<bool> mytempList ;

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
CNDFS::_state* CNDFS::getInitialState(){
    _state * initStatePtr  = static_cast<_state *>(malloc(sizeof(_state)));
    initStatePtr->left = mMcl->getInitialMetaState();
    initStatePtr->right = mAa->get_init_state();
    initStatePtr->new_successors =  static_cast<vector<pair<_state *, int>>>(NULL);
    initStatePtr->isAcceptance = mAa->state_is_accepting(mAa->get_init_state());
    initStatePtr->isConstructed = true;
    return initStatePtr;
}


//this function is to build a state with list of successor initially null
CNDFS::_state* CNDFS::buildState(LDDState* left, spot::state* right, vector<pair<_state *, int>> succ, bool acc, bool constructed){
    _state * buildStatePtr  = static_cast<_state *>(malloc(sizeof(_state)));
    buildStatePtr->left = left;
    buildStatePtr->right = dynamic_cast<const spot::twa_graph_state *>(right);
    buildStatePtr->new_successors =  static_cast<vector<pair<_state *, int>>>(NULL);
    buildStatePtr->isAcceptance = acc;
    buildStatePtr->isConstructed = constructed;
    return buildStatePtr;
}

std::ostream & operator<<(std::ostream & Str, CNDFS::_state* state) {
     Str << "({ Sog state= " << state->left << ", BA state= " << state->right << ", acceptance= "  << state->isAcceptance <<  ", constructed= "  << state->isConstructed <<  ", cyan= "  << state->cyan << ", red= "  << state->red << ", blue= "  <<  state->blue <<" }" << endl;
    return Str;
}

//algo line 22 test
atomic_bool CNDFS::awaitCondition(_state* state)
{
    for (auto qu: mydeque)
    {
        if ((qu->isAcceptance) && (qu!=state))
            mytempList.push_back(qu->red); // add all the red values
//            return(qu->red == true);
    }
    //test if all elements are true
    if (all_of(mytempList.begin(),mytempList.end(),[] (bool cond) {return cond ==true; }))
    {
        return true;
    }
    return  false;
}


//block all threads while awaitCondition is false
void  CNDFS::WaitForTestCompleted(_state* state) {
    while ( awaitCondition(state) == false) ;
}


void CNDFS::dfsRed(_state* state)
{
    cout << "dfsRed" << endl;
    mydeque.push_back(state);
    computeSuccessors(state);
    for (auto p:state->new_successors) {
      if (p.first->cyan)
      {
          cout << "cycle detected"  << endl;
          exit(1);
      }
      if ((find(mydeque.begin(), mydeque.end(), state) != mydeque.end()) && ! p.first->red )
          dfsRed(p.first);
    }
}

//compute new successors of a product state
void CNDFS::computeSuccessors(_state *state)
{
    LDDState* sog_current_state = state->left;
    const spot::twa_graph_state* ba_current_state = state->right;
    while (!sog_current_state);
    while (!sog_current_state->isCompletedSucc());
    //fetch the state's atomic proposition
    for (auto vv: sog_current_state->getMarkedPlaces(mMcl->getPlaceProposition()))
    {
        string name = string(mMcl->getPlace(vv));
        auto f = spot::formula::ap(name);
        transitionNames.push_back(f);
    }

    for (auto tn: transitionNames)
    {
        cout << " \n AP in state " << tn << endl;
    }

    //iterate over the successors of an aggregate
    for (int i = 0; i < sog_current_state->Successors.size(); i++)
    {
        cout <<"------"<< " SOG's successor " << i << " ------" << endl;
        int transition = sog_current_state->Successors.at(i).second; // je récupère le numéro du transition
        auto name = string(mMcl->getTransition(transition)); // récuprer le nom de la transition
        auto f = spot::formula::ap(name);// récuperer la proposition atomique qui correspond à la transition
        transitionNames.push_back(f); // push state'S AP to edge'S AP
        auto p = mAa->get_dict(); //avoir le dictionnaire bdd,proposition atomique
        //cout << p->bdd_map.size() << endl;
//        for (auto v: p->var_map)
//        {
//            cout << v.first << "-----" << v.second << endl;
//        }

        for (auto it = transitionNames.begin(); it != transitionNames.end(); ++it)
        {
            if (p->var_map.find(*it) != p->var_map.end())
            { //fetch the transition of BA that have the same AP as the SOG transition
                const bdd  result=bdd_ithvar((p->var_map.find(*it))->second);
                cout << "dbb result " << result << endl;
                //iterate over the successors of a BA state
                spot::twa_succ_iterator* ii = mAa->succ_iter(ba_current_state);
                if (ii->first())
                    do
                    {
                        bdd matched = ii->cond()&result;
                        if (matched!=bddfalse)
                        {
                            cout << "matched bdd " << matched << endl;
                            //new terminal state without successors
//                            std::lock_guard<std::mutex> lg(*mMutex);
                            _state* nd = buildState(sog_current_state->Successors.at(i).first,
                                                    const_cast<spot::state *>(ii->dst()), static_cast<vector<pair<_state *, int>>>(NULL), mAa->state_is_accepting(ii->dst()), true);
                            cout << "new succ state " << nd ;
                            //add the successor found to the successor's vector of the current state
                            state->new_successors.push_back(make_pair(nd,transition));
                        }
                    }
                    while (ii->next());
                mAa->release_iter(ii);
                cout << *it << " is a common transaction!" << endl;
            } else cout << *it << " isn't a common transaction" << endl;
        }
        transitionNames.pop_back();
    }
}

//Compute the synchronized product
void CNDFS::dfsBlue(_state *state) {
    uint16_t idThread = mIdThread++;
    cout << state;
    state->cyan = true;
//    cout << "first state " << "({ " << state->right << ", " << state->left << ", "  << state->isAcceptance <<  ", "  << state->isConstructed <<  ", "  << state->cyan << ", "  << state->red << ", "  <<  state->blue <<" }" << endl;
    computeSuccessors(state);
    for (auto p:state->new_successors)
    {
        cout <<"STATE 1 " <<  state<< endl;
        cout << "STATE 2 "<<  p.first<< endl;
         cout << " cyan " <<  p.first->cyan << endl;
//          cout << "list of successors of current state  " << "({ " << p.first->right << ", " << p.first->left << ", "  << p.first->isAcceptance <<  ", "  << p.first->isConstructed <<  ", "  << p.first->cyan << ", "  << p.first->red << ", "  <<  p.first->blue <<" }"  << ", " << p.second << ") " << endl;
          while((p.first->cyan == 0) && (p.first->blue == 0))
          {
              cout << " appel recursive 2 " << endl;
              dfsBlue(p.first);
          }

    }
    state->blue = true;
//        if (state->isAcceptance)
//        {
//            dfsRed(state);
//            WaitForTestCompleted(state);
//            std::lock_guard<std::mutex> lk(*mMutex);
//            cv.notify_all();
//            for (auto qu: mydeque)
//            {
//                qu->red = true;
//            }
//        }
//        state->cyan = false;
}
spot::bdd_dict_ptr *CNDFS::m_dict_ptr;

