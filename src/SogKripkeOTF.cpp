#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/kripke/kripke.hh>
#include <spot/twa/bdddict.hh>


#include "SogKripkeIteratorOTF.h"
#include "SogKripkeStateOTF.h"

#include "SogKripkeOTF.h"
#include <map>
using namespace spot;
SogKripkeOTF::SogKripkeOTF(const bdd_dict_ptr &dict_ptr,ModelCheckLace *builder): spot::kripke(dict_ptr),m_builder(builder)
{
    SogKripkeIteratorOTF::m_builder=builder;
    SogKripkeStateOTF::m_builder=builder;
    SogKripkeIteratorOTF::m_dict_ptr=&dict_ptr;

    //cout<<__func__<<endl;
}

SogKripkeOTF::SogKripkeOTF(const spot::bdd_dict_ptr& dict_ptr,ModelCheckLace *builder,set<string> &l_transap,set<string> &l_placeap):SogKripkeOTF(dict_ptr,builder)
{

    for (auto it=l_transap.begin(); it!=l_transap.end(); it++)
    {
        register_ap(*it);
    }
    for (auto it=l_placeap.begin(); it!=l_placeap.end(); it++)
        register_ap(*it);

}


state* SogKripkeOTF::get_init_state() const
{
    // cout<<__func__<<endl;
    return new SogKripkeStateOTF(m_builder->buildInitialMetaState());//new SpotSogState();

}
// Allows to print state label representing its id
std::string SogKripkeOTF::format_state(const spot::state* s) const
{
    //cout<<__func__<<endl;
    auto ss = static_cast<const SogKripkeStateOTF*>(s);
    std::ostringstream out;
    out << "( " << ss->getLDDState()->getLDDValue() <<  ")";
    return out.str();
}

SogKripkeIteratorOTF* SogKripkeOTF::succ_iter(const spot::state* s) const
{
//  cout<<__func__<<endl;

    auto ss = static_cast<const SogKripkeStateOTF*>(s);
    //////////////////////////////////////////////

    return new SogKripkeIteratorOTF(ss->getLDDState(),state_condition(ss));//,b);//s state_condition(ss));
}

bdd SogKripkeOTF::state_condition(const spot::state* s) const
{

    auto ss = static_cast<const SogKripkeStateOTF*>(s);
    vector<int> marked_place=ss->getLDDState()->getMarkedPlaces(m_builder->getPlaceProposition());


    bdd result=bddtrue;
    // Marked places
    for (auto it=marked_place.begin(); it!=marked_place.end(); it++)
    {
        string name=m_builder->getPlace(*it);
        spot::formula f=spot::formula::ap(name);
        result&=bdd_ithvar((dict_->var_map.find(f))->second);
    }
    vector<int> unmarked_place=ss->getLDDState()->getUnmarkedPlaces(m_builder->getPlaceProposition());
    for (auto it=unmarked_place.begin(); it!=unmarked_place.end(); it++)
    {
        string name=m_builder->getPlace(*it);
        spot::formula f=spot::formula::ap(name);
        result&=!bdd_ithvar((dict_->var_map.find(f))->second);
    }

    return result;
}


SogKripkeOTF::~SogKripkeOTF()
{
    //dtor
}

