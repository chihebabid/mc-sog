#include <spot/kripke/kripke.hh>
#include "LDDGraph.h"
#include "SogKripkeStateTh.h"
#include "SogKripkeIteratorTh.h"


SogKripkeIteratorTh::SogKripkeIteratorTh ( LDDState* lddstate, bdd cnd ) :m_lddstate ( lddstate ), kripke_succ_iterator ( cnd )
{
    m_lsucc.reserve(m_lddstate->getSuccessors()->size()+ 2);
    if ( lddstate->isDeadLock() ) {
        m_lsucc.push_back ( pair<LDDState*,int> ( &m_deadlock,-1 ) );
    }
    if ( lddstate->isDiv() ) {
        m_lsucc.push_back ( pair<LDDState*,int> ( (LDDState*)lddstate,-1 ) );
    }

    for ( int i=0; i<m_lddstate->getSuccessors()->size(); i++ ) {
        m_lsucc.push_back ( m_lddstate->getSuccessors()->at ( i ) );
    }


}
bool SogKripkeIteratorTh::first()
{

    // cout<<"entering "<<__func__<<endl;
    m_current_edge=0;
    //cout<<"exciting "<<__func__<<endl;
    return m_lsucc.size() !=0;

}

bool SogKripkeIteratorTh::next()
{
    //cout<<"entering "<<__func__<<"   "<<m_current_edge<<endl;
    m_current_edge++;
    return m_current_edge<m_lsucc.size();

}

bool SogKripkeIteratorTh::done() const
{
    //cout<<"entring /excit"<<__func__<<endl;
    return m_current_edge==m_lsucc.size();

}

SogKripkeStateTh* SogKripkeIteratorTh::dst() const
{
    //cout<<"Source "<<m_lddstate->getLDDValue()<<"Destination :"<<m_lsucc.at(m_current_edge).first->getLDDValue()<<" in "<<m_lsucc.size()<<" / "<<m_current_edge<<endl;
    return new SogKripkeStateTh ( m_lsucc.at ( m_current_edge ).first );
}

bdd SogKripkeIteratorTh::cond()  const
{
    if ( m_lsucc.at ( m_current_edge ).second==-1 ) {
        return bddtrue;
    }
    spot::formula f=spot::formula::ap (string(m_builder->getTransition ( m_lsucc.at ( m_current_edge ).second )));
    spot::bdd_dict *p=m_dict_ptr->get();
    bdd   result=bdd_ithvar ( ( p->var_map.find ( f ) )->second );
    return result & spot::kripke_succ_iterator::cond();
}


SogKripkeIteratorTh::~SogKripkeIteratorTh()
{
    //dtor
}

void SogKripkeIteratorTh::recycle ( LDDState* aggregate, bdd cond )
{
    m_lddstate=aggregate;
    spot::kripke_succ_iterator::recycle ( cond );
}

ModelCheckBaseMT * SogKripkeIteratorTh::m_builder;
spot::bdd_dict_ptr* SogKripkeIteratorTh::m_dict_ptr;
LDDState SogKripkeIteratorTh::m_deadlock;
//LDDState SogKripkeIteratorTh::m_div;
