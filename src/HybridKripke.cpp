#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/kripke/kripke.hh>
#include <spot/twa/bdddict.hh>


#include "HybridKripkeIterator.h"
#include "HybridKripkeState.h"

#include "HybridKripke.h"
#include <map>
using namespace spot;
HybridKripke::HybridKripke(const bdd_dict_ptr &dict_ptr): spot::kripke(dict_ptr)
{
    
    HybridKripkeIterator::m_dict_ptr=&dict_ptr;
    HybridKripkeIterator::m_deadlock.setLDDValue(1);
    HybridKripkeIterator::m_deadlock.setVisited();
    HybridKripkeIterator::m_deadlock.setCompletedSucc();
    HybridKripkeIterator::m_div.setLDDValue(0);
    HybridKripkeIterator::m_div.setVisited();
    HybridKripkeIterator::m_div.setCompletedSucc();
    HybridKripkeIterator::m_div.Successors.push_back(pair<LDDState*,int>(&HybridKripkeIterator::m_div,-1));
}

HybridKripke::HybridKripke(const spot::bdd_dict_ptr& dict_ptr,set<string> &l_transap,set<string> &l_placeap):HybridKripke(dict_ptr) {

    for (auto it=l_transap.begin();it!=l_transap.end();it++) {
        register_ap(*it);
    }
    for (auto it=l_placeap.begin();it!=l_placeap.end();it++)
        register_ap(*it);

}


state* HybridKripke::get_init_state() const {
    // LDDState *ss=m_builder->getInitialMetaState();   
    
    string id;
    uint16_t p_container;
    return new HybridKripkeState(id,p_container);//new SpotSogState();

}
// Allows to print state label representing its id
std::string HybridKripke::format_state(const spot::state* s) const
  {
    //cout<<__func__<<endl;
    auto ss = static_cast<const HybridKripkeState*>(s);
    std::ostringstream out;
    out << "( " << ss->getLDDState()->getLDDValue() <<  ")";
   // cout << " ( " << ss->getLDDState()->getLDDValue() <<  ")";
    return out.str();
  }

HybridKripkeIterator* HybridKripke::succ_iter(const spot::state* s) const {
   
    auto ss = static_cast<const HybridKripkeState*>(s);
    LDDState *aggregate=ss->getLDDState();
    bdd cond = state_condition(ss);
    if (iter_cache_)
    {
      auto it = static_cast<HybridKripkeIterator*>(iter_cache_);
      iter_cache_ = nullptr;    // empty the cache
      it->recycle(aggregate, cond);
      return it;
    }
  return new HybridKripkeIterator(aggregate,cond);

    
    
    
/*    auto ss = static_cast<const SogKripkeStateTh*>(s);
   //////////////////////////////////////////////

    return new SogKripkeIteratorTh(ss->getLDDState(),state_condition(ss));//,b);//s state_condition(ss));*/
}

bdd HybridKripke::state_condition(const spot::state* s) const
  {

    auto ss = static_cast<const HybridKripkeState*>(s);
    //vector<int> marked_place=ss->getLDDState()->getMarkedPlaces(m_builder->getPlaceProposition());


    bdd result=bddtrue;
    // Marked places
    //for (auto it=marked_place.begin();it!=marked_place.end();it++) {
    // string name=m_builder->getPlace(*it);
    //spot::formula f=spot::formula::ap(name);
    //result&=bdd_ithvar((dict_->var_map.find(f))->second);
   // }
    //vector<int> unmarked_place=ss->getLDDState()->getUnmarkedPlaces(m_builder->getPlaceProposition());
    //for (auto it=unmarked_place.begin();it!=unmarked_place.end();it++) {
    /*string name=m_builder->getPlace(*it);
    spot::formula f=spot::formula::ap(name);
    result&=!bdd_ithvar((dict_->var_map.find(f))->second);*/
    //}

  return result;
  }


HybridKripke::~HybridKripke()
{
    //dtor
}

