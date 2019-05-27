#ifndef SOGKRIPKESTATEOTF_H_INCLUDED
#define SOGKRIPKESTATEOTF_H_INCLUDED


#include "LDDState.h"

class SogKripkeStateOTF : public spot::state
{
public:
    SogKripkeStateOTF(LDDState *st):m_state(st) {};
    virtual ~SogKripkeStateOTF();

    SogKripkeStateOTF* clone() const override;
    size_t hash() const override;

    int compare(const spot::state* other) const override;

    LDDState* getLDDState()
    {
        return m_state;
    }
private:
    LDDState *m_state;
};


#endif // SOGKRIPKESTATE_H_INCLUDED
