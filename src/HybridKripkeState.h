#ifndef HYBRIDKRIPKESTATETH_H_INCLUDED
#define HYBRIDKRIPKESTATETH_H_INCLUDED

#include <mpi.h>
#include "LDDState.h"
#include "ModelCheckBaseMT.h"

class HybridKripkeState : public spot::state
{
public:
    static ModelCheckBaseMT * m_builder;
    HybridKripkeState(unsigned char * id,uint16_t pcontainer):m_container(pcontainer) {
        memcpy(m_id,id,16);               
    };
    // Constructor for cloning
    HybridKripkeState(unsigned char *id,uint16_t pcontainer,bool ddiv, bool deadlock):m_container(pcontainer),m_div(ddiv),m_deadlock(deadlock) {
        memcpy(m_id,id,16);
    }
    virtual ~HybridKripkeState();

    HybridKripkeState* clone() const override
    {
        return new HybridKripkeState(m_id,m_container,m_div,m_deadlock);
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
    unsigned char * getId() {
        return m_id;
    }
    uint16_t getContainerProcess() {
        return m_container;
    }
protected:

private:
    LDDState *m_state;
    unsigned char  m_id[16];
    bool m_div;
    bool m_deadlock;
    uint16_t m_container;
};


#endif // SOGKRIPKESTATE_H_INCLUDED
