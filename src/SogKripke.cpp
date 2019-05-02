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
/*spot::bdd_dict *p=dict_ptr.get();
cout<<"Taille du dictionnaire :"<<p->var_map.size()<<endl;

map<spot::formula,int>::iterator  i=(p->var_map).begin();

while (i!=(p->var_map).end()) {
    cout<<" test :"<<(*i).first.ap_name()<<endl;
std::ostringstream stream;
     stream << (*i).first;
     std::string str =  stream.str();
  cout<<" formule 1: "<<str<<endl;
  cout<<" formule 2: "<<str<<endl;
  i++;
}*/
//spot::bdd_format_formula(dict_ptr,"test");
//map<bdd,spot::ltl::formula>.iterator it=p->vf_map.begin();

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

  /* MDD state_condition(const spot::state* s) const override
  {
    auto ss = static_cast<const SogKripkeState*>(s);

  MDD res = lddmc_true;
  std::map<int, int>::const_iterator it;
  for (it = place_prop.begin(); it != place_prop.end(); ++it)
    if (m_sog->is_marked(it->first, m))

  return res;
  }*/


SogKripke::~SogKripke()
{
    //dtor
}

