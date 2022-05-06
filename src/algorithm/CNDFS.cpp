//
// Created by ghofrane on 5/4/22.
//
#include "CNDFS.h"
#include "ModelCheckBaseMT.h"
#include <iostream>
#include <spot/twa/twagraph.hh>
#include <thread>
#include <vector>

using namespace std;

bool red=false;
bool blue=false;
thread_local bool cyan = false;

CNDFS::~CNDFS()=default;

void CNDFS::DfsBlue(ModelCheckBaseMT &m,shared_ptr<spot::twa_graph> a) {
    cout << "First state SOG from CNDFS " << m.getInitialMetaState() << endl;
   cout << "First state BA from CNDFS "  << a->get_init_state()<<endl;
}

void CNDFS::spawnThreads(int n, ModelCheckBaseMT &mcl, shared_ptr<spot::twa_graph> af  )
{
    std::vector<thread> threads(n);
    // spawn n threads:
    for (int i = 0; i < n; i++) {
        threads.push_back(thread(DfsBlue,ref(mcl),af));
    }
    for (auto& th : threads) {
        th.join();
    }
}