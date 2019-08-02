#include <spot/kripke/kripke.hh>
#include "LDDGraph.h"
#include "HybridKripkeState.h"
#include "HybridKripkeIterator.h"
#define TAG_ASK_SUCC 4
#define TAG_ACK_SUCC 11
#define TAG_NOTCOMPLETED 20

HybridKripkeIterator::HybridKripkeIterator(HybridKripkeState &kstate, bdd cnd):m_current_state(&kstate), kripke_succ_iterator(cnd)
{
    cout<<__func__<<endl;
    // Ask list of successors 
     MPI_Send(m_current_state->getId(),16,MPI_BYTE,m_current_state->getContainerProcess(), TAG_ASK_SUCC, MPI_COMM_WORLD);
     cout<<"Container process to ask :"<<m_current_state->getContainerProcess()<<endl;
     // Receive list of successors
     
     MPI_Status status;
     MPI_Probe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
     uint32_t nbytes;
    MPI_Get_count(&status, MPI_BYTE, &nbytes);
    char inmsg[nbytes+1];
    cout<<"Received bytes : "<<nbytes<<endl;
    MPI_Recv(inmsg, nbytes, MPI_BYTE,status.MPI_SOURCE,TAG_ACK_SUCC,MPI_COMM_WORLD, &status);
    uint32_t nb_succ;
    memcpy(&nb_succ,inmsg,4);
    cout<<"Number of successors : "<<nb_succ<<endl;
    uint32_t indice=4;
    for (uint32_t i=0;i<nb_succ;i++) {
        succ_t succ_elt;
        
        memcpy(succ_elt.id,inmsg+indice,16);
        indice+=16;
        
        memcpy(&(succ_elt.pcontainer),inmsg+indice,2);
        indice+=2;
        memcpy(&(succ_elt.transition),inmsg+indice,2);
        indice+=2;
        
        m_succ.push_back(succ_elt);      
    }
    
   /*if (lddstate->isDeadLock()) {
     m_lsucc.push_back(pair<LDDState*,int>(&m_deadlock,-1));
    }
    if (lddstate->isDiv()) {
        m_lsucc.push_back(pair<LDDState*,int>(&m_div,-1));
    }*/

}
bool HybridKripkeIterator::first() {

    cout<<"entering "<<__func__<<endl;
    m_current_edge=0;
    
    return m_succ.size()!=0;

}

bool HybridKripkeIterator::next() {
   
    m_current_edge++;
    return m_current_edge<m_succ.size();

}

bool HybridKripkeIterator::done() const {
    cout<<"entring /excit"<<__func__<<endl;
    return m_current_edge==m_succ.size();
}

HybridKripkeState* HybridKripkeIterator::dst() const
  {
     succ_t succ_elt= m_succ.at(m_current_edge);
   return new HybridKripkeState(succ_elt.id,succ_elt.pcontainer);
  
  }

bdd HybridKripkeIterator::cond()  const {
    cout<<"entering "<<__func__<<endl;
    succ_t succ_elt=m_succ.at(m_current_edge);
    
    if (succ_elt.transition==-1) return bddtrue;
    
    string name=m_net->getTransitionName(succ_elt.transition);

    spot::bdd_dict *p=m_dict_ptr->get();
    spot::formula f=spot::formula::ap(name);
    bdd   result=bdd_ithvar((p->var_map.find(f))->second);

    return result & spot::kripke_succ_iterator::cond();
}

/*spot::acc_cond::mark_t SogKripkeIterator::acc() const {
  //cout<<"Iterator acc()\n";
  return 0U;
}*/
HybridKripkeIterator::~HybridKripkeIterator()
{
    
}

void HybridKripkeIterator::recycle(HybridKripkeState* aggregate, bdd cond)
{
        cout<<__func__<<endl;
        m_current_state=aggregate;
        spot::kripke_succ_iterator::recycle(cond);
}

/*HybridKripkeState* HybridKripkeIterator::current_state() const {
    cout<<__func__<<endl;
    return m_current_state;
}*/
static NewNet * HybridKripkeIterator::m_net;
static spot::bdd_dict_ptr* HybridKripkeIterator::m_dict_ptr;
static LDDState HybridKripkeIterator::m_deadlock;
static LDDState HybridKripkeIterator::m_div;
