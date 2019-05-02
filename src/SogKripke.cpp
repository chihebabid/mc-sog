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

SogKripke::SogKripke(const spot::bdd_dict_ptr& dict_ptr,LDDGraph *sog,set<string> &l_transap):SogKripke(dict_ptr,sog) {
    for (auto it=l_transap.begin();it!=l_transap.end();it++) {
        register_ap(*it);
    }
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
    return out.str();
  }

SogKripkeIterator* SogKripke::succ_iter(const spot::state* s) const {

    auto ss = static_cast<const SogKripkeState*>(s);
   //////////////////////////////////////////////
    // Must be changed
    // State condition ????
    ///////////////////////
    bdd b=bddtrue;
    return new SogKripkeIterator(ss->getLDDState(),b);//,b);//s state_condition(ss));
}

bdd SogKripke::state_condition(const spot::state* s) const
  {
    /*auto ss = static_cast<const SogKripkeState*>(s);

  MDD res = lddmc_true;
  std::map<int, int>::const_iterator it;
  for (it = place_prop.begin(); it != place_prop.end(); ++it)
    if (m_sog->is_marked(it->first, m))

  return res;*/
  return bddtrue;
  }


SogKripke::~SogKripke()
{
    //dtor
}

