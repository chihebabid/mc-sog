#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/kripke/kripke.hh>
#include <spot/twa/bdddict.hh>


#include "SogKripkeIteratorTh.h"
#include "SogKripkeStateTh.h"

#include "SogKripkeTh.h"
#include <map>
#include "SylvanWrapper.h"
using namespace spot;
SogKripkeTh::SogKripkeTh(const bdd_dict_ptr &dict_ptr,ModelCheckBaseMT *builder): spot::kripke(dict_ptr),m_builder(builder)
{
    SogKripkeIteratorTh::m_builder=builder;
    SogKripkeStateTh::m_builder=builder;
    SogKripkeIteratorTh::m_dict_ptr=&dict_ptr;
    SogKripkeIteratorTh::m_deadlock.setLDDValue(1);
    SogKripkeIteratorTh::m_deadlock.setVisited();
    SogKripkeIteratorTh::m_deadlock.setCompletedSucc();
    /*SogKripkeIteratorTh::m_div.setLDDValue(0);
    SogKripkeIteratorTh::m_div.setVisited();
    SogKripkeIteratorTh::m_div.setCompletedSucc();
    SogKripkeIteratorTh::m_div.Successors.push_back(pair<LDDState*,int>(&SogKripkeIteratorTh::m_div,-1));*/
}

SogKripkeTh::SogKripkeTh(const spot::bdd_dict_ptr& dict_ptr,ModelCheckBaseMT *builder,set<string> &l_transap,set<string> &l_placeap):SogKripkeTh(dict_ptr,builder) {

    for (const auto& it:l_transap) {
        register_ap(it);
    }
    for (const auto & it: l_placeap)
        register_ap(it);

}


state* SogKripkeTh::get_init_state() const {
   LDDState *ss=m_builder->getInitialMetaState();   
    return new SogKripkeStateTh(ss);

}
// Allows to print state label representing its id
std::string SogKripkeTh::format_state(const spot::state* s) const
  {
    //cout<<__func__<<endl;
    auto ss = static_cast<const SogKripkeStateTh*>(s);
    std::ostringstream out;
    out << "( " << ss->getLDDState()->getLDDValue() <<  ")";

    return out.str();
  }

SogKripkeIteratorTh* SogKripkeTh::succ_iter(const spot::state* s) const {
    auto ss = static_cast<const SogKripkeStateTh*>(s);
    LDDState *aggregate=ss->getLDDState();
    bdd cond = state_condition(ss);
    return new SogKripkeIteratorTh(aggregate,cond);
}

bdd SogKripkeTh::state_condition(const spot::state* s) const
  {
    auto ss = static_cast<const SogKripkeStateTh*>(s);
	vector<uint16_t> marked_place = ss->getLDDState()->getMarkedPlaces(m_builder->getPlaceProposition());
#ifdef TESTENABLE
    SylvanWrapper::m_Size+=SylvanWrapper::getMarksCount(ss->getLDDState()->getLDDValue());
      SylvanWrapper::m_agg++;
    cout<<"Expolored states : "<<SylvanWrapper::m_Size<<" , Explored agg : "<<SylvanWrapper::m_agg<<" , Avg per agg : " <<SylvanWrapper::m_Size/SylvanWrapper::m_agg<<endl;
#endif
    bdd result=bddtrue;
    // Marked places
    for (const auto& it:marked_place) {
        string name=string(m_builder->getPlace(it));
        spot::formula f=spot::formula::ap(name);
        result&=bdd_ithvar((dict_->var_map.find(f))->second);
    }
    vector<uint16_t> unmarked_place=ss->getLDDState()->getUnmarkedPlaces(m_builder->getPlaceProposition());
    for (const auto& it: unmarked_place) {
        string name=string(m_builder->getPlace(it));
        spot::formula f=spot::formula::ap(name);
        result&=!bdd_ithvar((dict_->var_map.find(f))->second);
    }
  return result;
  }

SogKripkeTh::~SogKripkeTh()
{
    //dtor
}

