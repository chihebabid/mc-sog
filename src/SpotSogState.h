#ifndef SPOTSOGSTATE_H
#define SPOTSOGSTATE_H

#include "LDDState.h"
class SpotSogState : public spot::state
{
public:
    SpotSogState(LDDState *st):m_state(st) {};
    virtual ~SpotSogState();

    SpotSogState* clone() const override
    {
        return new SpotSogState(m_state);
    }
    size_t hash() const override
    {
        return m_state->getLDDValue();
    }

    int compare(const spot::state* other) const override
    {
        auto o = static_cast<const SpotSogState*>(other);
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

#endif // SPOTSOGSTATE_H
