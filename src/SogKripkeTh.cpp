#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/kripke/kripke.hh>
#include <spot/twa/bdddict.hh>


#include "SogKripkeIteratorTh.h"
#include "SogKripkeStateTh.h"

#include "SogKripkeTh.h"
#include <map>
using namespace spot;
SogKripkeTh::SogKripkeTh(const bdd_dict_ptr &dict_ptr,ModelCheckerTh *builder): spot::kripke(dict_ptr),m_builder(builder)
{
    SogKripkeIteratorTh::m_builder=builder;
    SogKripkeStateTh::m_builder=builder;
    SogKripkeIteratorTh::m_dict_ptr=&dict_ptr;
    //cout<<__func__<<endl;
}

SogKripkeTh::SogKripkeTh(const spot::bdd_dict_ptr& dict_ptr,ModelCheckerTh *builder,set<string> &l_transap,set<string> &l_placeap):SogKripkeTh(dict_ptr,builder) {

    for (auto it=l_transap.begin();it!=l_transap.end();it++) {
        register_ap(*it);
    }
    for (auto it=l_placeap.begin();it!=l_placeap.end();it++)
        register_ap(*it);

}


state* SogKripkeTh::get_init_state() const {
   // cout<<__func__<<endl;
    return new SogKripkeStateTh(m_builder->buildInitialMetaState());//new SpotSogState();

}
// Allows to print state label representing its id
std::string SogKripkeTh::format_state(const spot::state* s) const
  {
    //cout<<__func__<<endl;
    auto ss = static_cast<const SogKripkeStateTh*>(s);
    std::ostringstream out;
    out << "( " << ss->getLDDState()->getLDDValue() <<  ")";
   // cout << " ( " << ss->getLDDState()->getLDDValue() <<  ")";
    return out.str();
  }

SogKripkeIteratorTh* SogKripkeTh::succ_iter(const spot::state* s) const {
 //  cout<<__func__<<endl;

    auto ss = static_cast<const SogKripkeStateTh*>(s);
   //////////////////////////////////////////////

    return new SogKripkeIteratorTh(ss->getLDDState(),state_condition(ss));//,b);//s state_condition(ss));
}

bdd SogKripkeTh::state_condition(const spot::state* s) const
  {

    auto ss = static_cast<const SogKripkeStateTh*>(s);
    vector<int> marked_place=ss->getLDDState()->getMarkedPlaces(m_builder->getPlaceProposition());


    bdd result=bddtrue;
    // Marked places
    for (auto it=marked_place.begin();it!=marked_place.end();it++) {
    string name=m_builder->getPlace(*it);
    spot::formula f=spot::formula::ap(name);
    result&=bdd_ithvar((dict_->var_map.find(f))->second);
    }
    vector<int> unmarked_place=ss->getLDDState()->getUnmarkedPlaces(m_builder->getPlaceProposition());
    for (auto it=unmarked_place.begin();it!=unmarked_place.end();it++) {
    string name=m_builder->getPlace(*it);
    spot::formula f=spot::formula::ap(name);
    result&=!bdd_ithvar((dict_->var_map.find(f))->second);
    }

  return result;
  }


SogKripkeTh::~SogKripkeTh()
{
    //dtor
}

