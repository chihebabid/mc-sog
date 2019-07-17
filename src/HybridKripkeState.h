#ifndef HYBRIDKRIPKESTATETH_H_INCLUDED
#define HYBRIDKRIPKESTATETH_H_INCLUDED

#include <mpi.h>
#include "LDDState.h"
#include "ModelCheckBaseMT.h"
#define TAG_ASK_SUCC 4
class HybridKripkeState : public spot::state
{
public:
    static ModelCheckBaseMT * m_builder;
    HybridKripkeState(string &id,uint16_t pcontainer):m_id(id),m_container(pcontainer) {
        int v=1;        
        MPI_Send( &v, 1, MPI_INT, m_container, TAG_ASK_SUCC, MPI_COMM_WORLD); 
                
    };
    // Constructor for cloning
    HybridKripkeState(const string &id,uint16_t pcontainer,bool ddiv, bool deadlock):m_id(id),m_container(pcontainer),m_div(ddiv),m_deadlock(deadlock) {
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
    LDDState* getLDDState() {
        return m_state;
    }
protected:

private:
    LDDState *m_state;
    string m_id;
    bool m_div;
    bool m_deadlock;
    uint16_t m_container;
};


#endif // SOGKRIPKESTATE_H_INCLUDED
