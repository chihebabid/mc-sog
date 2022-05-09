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


bool red=false;
bool blue=false;
thread_local bool cyan = false;
struct product_node{
    ModelCheckBaseMT &left;
    shared_ptr<spot::twa_graph> right;
};

struct product_node_succ{
    vector<pair<LDDState*, int> > succ_left;
    spot::twa_succ_iterator &sadd structures of a new node and edge of the product ucc_right;
};

std::mutex mtx;

//CNDFS::CNDFS(auto mK, const shared_ptr<spot::twa_graph> &mAa) : mK(mK), mAa(mAa) {}
CNDFS::~CNDFS()=default;

//structure qui represente le produit de 2 états

 void CNDFS::DfsBlue(ModelCheckBaseMT &mcl,shared_ptr<spot::twa_graph> a) {
     mMcl = &mcl;
    //cout << "First state SOG from CNDFS " << mMcl->getInitialMetaState() << endl;
    //cout << "First state SOG from CNDFS " << typeid(m.getGraph()->getInitialAggregate()->getSuccessors()).name() << endl;
    //cout << "First state BA from CNDFS "  << a->get_init_state()<<endl;


    //iterate succ of BA initial state
    //mtx.lock();
    spot::twa_succ_iterator* i = a->succ_iter(a->get_init_state());
    if (i->first())
        do
        {
            const std::lock_guard<std::mutex> lock(mtx);
            cout << "BA succ "<< i->dst() << "; in thread "
            << std::this_thread::get_id()<<   endl;
        }
        while (i->next());
    a->release_iter(i);
    //mtx.unlock();

     //iterate succ of SOG first state
     vector<pair<LDDState*, int>> * edges =mMcl->getGraph()->getInitialAggregate()->getSuccessors();
     for (const auto& pair : *edges)
     {
       std::cout << "sog succ list "<< endl;
     }
}



//void CNDFS::spawnThreads(int n, ModelCheckBaseMT &mcl, shared_ptr<spot::twa_graph> af  )
//{
//    std::vector<thread> threads(n);
//    // spawn n threads:
//    for (int i = 0; i < n; i++) {
//        threads.push_back(thread(DfsBlue,ref(mcl),af));
//    }
//    for (auto& th : threads) {
//        th.join();
//
//    }
//}

