//
// Created by ghofrane on 5/4/22.
//

#include "CNDFS.h"
#include "ModelCheckBaseMT.h"
#include <iostream>
#include <spot/twa/twagraph.hh>
#include <thread>
#include <vector>
#include <spot/twa/twaproduct.hh>
#include <spot/twa/twa.hh>

using namespace std;


bool red = false;
bool blue = false;
thread_local bool cyan = false;

struct new_state {
    ModelCheckBaseMT &left;
    shared_ptr<spot::twa_graph> right;
};

struct new_state_succ {
    LDDState *succ_left;
    spot::twa_succ_iterator *succ_right;
};

vector<new_state_succ> successors_new_node;




CNDFS::CNDFS(ModelCheckBaseMT *mcl, const spot::twa_graph_ptr &af, const uint16_t &nbTh) : mMcl(mcl), mAa(af),
                                                                                           mNbTh(nbTh) {
    spawnThreads();
}

CNDFS::~CNDFS() {
    for (int i = 0; i < mNbTh; ++i) {
        mlThread[i]->join();
        delete mlThread[i];
    }
}

//structure qui represente le produit de 2 états
/*
void CNDFS::DfsBlue() {

    //cout << "First state SOG from CNDFS " << mMcl->getInitialMetaState() << endl;
    //cout << "First state SOG from CNDFS " << typeid(m.getGraph()->getInitialAggregate()->getSuccessors()).name() << endl;
    //cout << "First state BA from CNDFS "  << a->get_init_state()<<endl;


    //iterate succ of BA initial state
    //mtx.lock();
    spot::twa_succ_iterator *i = mAa->succ_iter(mAa->get_init_state());
    if (i->first())
        do {
            const std::lock_guard<std::mutex> lock(mMutex);
            cout << "BA succ " << i->dst() << "; in thread "
                 << std::this_thread::get_id() << endl;
        } while (i->next());
    mAa->release_iter(i);
    //mtx.unlock();

    //iterate succ of SOG first state
    //error: segmentation fault here don't know how to fix it
    vector<pair<LDDState *, int>> *edges = mMcl->getGraph()->getInitialAggregate()->getSuccessors();
    for (const auto &pair: *edges) {
        std::cout << "sog succ list " << endl;
    }
}
*/

/*
 * @Brief Create threads
 */
void CNDFS::spawnThreads() {
    for (int i = 0; i < mNbTh; ++i) {
        mlThread[i] = new thread(threadHandler, this);
        if (mlThread[i] == nullptr) {
            cout << "error: pthread creation failed. " << endl;
        }
    }
}


void CNDFS::threadHandler(void *context) {
    ((CNDFS *) context)->computeProduct();
}

/*
 * @brief Compute the synchornized product
 */
void CNDFS::computeProduct() {
    uint16_t idThread=mIdThread++;
    //cout<<"My id : "<<mMcl;
    while (!mMcl->getInitialMetaState());
    LDDState * initialAgg=mMcl->getInitialMetaState();
    while (!initialAgg->isCompletedSucc());
    int transition=mMcl->getInitialMetaState()->Successors.at(0).second;
    std::cout<<"La transition : "<<string(mMcl->getTransition ( transition ))<<std::endl;
    auto f=spot::formula::ap (string("Catch1_3"));//string(mMcl->getTransition ( transition )));
    auto p=mAa->get_dict();
    if (p->var_map.find ( f )==p->var_map.end()) {
        cout<<"Ok!";
    }
    //bdd   result=bdd_ithvar ( ( p->var_map.find ( f ) )->second );

    //mAa->edge_data(0)
}

spot::bdd_dict_ptr* CNDFS::m_dict_ptr;