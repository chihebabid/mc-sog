#include "LDDState.h"


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
