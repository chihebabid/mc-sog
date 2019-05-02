#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twa/twa.hh>
#include <spot/twa/bdddict.hh>


#include "SpotSogIterator.h"
#include "SpotSogState.h"

#include "SogTwa.h"
#include <map>
using namespace spot;
SogTwa::SogTwa(const bdd_dict_ptr &dict_ptr,LDDGraph *sog): twa(dict_ptr),m_sog(sog)
{
    SpotSogIterator::m_graph=sog;
    SpotSogIterator::m_dict_ptr=&dict_ptr;
}


SogTwa::SogTwa(const spot::bdd_dict_ptr& dict_ptr,LDDGraph *sog,set<string> &l_transap):SogTwa(dict_ptr,sog) {
    for (auto it=l_transap.begin();it!=l_transap.end();it++) {
        register_ap(*it);
    }
}
state* SogTwa::get_init_state() const {
    //cout<<"Initial state given...\n";
    return new SpotSogState(m_sog->getInitialState());//new SpotSogState();

}
// Allows to print state label representing its id
std::string SogTwa::format_state(const spot::state* s) const
  {
    auto ss = static_cast<const SpotSogState*>(s);
    std::ostringstream out;
    out << "( " << ss->getLDDState()->getLDDValue() <<  ")";
    return out.str();
  }

SpotSogIterator* SogTwa::succ_iter(const spot::state* s) const {

    auto ss = static_cast<const SpotSogState*>(s);
    bdd b=bddfalse;
    return new SpotSogIterator(ss->getLDDState());//,b);//s state_condition(ss));
}


SogTwa::~SogTwa()
{
    //dtor
}

