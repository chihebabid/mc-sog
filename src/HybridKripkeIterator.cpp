#include <spot/kripke/kripke.hh>
#include "LDDGraph.h"
#include "HybridKripkeState.h"
#include "HybridKripkeIterator.h"


HybridKripkeIterator::HybridKripkeIterator(const LDDState* lddstate, bdd cnd):m_lddstate(lddstate), kripke_succ_iterator(cnd)
{
    for (int i=0;i<m_lddstate->getSuccessors()->size();i++) {
        m_lsucc.push_back(m_lddstate->getSuccessors()->at(i));
    }
   if (lddstate->isDeadLock()) {
     m_lsucc.push_back(pair<LDDState*,int>(&m_deadlock,-1));
    }
    if (lddstate->isDiv()) {
        m_lsucc.push_back(pair<LDDState*,int>(&m_div,-1));
    }

}
bool HybridKripkeIterator::first() {

   // cout<<"entering "<<__func__<<endl;
    m_current_edge=0;
    //cout<<"exciting "<<__func__<<endl;
    return m_lsucc.size()!=0;

}

bool HybridKripkeIterator::next() {
    //cout<<"entering "<<__func__<<"   "<<m_current_edge<<endl;
    m_current_edge++;
    return m_current_edge<m_lsucc.size();

}

bool HybridKripkeIterator::done() const {
    //cout<<"entring /excit"<<__func__<<endl;
    return m_current_edge==m_lsucc.size();

}

HybridKripkeState* HybridKripkeIterator::dst() const
  {
    /*cout<<"Source "<<m_lddstate->getLDDValue()<<"Destination :"<<m_lsucc.at(m_current_edge).first->getLDDValue()<<" in "<<m_lsucc.size()<<" / "<<m_current_edge<<endl;*/
    return new HybridKripkeState(m_lsucc.at(m_current_edge).first);
  }

bdd HybridKripkeIterator::cond()  const {
    //cout<<"entering "<<__func__<<endl;
    if (m_lsucc.at(m_current_edge).second==-1) return bddtrue;

    string name=m_builder->getTransition(m_lsucc.at(m_current_edge).second);
    //cout<<"Value "<<m_lddstate->getSuccessors()->at(m_current_edge).second<<" Transition name "<<name<<endl;

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
    //dtor
}

void HybridKripkeIterator::recycle(LDDState* aggregate, bdd cond)
{
        m_lddstate=aggregate;
        spot::kripke_succ_iterator::recycle(cond);
}

static ModelCheckBaseMT * HybridKripkeIterator::m_builder;
static spot::bdd_dict_ptr* HybridKripkeIterator::m_dict_ptr;
static LDDState HybridKripkeIterator::m_deadlock;
static LDDState HybridKripkeIterator::m_div;
