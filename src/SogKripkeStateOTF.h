#ifndef SOGKRIPKESTATEOTF_H_INCLUDED
#define SOGKRIPKESTATEOTF_H_INCLUDED


#include "LDDState.h"
#include "ModelCheckLace.h"

class SogKripkeStateOTF : public spot::state
{
public:
    static ModelCheckLace * m_builder;
    SogKripkeStateOTF(LDDState *st):m_state(st) {
            m_builder->buildSucc(st);
    };
    virtual ~SogKripkeStateOTF();

    SogKripkeStateOTF* clone() const override
    {
        return new SogKripkeStateOTF(m_state);
    }
    size_t hash() const override
    {
        return m_state->getLDDValue();
    }

    int compare(const spot::state* other) const override
    {
        auto o = static_cast<const SogKripkeStateOTF*>(other);
        size_t oh = o->hash();
        size_t h = hash();
        if (h < oh)
            return -1;
        else
            return h > oh;
    }
    LDDState* getLDDState() {
        return m_state;
    }
protected:

private:
    LDDState *m_state;
};


#endif // SOGKRIPKESTATE_H_INCLUDED
