#include <spot/twa/twa.hh>
#include "LDDGraph.h"
#include "SpotSogState.h"
#include "SpotSogIterator.h"


SpotSogIterator::SpotSogIterator(const LDDState* lddstate):m_lddstate(lddstate)
{

}
bool SpotSogIterator::first() {
  //  m_sog->getLDDStateById(m_id)->Successors().
    //return m_sog->get_successor()

    m_current_edge=0;
    return m_lddstate->getSuccessors()->size()!=0;
}

bool SpotSogIterator::next() {

    m_current_edge++;
    return m_current_edge<m_lddstate->getSuccessors()->size();
}

bool SpotSogIterator::done() const {

    return m_current_edge==m_lddstate->getSuccessors()->size();
}

SpotSogState* SpotSogIterator::dst() const
  {
    return new SpotSogState(m_lddstate->getSuccessors()->at(m_current_edge).first);
  }

bdd SpotSogIterator::cond()  const {
    bdd a=bddtrue;//1;
    string name=m_graph->getTransition(m_lddstate->getSuccessors()->at(m_current_edge).second);
    cout<<"Value "<<m_lddstate->getSuccessors()->at(m_current_edge).second<<" Transition name "<<name<<endl;

    spot::bdd_dict *p=m_dict_ptr->get();
    spot::formula f=spot::formula::ap(name);
    bdd   result=bdd_ithvar((p->var_map.find(f))->second);

    //cout<<"Iterator cond()"<<m_current_edge<<"\n";
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
