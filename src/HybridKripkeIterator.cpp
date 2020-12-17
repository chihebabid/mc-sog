#include <spot/kripke/kripke.hh>
#include "LDDGraph.h"
#include "HybridKripkeState.h"
#include "HybridKripkeIterator.h"


HybridKripkeIterator::HybridKripkeIterator(HybridKripkeState &st, bdd cnd): kripke_succ_iterator(cnd)
{
   
    m_current_state=&st;
    
}
bool HybridKripkeIterator::first() {

    //cout<<"entering "<<__func__<<endl;
    m_current_edge=0;
    return !m_current_state->getListSucc()->empty();

}

bool HybridKripkeIterator::next() {
   
    m_current_edge++;
    return m_current_edge<m_current_state->getListSucc()->size();

}

bool HybridKripkeIterator::done() const {
    //cout<<"entring /excit"<<__func__<<endl;
    return m_current_edge==m_current_state->getListSucc()->size();
}

HybridKripkeState* HybridKripkeIterator::dst() const
{     
    succ_t succ_elt= m_current_state->getListSucc()->at(m_current_edge);    
   return new HybridKripkeState(succ_elt);  
}

bdd HybridKripkeIterator::cond()  const {
    //cout<<"entering "<<__func__<<endl;
    succ_t succ_elt=m_current_state->getListSucc()->at(m_current_edge);    
    if (succ_elt.transition==-1) return bddtrue;    
    string name=string(m_net->getTransitionName(succ_elt.transition));
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

void HybridKripkeIterator::recycle(HybridKripkeState &st, bdd cond)
{       
        m_current_state=&st;        
        spot::kripke_succ_iterator::recycle(cond);
}

NewNet * HybridKripkeIterator::m_net;
spot::bdd_dict_ptr* HybridKripkeIterator::m_dict_ptr;

