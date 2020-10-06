#include <spot/kripke/kripke.hh>
#include "LDDGraph.h"
#include "SogKripkeState.h"
#include "SogKripkeIterator.h"


SogKripkeIterator::SogKripkeIterator(const LDDState* lddstate, bdd cond):kripke_succ_iterator(cond),m_lddstate(lddstate)
{
    //vector<pair<LDDState*, int>>
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
bool SogKripkeIterator::first() {
  //  m_sog->getLDDStateById(m_id)->Successors().
    //return m_sog->get_successor()

    m_current_edge=0;
    return m_lsucc.size()!=0;
}

bool SogKripkeIterator::next() {

    m_current_edge++;
    return m_current_edge<m_lsucc.size();
}

bool SogKripkeIterator::done() const {

    return m_current_edge==m_lsucc.size();
}

SogKripkeState* SogKripkeIterator::dst() const
  {
    return new SogKripkeState(m_lsucc.at(m_current_edge).first);
  }

bdd SogKripkeIterator::cond()  const {
    if (m_lsucc.at(m_current_edge).second==-1) return bddtrue;

    string name=string(m_graph->getTransition(m_lsucc.at(m_current_edge).second));
    //cout<<"Value "<<m_lddstate->getSuccessors()->at(m_current_edge).second<<" Transition name "<<name<<endl;

    spot::bdd_dict *p=m_dict_ptr->get();
    spot::formula f=spot::formula::ap(name);
    bdd   result=bdd_ithvar((p->var_map.find(f))->second);

    //cout<<"Iterator "<<__func__<<"  "<<m_current_edge<<"\n";*/
    //return result;
    return result & spot::kripke_succ_iterator::cond();
}

/*spot::acc_cond::mark_t SogKripkeIterator::acc() const {
  //cout<<"Iterator acc()\n";
  return 0U;
}*/
SogKripkeIterator::~SogKripkeIterator()
{
    //dtor
}

static LDDGraph * SogKripkeIterator::m_graph;
static spot::bdd_dict_ptr* SogKripkeIterator::m_dict_ptr;
