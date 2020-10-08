
#include "LDDState.h"
#include "LDDGraph.h"
#include <iostream>
#include "SylvanWrapper.h"

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
    int depth=0;
    for (auto & iter : lplacesAP) {
        mddnode_t node=SylvanWrapper::GETNODE(mdd);
        for (; depth < iter; depth++) {
            mdd=SylvanWrapper::mddnode_getdown(node);
            node=SylvanWrapper::GETNODE(mdd);
        }
        if (SylvanWrapper::mddnode_getvalue(node)!=0) {
            result.push_back(depth);
        }
    }
    return result;
}

vector<uint16_t> LDDState::getUnmarkedPlaces(set<uint16_t>& lplacesAP) {
    vector<uint16_t> result;
    MDD mdd=m_lddstate;
    int depth=0;
    for (auto & iter : lplacesAP) {
        mddnode_t node=SylvanWrapper::GETNODE(mdd);
        for (; depth < iter; depth++) {
            mdd=SylvanWrapper::mddnode_getdown(node);
            node=SylvanWrapper::GETNODE(mdd);
        }
        if (SylvanWrapper::mddnode_getvalue(node)==0) {
            result.push_back(depth);
        }
    }
    return result;
}


