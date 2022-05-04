//
// Created by ghofrane on 5/4/22.
//
#include "CNDFS.h"
#include "ModelCheckBaseMT.h"
#include <iostream>
#include <spot/tl/apcollect.hh>
#include <spot/twa/twagraph.hh>
using namespace std;

CNDFS::~CNDFS()=default;

void CNDFS::DfsBlue( ModelCheckBaseMT &mcl, shared_ptr<spot::twa_graph> af) {
    cout << "First state SOG from CNDFS " << mcl.getInitialMetaState() << endl;
   cout << "First state BA from CNDFS "  << af->acc() <<endl;
}