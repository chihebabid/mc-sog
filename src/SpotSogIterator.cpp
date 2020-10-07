#include <spot/twa/twa.hh>
#include "LDDGraph.h"
#include "SpotSogState.h"
#include "SpotSogIterator.h"


SpotSogIterator::SpotSogIterator(const LDDState* lddstate):m_lddstate(lddstate)
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
bool SpotSogIterator::first() {
  //  m_sog->getLDDStateById(m_id)->Successors().
    //return m_sog->get_successor()

    m_current_edge=0;
    return m_lsucc.size()!=0;
}

bool SpotSogIterator::next() {

    m_current_edge++;
    return m_current_edge<m_lsucc.size();
}

bool SpotSogIterator::done() const {

    return m_current_edge==m_lsucc.size();
}

SpotSogState* SpotSogIterator::dst() const
  {
    return new SpotSogState(m_lsucc.at(m_current_edge).first);
  }

bdd SpotSogIterator::cond()  const {
    if (m_lsucc.at(m_current_edge).second==-1) return bddtrue;
    string name=string(m_graph->getTransition(m_lsucc.at(m_current_edge).second));
    //cout<<"Value "<<m_lddstate->getSuccessors()->at(m_current_edge).second<<" Transition name "<<name<<endl;

    spot::bdd_dict *p=m_dict_ptr->get();
    spot::formula f=spot::formula::ap(name);
    bdd   result=bdd_ithvar((p->var_map.find(f))->second);

    //cout<<"Iterator "<<__func__<<"  "<<m_current_edge<<"\n";
    return result;
}

spot::acc_cond::mark_t SpotSogIterator::acc() const {
  //cout<<"Iterator acc()\n";
  return 1U;
}
SpotSogIterator::~SpotSogIterator()
{
    //dtor
}

static LDDGraph * SpotSogIterator::m_graph;
static spot::bdd_dict_ptr* SpotSogIterator::m_dict_ptr;
