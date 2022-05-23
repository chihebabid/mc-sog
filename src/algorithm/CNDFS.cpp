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

struct new_state{
    LDDState *left;;
    spot::state* right;
};

list<spot::formula> transitionNames;

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
    uint16_t idThread = mIdThread++;
    while (!mMcl->getInitialMetaState());
    LDDState *initialAgg = mMcl->getInitialMetaState();
    while (!initialAgg->isCompletedSucc());

    //fetch the state's atomic proposition
    for (auto vv: mMcl->getInitialMetaState()->getMarkedPlaces(mMcl->getPlaceProposition()))
    {
        string name = string(mMcl->getPlace(vv));
        auto f = spot::formula::ap(name);
        transitionNames.push_back(f);
    }

    for (int i = 0; i < mMcl->getInitialMetaState()->Successors.size(); i++) {
        cout <<"------"<< i << " SOG's successor ------" << endl;
        int transition = mMcl->getInitialMetaState()->Successors.at(
                i).second; // je récupère le numéro de la première transition
        auto name = string(mMcl->getTransition(transition)); // récuprer le nom de la transition
        auto f = spot::formula::ap(name);// récuperer la proposition atomique qui correspond à la transiition
        transitionNames.push_back(f); // push state'S AP to edge'S AP
        auto p = mAa->get_dict(); //avoir le dictionnaire bdd,proposition atomique
        for (auto it = transitionNames.begin(); it != transitionNames.end(); ++it) {
            if (p->var_map.find(*it) != p->var_map.end()) { // Chercher la transition
                cout << *it << " is a common transaction!" << endl; // p->var_map.find ( f )->second => donne la bdd
            } else cout << *it << " isn't a common transaction" << endl;
        }
    }
}

spot::bdd_dict_ptr *CNDFS::m_dict_ptr;

////
//// Created by ghofrane on 5/4/22.
////
//
//#include "CNDFS.h"
//#include "ModelCheckBaseMT.h"
//#include <iostream>
//#include <spot/twa/twagraph.hh>
//#include <thread>
//#include <vector>
//#include <spot/twa/twa.hh>
//
//using namespace std;
//
//
//struct new_state{
//    LDDState *left;;
//    spot::state* right;
//};
//
////temporary list to collect the successors of a BA state
//vector<bdd>BA_succ_temp;
//
////temporary list to collect the successors of a SOG aggregate
//vector<bdd> SOG_succ_temp;
//
////list of new successors of the product
//vector<bdd> successors_new_node;
//
//
//CNDFS::CNDFS(shared_ptr<SogKripkeTh> k, const spot::twa_graph_ptr &af, const uint16_t &nbTh) : mK(k), mAa(af),
//                                                                                               mNbTh(nbTh) {
//    spawnThreads();
//}
//
////CNDFS::CNDFS(ModelCheckBaseMT *mcl, const spot::twa_graph_ptr &af, const uint16_t &nbTh) : mMcl(mcl), mAa(af),
////                                                                                           mNbTh(nbTh) {
////    spawnThreads();
////}
//
//CNDFS::~CNDFS() {
//    for (int i = 0; i < mNbTh; ++i) {
//        mlThread[i]->join();
//        delete mlThread[i];
//    }
//}
//
///*
// * @Brief Create threads
// */
//void CNDFS::spawnThreads() {
//    for (int i = 0; i < mNbTh; ++i) {
//        mlThread[i] = new thread(threadHandler, this);
//        if (mlThread[i] == nullptr) {
//            cout << "error: pthread creation failed. " << endl;
//        }
//    }
//}
//
//
//void CNDFS::threadHandler(void *context) {
//    ((CNDFS *) context)->computeProduct();
//}
//
///*
// * @brief Compute the synchornized product
// */
//void CNDFS::computeProduct()
//{
//    mIdThread++;
//    //list of successors of a BA state
//    spot::twa_succ_iterator* i = mAa->succ_iter(mAa->get_init_state());
//    if (i->first())
//        do
//        {
//            //std::lock_guard<std::mutex> lg(mMutex);
//            BA_succ_temp.push_back(i->cond());
//        }
//        while (i->next());
//    mAa->release_iter(i);
//
//    for(bdd n : BA_succ_temp)
//    {
//        std::cout << "BA succ"<< n << endl;
//    }
//
////    //list of successors of a SOG aggregate
//    auto  edges = mK->succ_iter(mK->get_init_state());
//    if (edges->first())
//        do
//        {
//            //std::lock_guard<std::mutex> lg(mMutex);
//            SOG_succ_temp.push_back(edges->cond());
//        }
//        while (edges->next());
//    mK->release_iter(edges);
//
//    for(bdd m : SOG_succ_temp)
//    {
//        std::cout << "SOG succ"<< m << endl;
//    }
//
//    for(bdd n : SOG_succ_temp)
//    {
//        for(bdd m : BA_succ_temp)
//        {
//            if (n==m)
//            {
//                successors_new_node.push_back(m);
//            }
//        }
//    }
//
//    if (!successors_new_node.empty())
//    {
//        for(bdd k : successors_new_node)
//        {
////            new_state new_product_state(mK->get_init_state(),mAa->get_init_state());
//            cout << "common succ "<< k << endl;
//        }
//
//    } else{
//        cout << "no common succ" << endl;
//    }
//
//}
