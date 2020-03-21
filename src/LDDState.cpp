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




vector<pair<LDDState*, int>>* LDDState::getSuccessors() {
    return &Successors;
}

vector<uint16_t> LDDState::getMarkedPlaces(set<uint16_t>& lplacesAP) {
    vector<uint16_t> result;
    MDD mdd=m_lddstate;
    uint16_t depth=0;
    while (mdd>lddmc_true)
    {
        //printf("mddd : %d \n",mdd);
        mddnode_t node=GETNODE(mdd);
        if (lplacesAP.find(depth)!=lplacesAP.end())
        if (mddnode_getvalue(node)==1) {
            result.push_back(depth);

        }

        mdd=mddnode_getdown(node);
        depth++;

    }
    return result;
}

vector<uint16_t> LDDState::getUnmarkedPlaces(set<uint16_t>& lplacesAP) {
    vector<uint16_t> result;
    MDD mdd=m_lddstate;

    uint16_t depth=0;
    while (mdd>lddmc_true)
    {
        //printf("mddd : %d \n",mdd);
        mddnode_t node=GETNODE(mdd);
        if (lplacesAP.find(depth)!=lplacesAP.end())
        if (mddnode_getvalue(node)==0) {
            result.push_back(depth);
        }
        mdd=mddnode_getdown(node);
        depth++;
    }
    return result;
}


