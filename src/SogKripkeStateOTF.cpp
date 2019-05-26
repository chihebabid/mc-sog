#include <spot/kripke/kripke.hh>
#include "LDDState.h"
#include "SogKripkeStateOTF.h"


SogKripkeStateOTF::~SogKripkeStateOTF()
{
    //dtor
}
SogKripkeStateOTF::SogKripkeStateOTF(const MDD& m)
    : spot::state(), ma(m)
{
} //


SogKripkeStateOTF* SogKripkeStateOTF::clone() const
{
    return new SogKripkeStateOTF(*this);
}

size_t SogKripkeStateOTF::hash() const
{
    return m_state->getLDDValue();
}

int SogKripkeStateOTF::compare(const spot::state* other) const
{
    auto o = static_cast<const SogKripkeStateOTF*>(other);
    size_t oh = o->hash();
    size_t h = hash();
    if (h < oh)
        return -1;
    else
        return h > oh;
}

const MDD& SogKripkeStateOTF::get_marking() const
{
    return ma;
} //
