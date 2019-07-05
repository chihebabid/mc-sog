#include <spot/kripke/kripke.hh>
#include "LDDState.h"
#include "HybridKripkeState.h"


HybridKripkeState::~HybridKripkeState()
{
    //dtor
}

static ModelCheckBaseMT * HybridKripkeState::m_builder;
