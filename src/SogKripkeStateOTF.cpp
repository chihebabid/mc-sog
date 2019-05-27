#include <spot/kripke/kripke.hh>
#include "LDDState.h"
#include "SogKripkeStateOTF.h"


SogKripkeStateOTF::~SogKripkeStateOTF()
{
    //dtor
}

static ModelCheckLace * SogKripkeStateOTF::m_builder;
