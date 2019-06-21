#include <spot/kripke/kripke.hh>
#include "LDDState.h"
#include "SogKripkeStateTh.h"


SogKripkeStateTh::~SogKripkeStateTh()
{
    //dtor
}

static ModelCheckBaseMT * SogKripkeStateTh::m_builder;
