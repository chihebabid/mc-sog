#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/kripke/kripke.hh>
#include <spot/twa/bdddict.hh>


#include "SogKripkeIterator.h"
#include "SogKripkeState.h"

#include "SogKripke.h"
#include <map>
using namespace spot;
SogKripke::SogKripke(const bdd_dict_ptr &dict_ptr,LDDGraph *sog): spot::kripke(dict_ptr),m_sog(sog)
{
    SogKripkeIterator::m_graph=sog;
    SogKripkeIterator::m_dict_ptr=&dict_ptr;
}

SogKripke::SogKripke(const spot::bdd_dict_ptr& dict_ptr,LDDGraph *sog,set<string> &l_transap,set<string> &l_placeap):SogKripke(dict_ptr,sog) {
    for (auto it=l_transap.begin();it!=l_transap.end();it++) {
        register_ap(*it);
    }
    for (auto it=l_placeap.begin();it!=l_placeap.end();it++)
        register_ap(*it);
}


state* SogKripke::get_init_state() const {
    //cout<<"Initial state given...\n";
    return new SogKripkeState(m_sog->getInitialState());//new SpotSogState();

}
// Allows to print state label representing its id
std::string SogKripke::format_state(const spot::state* s) const
  {
    auto ss = static_cast<const SogKripkeState*>(s);
    std::ostringstream out;
    out << "( " << ss->getLDDState()->getLDDValue() <<  ")";
    cout << "( " << ss->getLDDState()->getLDDValue() <<  ")";
    return out.str();
  }

SogKripkeIterator* SogKripke::succ_iter(const spot::state* s) const {

    auto ss = static_cast<const SogKripkeState*>(s);
   //////////////////////////////////////////////

    return new SogKripkeIterator(ss->getLDDState(),state_condition(ss));//,b);//s state_condition(ss));
}

bdd SogKripke::state_condition(const spot::state* s) const
  {

    auto ss = static_cast<const SogKripkeState*>(s);
    vector<int> marked_place=ss->getLDDState()->getMarkedPlaces(m_sog->getConstructor()->getPlaceProposition());


    bdd result=bddtrue;
    for (auto it=marked_place.begin();it!=marked_place.end();it++) {
    string name=m_sog->getPlace(*it);
    //cout<<"Place name marked : "<<"  "<<name<<endl;
    spot::formula f=spot::formula::ap(name);
    result=bdd_ithvar((dict_->var_map.find(f))->second);
    cout<<"verified ldd : "<<ss->getLDDState()->getLDDValue()<<"Place name marked : "<<name<<endl;
    }

  return result;
  }


SogKripke::~SogKripke()
{
    //dtor
}

