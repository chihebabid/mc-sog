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
CNDFS::_state* CNDFS::getInitialState(){
    _state * initStatePtr  = static_cast<_state *>(malloc(sizeof(_state)));
    initStatePtr->left = mMcl->getInitialMetaState();
    initStatePtr->right = mAa->get_init_state();
    initStatePtr->isAcceptance = mAa->state_is_accepting(mAa->get_init_state());
    initStatePtr->isConstructed = true;
    return initStatePtr;
}

//compute new successors of a product state
void CNDFS::computeSuccessors(LDDState *sog_current_state, const spot::twa_graph_state *ba_current_state)
{

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
        for (auto v: p->var_map)
        {
            cout << v.first << "-----" << v.second << endl;
        }

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
                            //new terminal state
                            _state nd = {sog_current_state->Successors.at(i).first, dynamic_cast<const spot::twa_graph_state *> (ii->dst()), mAa->state_is_accepting(ii->dst()),true};
                            new_successors.push_back(make_pair(nd,transition));
                            cout << *it << " is a common transaction!" << endl; // p->var_map.find ( f )->second => donne la bdd
                        }
                    }
                    while (ii->next());
                mAa->release_iter(ii);
            } else cout << *it << " isn't a common transaction" << endl;
        }
        transitionNames.pop_back();
    }

}

//Compute the synchronized product
void CNDFS::dfsBlue(_state *state) {
    uint16_t idThread = mIdThread++;

    cout << state->isConstructed << endl;
    computeSuccessors(state->left,state->right);
        for (auto p: new_successors) {
            std::cout << "list of successors of current state  " << "({ " << p.first.right << ", " << p.first.left << ", "  << p.first.isAcceptance <<  ", "  << p.first.isConstructed << " }" << ", " << p.second << ") ";
        }

//        for (auto k = new_successors.begin(); k != new_successors.end(); ++k)
//        {
//            sog_current_state= k->first.left;
//            ba_current_state= dynamic_cast<const spot::twa_graph_state *>(k->first.right);
//            //computeSuccessors(sog_current_state,ba_current_state);
//            if (! k->first.isConstructed)
//                dfsBlue();
//        }


}
spot::bdd_dict_ptr *CNDFS::m_dict_ptr;

