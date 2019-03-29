#include "LDDStateExtend.h"


LDDStateExtend::~LDDStateExtend()
{
    //dtor
}
void LDDStateExtend::setLDDExValue(MDD m) {
    m_lddstate=m;
}
MDD  LDDStateExtend::getLDDExValue() {
    return m_lddstate;
}
