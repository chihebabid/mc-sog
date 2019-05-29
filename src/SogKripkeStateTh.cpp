#include <spot/kripke/kripke.hh>
#include "LDDState.h"
#include "SogKripkeStateTh.h"


SogKripkeStateTh::~SogKripkeStateTh()
{
    //dtor
}

static ModelCheckerTh * SogKripkeStateTh::m_builder;
