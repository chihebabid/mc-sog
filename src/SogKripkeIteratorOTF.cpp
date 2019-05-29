#include <spot/kripke/kripke.hh>
#include "LDDGraph.h"
#include "SogKripkeStateOTF.h"
#include "SogKripkeIteratorOTF.h"


SogKripkeIteratorOTF::SogKripkeIteratorOTF(const LDDState* lddstate, bdd cnd):m_lddstate(lddstate), kripke_succ_iterator(cnd)
{

    m_lddstate->setDiv(true);
    for (int i=0;i<m_lddstate->getSuccessors()->size();i++) {
        m_lsucc.push_back(m_lddstate->getSuccessors()->at(i));
    }
   /* if (lddstate->isDeadLock()) {
        m_lsucc.push_back(pair(,-1));
    }*/
    if (lddstate->isDiv()) {
        m_lsucc.push_back(pair<LDDState*,int>(m_lddstate,-1));
    }

}
bool SogKripkeIteratorOTF::first() {

   // cout<<"entering "<<__func__<<endl;
    m_current_edge=0;
    //cout<<"exciting "<<__func__<<endl;
    return m_lsucc.size()!=0;

}

bool SogKripkeIteratorOTF::next() {
    //cout<<"entering "<<__func__<<endl;
    m_current_edge++;
    return m_current_edge<m_lsucc.size();

}

bool SogKripkeIteratorOTF::done() const {
    //cout<<"entring /excit"<<__func__<<endl;
    return m_current_edge==m_lsucc.size();

}

SogKripkeStateOTF* SogKripkeIteratorOTF::dst() const
  {
    //cout<<"enter/excit "<<__func__<<endl;
    return new SogKripkeStateOTF(m_lsucc.at(m_current_edge).first);
  }

bdd SogKripkeIteratorOTF::cond()  const {
    //cout<<"entering "<<__func__<<endl;
    if (m_lsucc.at(m_current_edge).second==-1) return bddtrue;

    string name=m_builder->getTransition(m_lsucc.at(m_current_edge).second);
    //cout<<"Value "<<m_lddstate->getSuccessors()->at(m_current_edge).second<<" Transition name "<<name<<endl;

    spot::bdd_dict *p=m_dict_ptr->get();
    spot::formula f=spot::formula::ap(name);
    bdd   result=bdd_ithvar((p->var_map.find(f))->second);

    //cout<<"Iterator "<<__func__<<"  "<<m_current_edge<<"\n";*/
    //cout<<"exciting "<<__func__<<endl;
    return result & spot::kripke_succ_iterator::cond();
}

/*spot::acc_cond::mark_t SogKripkeIterator::acc() const {
  //cout<<"Iterator acc()\n";
  return 0U;
}*/
SogKripkeIteratorOTF::~SogKripkeIteratorOTF()
{
    //dtor
}

static ModelCheckLace * SogKripkeIteratorOTF::m_builder;
static spot::bdd_dict_ptr* SogKripkeIteratorOTF::m_dict_ptr;
