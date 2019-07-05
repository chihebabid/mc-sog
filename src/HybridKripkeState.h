#ifndef HYBRIDKRIPKESTATETH_H_INCLUDED
#define HYBRIDKRIPKESTATETH_H_INCLUDED


#include "LDDState.h"
#include "ModelCheckBaseMT.h"

class HybridKripkeState : public spot::state
{
public:
    static ModelCheckBaseMT * m_builder;
    HybridKripkeState(LDDState *st):m_state(st) {
            m_builder->buildSucc(st);
    };
    virtual ~HybridKripkeState();

    HybridKripkeState* clone() const override
    {
        return new HybridKripkeState(m_state);
    }
    size_t hash() const override
    {
        return m_state->getLDDValue();
    }

    int compare(const spot::state* other) const override
    {
        auto o = static_cast<const HybridKripkeState*>(other);
        size_t oh = o->hash();
        size_t h = hash();
        //return (h!=oh);
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
