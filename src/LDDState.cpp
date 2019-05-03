#include <sylvan.h>

#include <sylvan_int.h>
#include "LDDState.h"
#include "LDDGraph.h"
#include <iostream>
#define GETNODE(mdd) ((mddnode_t)llmsset_index_to_ptr(nodes, mdd))
LDDState::~LDDState()
{
    //dtor
}
void LDDState::setLDDValue(MDD m) {
    m_lddstate=m;
}
MDD  LDDState::getLDDValue() {
    return m_lddstate;
}

unsigned char* LDDState::getSHAValue() {
    return m_SHA2;
}


bool LDDState::isVirtual() {
    return m_virtual;
}

void LDDState::setVirtual() {
    m_virtual=true;
}

vector<pair<LDDState*, int>>* LDDState::getSuccessors() {
    return &Successors;
}

vector<int> LDDState::getMarkedPlaces(set<int>& lplacesAP) {
    vector<int> result;
    MDD mdd=m_lddstate;

    int depth=0;
    while (mdd>lddmc_true)
    {
        //printf("mddd : %d \n",mdd);
        mddnode_t node=GETNODE(mdd);
        if (lplacesAP.find(depth)!=lplacesAP.end())
        if (mddnode_getvalue(node)==1) {
            result.push_back(depth);
            cout<<"depth "<<depth<<endl;
        }

        mdd=mddnode_getdown(node);
        depth++;

    }
    return result;
}



