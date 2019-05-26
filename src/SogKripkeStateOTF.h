#ifndef SOGKRIPKESTATEOTF_H_INCLUDED
#define SOGKRIPKESTATEOTF_H_INCLUDED


#include "LDDState.h"

class SogKripkeStateOTF : public spot::state
{
public:
    SogKripkeStateOTF(const MDD &m);
    virtual ~SogKripkeStateOTF();

    SogKripkeStateOTF* clone() const override;
    size_t hash() const override;

    int compare(const spot::state* other) const override;

    LDDState* getLDDState()
    {
        return m_state;
    }
    const MDD& get_marking() const;
    double limit_marking(const bdd& m);


private:
    LDDState *m_state;
    MDD ma;
};


#endif // SOGKRIPKESTATE_H_INCLUDED