#ifndef HYBRIDKRIPKESTATETH_H_INCLUDED
#define HYBRIDKRIPKESTATETH_H_INCLUDED

#include <mpi.h>
#include "LDDState.h"
#include "ModelCheckBaseMT.h"
#define TAG_ASK_STATE 9
#define TAG_ACK_STATE 10
#define TAG_ASK_SUCC 4
#define TAG_ACK_SUCC 11
#define TAG_NOTCOMPLETED 20
typedef struct {
    char id[17];
    int16_t transition;
    uint16_t pcontainer;
} succ_t;
class HybridKripkeState : public spot::state
{
public:
    //static ModelCheckBaseMT * m_builder;
    HybridKripkeState(unsigned char * id,uint16_t pcontainer):m_container(pcontainer) {        
        memcpy(m_id,id,16);        
        MPI_Send(m_id,16,MPI_BYTE,pcontainer, TAG_ASK_STATE, MPI_COMM_WORLD);       
        
        MPI_Status status; //int nbytes;
        MPI_Probe(MPI_ANY_SOURCE, TAG_ACK_STATE, MPI_COMM_WORLD, &status);
        uint32_t nbytes;
        MPI_Get_count(&status, MPI_BYTE, &nbytes);
        // cout<<"ACK state received..."<<nbytes<<endl;
        char message[nbytes];
        MPI_Recv(message, nbytes, MPI_BYTE,MPI_ANY_SOURCE,TAG_ACK_STATE,MPI_COMM_WORLD, &status);
        memcpy(&m_hashid,message,8);     
        //cout<<"Pos :"<<m_hashid<<endl;
        //cout<<"Process :"<<m_container<<endl;
        m_hashid=m_hashid | (size_t(m_container)<<56);
        
        // Determine Place propositions
        uint16_t n_mp;
        memcpy(&n_mp,message+8,2);
        
        //cout<<" Marked places :"<<n_mp<<endl;
        uint32_t indice=10;
        for(uint16_t i=0;i<n_mp;i++) {
            uint16_t val;
            memcpy(&val,message+indice,2);
            m_marked_places.emplace_front(val);
            indice+=2;            
        }
        uint16_t n_up;
        memcpy(&n_up,message+indice,2);
        //cout<<" Unmarked places :"<<n_mp<<endl;
        indice+=2;
        for(uint16_t i=0;i<n_up;i++) {
            uint16_t val;
            memcpy(&val,message+indice,2);
            m_unmarked_places.emplace_front(val);
            indice+=2;
        }    
        // Get successors 
         MPI_Send(m_id,16,MPI_BYTE,m_container, TAG_ASK_SUCC, MPI_COMM_WORLD);
   
     // Receive list of successors     
     
    MPI_Probe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
    MPI_Get_count(&status, MPI_BYTE, &nbytes);
    char inmsg[nbytes+1];
    //cout<<"Received bytes : "<<nbytes<<endl;
    MPI_Recv(inmsg, nbytes, MPI_BYTE,status.MPI_SOURCE,TAG_ACK_SUCC,MPI_COMM_WORLD, &status);
    uint32_t nb_succ;
    memcpy(&nb_succ,inmsg,4);
    //cout<<"Number of successors "<<nb_succ<<endl;
    indice=4;
    succ_t *succ_elt;
    //printf("List of successors of %.16s\n",m_id);
    for (uint32_t i=0;i<nb_succ;i++) {
        succ_elt=new succ_t;        
        memcpy(succ_elt->id,inmsg+indice,16);
        indice+=16;
        
        memcpy(&(succ_elt->pcontainer),inmsg+indice,2);
        indice+=2;
        memcpy(&(succ_elt->transition),inmsg+indice,2);
        indice+=2;        
        m_succ.push_back(*succ_elt);   
        delete succ_elt;
    }  
        
        
}
    // Constructor for cloning
    HybridKripkeState(unsigned char *id,uint16_t pcontainer,size_t hsh,bool ddiv, bool deadlock):m_container(pcontainer),m_div(ddiv),m_deadlock(deadlock),m_hashid(hsh) {
        memcpy(m_id,id,16); 
    
      //  cout<<__func__<<endl;
        
    }
    virtual ~HybridKripkeState();

    HybridKripkeState* clone() const override
    {
        //cout<<__func__<<endl;
        return new HybridKripkeState(m_id,m_container,m_hashid,m_div,m_deadlock);
    }
    size_t hash() const override
    {                
        return m_hashid;
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
    char * getId() {       
        return m_id;
    }
    uint16_t getContainerProcess() {        
        return m_container;
    }
    list<uint16_t> * getMarkedPlaces() { return &m_marked_places;}
    list<uint16_t> * getUnmarkedPlaces() { return &m_unmarked_places;}
    vector<succ_t>* getListSucc() { return &m_succ;}    
protected:

private:    
    char  m_id[17];
    uint32_t m_pos;
    size_t m_hashid;
    bool m_div;
    bool m_deadlock;
    uint16_t m_container;
    list<uint16_t> m_marked_places;
    list<uint16_t> m_unmarked_places;
    vector<succ_t> m_succ;
};


#endif // SOGKRIPKESTATE_H_INCLUDED
