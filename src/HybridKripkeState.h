#ifndef HYBRIDKRIPKESTATETH_H_INCLUDED
#define HYBRIDKRIPKESTATETH_H_INCLUDED

#include <mpi.h>
#include "LDDState.h"
#include "ModelCheckBaseMT.h"
#define TAG_ASK_STATE 9
#define TAG_ACK_STATE 10
class HybridKripkeState : public spot::state
{
public:
    //static ModelCheckBaseMT * m_builder;
    HybridKripkeState(unsigned char * id,uint16_t pcontainer):m_container(pcontainer) {
        cout<<__func__<<"pcontainer :"<<pcontainer<<endl;
        memcpy(m_id,id,16);
        
        MPI_Send(m_id,16,MPI_BYTE,pcontainer, TAG_ASK_STATE, MPI_COMM_WORLD);
        
        
        char message[9];
        MPI_Status status; //int nbytes;
        MPI_Probe(MPI_ANY_SOURCE, TAG_ACK_STATE, MPI_COMM_WORLD, &status);
        cout<<"ACK state received..."<<endl;
        MPI_Recv(message, 8, MPI_BYTE,MPI_ANY_SOURCE,TAG_ACK_STATE,MPI_COMM_WORLD, &status);
        memcpy(&m_hashid,message,8);        
        m_hashid=m_hashid | (size_t(m_container)<<56);
        
        
    }
    // Constructor for cloning
    HybridKripkeState(unsigned char *id,uint16_t pcontainer,size_t hsh,bool ddiv, bool deadlock):m_container(pcontainer),m_div(ddiv),m_deadlock(deadlock),m_hashid(hsh) {
        memcpy(m_id,id,16); 
    
        cout<<__func__<<endl;
        
    }
    virtual ~HybridKripkeState();

    HybridKripkeState* clone() const override
    {
        cout<<__func__<<endl;
        return new HybridKripkeState(m_id,m_container,m_hashid,m_div,m_deadlock);
    }
    size_t hash() const override
    {
        cout<<__func__<<m_hashid<<endl;        
        return m_hashid;
    }

    int compare(const spot::state* other) const override
    {
        cout<<__func__<<endl;
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
        cout<<__func__<<endl;
        return m_id;
    }
    uint16_t getContainerProcess() {
        cout<<__func__<<endl;
        return m_container;
    }
    set<uint16_t> & getMarkedPlaces() { return m_marked_places;}
    set<uint16_t> & getUnmarkedPlaces() { return m_unmarked_places;}
protected:

private:
    //LDDState *m_state;
    char  m_id[17];
    uint32_t m_pos;
    size_t m_hashid;
    bool m_div;
    bool m_deadlock;
    uint16_t m_container;
    set<uint16_t> m_marked_places;
    set<uint16_t> m_unmarked_places;
};


#endif // SOGKRIPKESTATE_H_INCLUDED
