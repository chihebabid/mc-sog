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

