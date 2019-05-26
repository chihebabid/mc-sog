#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/kripke/kripke.hh>
#include <spot/twa/bdddict.hh>


#include "SogKripkeIteratorOTF.h"
#include "SogKripkeStateOTF.h"

#include "SogKripkeOTF.h"
#include <map>
using namespace spot;
SogKripkeOTF::SogKripkeOTF(const bdd_dict_ptr &dict_ptr,LDDGraph *sog): spot::kripke(dict_ptr),m_sog(sog)
{
    SogKripkeIteratorOTF::m_graph=sog;
    SogKripkeIteratorOTF::m_dict_ptr=&dict_ptr;
}

SogKripkeOTF::SogKripkeOTF(const spot::bdd_dict_ptr& dict_ptr,LDDGraph *sog,set<string> &l_transap,set<string> &l_placeap):SogKripkeOTF(dict_ptr,sog) {
    for (auto it=l_transap.begin();it!=l_transap.end();it++) {
        register_ap(*it);
    }
    for (auto it=l_placeap.begin();it!=l_placeap.end();it++)
        register_ap(*it);
}


state* SogKripkeOTF::get_init_state() const {
    //cout<<"Initial state given...\n";
    return new SogKripkeStateOTF(m_sog->getInitialState());//new SpotSogState();

}
// Allows to print state label representing its id
std::string SogKripkeOTF::format_state(const spot::state* s) const
  {
    auto ss = static_cast<const SogKripkeStateOTF*>(s);
    std::ostringstream out;
    out << "( " << ss->getLDDState()->getLDDValue() <<  ")";
    cout << "( " << ss->getLDDState()->getLDDValue() <<  ")";
    return out.str();
  }

SogKripkeIteratorOTF* SogKripkeOTF::succ_iter(const spot::state* s) const {

    auto ss = static_cast<const SogKripkeStateOTF*>(s);
   //////////////////////////////////////////////

    return new SogKripkeIteratorOTF(ss->getLDDState(),state_condition(ss));//,b);//s state_condition(ss));
}

bdd SogKripkeOTF::state_condition(const spot::state* s) const
  {

    auto ss = static_cast<const SogKripkeStateOTF*>(s);
    vector<int> marked_place=ss->getLDDState()->getMarkedPlaces(m_sog->getConstructor()->getPlaceProposition());


    bdd result=bddtrue;
    // Marked places
    for (auto it=marked_place.begin();it!=marked_place.end();it++) {
    string name=m_sog->getPlace(*it);
    spot::formula f=spot::formula::ap(name);
    result&=bdd_ithvar((dict_->var_map.find(f))->second);
    }
    vector<int> unmarked_place=ss->getLDDState()->getUnmarkedPlaces(m_sog->getConstructor()->getPlaceProposition());
    for (auto it=unmarked_place.begin();it!=unmarked_place.end();it++) {
    string name=m_sog->getPlace(*it);
    spot::formula f=spot::formula::ap(name);
    result&=!bdd_ithvar((dict_->var_map.find(f))->second);
    }
  return result;
  }


SogKripkeOTF::~SogKripkeOTF()
{
    //dtor
}

