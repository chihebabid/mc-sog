#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/kripke/kripke.hh>
#include <spot/twa/bdddict.hh>


#include "HybridKripkeIterator.h"
#include "HybridKripkeState.h"

#include "HybridKripke.h"
#include <map>
#define TAG_FINISH 2
#define TAG_INITIAL 3
#define TAG_ACK_INITIAL 8
using namespace spot;

HybridKripke::HybridKripke(const bdd_dict_ptr &dict_ptr): spot::kripke(dict_ptr)
{
    cout<<__func__<<endl;
    HybridKripkeIterator::m_dict_ptr=&dict_ptr;
    /*HybridKripkeIterator::m_deadlock.setLDDValue(1);
    HybridKripkeIterator::m_deadlock.setVisited();
    HybridKripkeIterator::m_deadlock.setCompletedSucc();
    HybridKripkeIterator::m_div.setLDDValue(0);
    HybridKripkeIterator::m_div.setVisited();
    HybridKripkeIterator::m_div.setCompletedSucc();
    HybridKripkeIterator::m_div.Successors.push_back(pair<LDDState*,int>(&HybridKripkeIterator::m_div,-1));*/
}

HybridKripke::HybridKripke(const spot::bdd_dict_ptr& dict_ptr,set<string> &l_transap,set<string> &l_placeap,NewNet &net_):HybridKripke(dict_ptr) {
    m_net=&net_;
    HybridKripkeIterator::m_net=&net_;
    for (auto it=l_transap.begin();it!=l_transap.end();it++) {
        register_ap(*it);
    }
    
    for (auto it=l_placeap.begin();it!=l_placeap.end();it++)
        register_ap(*it);
    

}


state* HybridKripke::get_init_state() const {
  
    
    int v;
    MPI_Send( &v, 1, MPI_INT, 0, TAG_INITIAL, MPI_COMM_WORLD); 
     char message[23];
    MPI_Status status; int nbytes;
    MPI_Probe(MPI_ANY_SOURCE, TAG_ACK_INITIAL, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_BYTE, &v);
    MPI_Recv(message, 22, MPI_BYTE,MPI_ANY_SOURCE,TAG_ACK_INITIAL,MPI_COMM_WORLD, &status);
    char id[16];
    memcpy(id,message,16);
    
    uint16_t p_container;
    memcpy(&p_container,message+16,2);    
    return new HybridKripkeState(id,p_container);    

}
// Allows to print state label representing its id
std::string HybridKripke::format_state(const spot::state* s) const
  {
    cout<<__func__<<endl;
    auto ss = static_cast<const HybridKripkeState*>(s);
    std::ostringstream out;
    out << "( " << ss->hash() <<  ")";
 
    return out.str();
  }

HybridKripkeIterator* HybridKripke::succ_iter(const spot::state* s) const {
   cout<<__func__<<endl;
    auto ss = static_cast<const HybridKripkeState*>(s);
    HybridKripkeState *aggregate=ss;
    bdd cond = state_condition(ss);
    if (iter_cache_)
    {
      auto it = static_cast<HybridKripkeIterator*>(iter_cache_);
      iter_cache_ = nullptr;    // empty the cache
      it->recycle(ss, cond);
      return it;
    }
  return new HybridKripkeIterator(*aggregate,cond);

    
    
    
/*    auto ss = static_cast<const SogKripkeStateTh*>(s);
   //////////////////////////////////////////////

    return new SogKripkeIteratorTh(ss->getLDDState(),state_condition(ss));//,b);//s state_condition(ss));*/
}

bdd HybridKripke::state_condition(const spot::state* s) const
  {
   
    auto ss = static_cast<const HybridKripkeState*>(s);
    set<uint16_t> marked_place=ss->getMarkedPlaces();


    bdd result=bddtrue;
     
    for (auto it=marked_place.begin();it!=marked_place.end();it++) {
        string name=m_net->getPlaceName(*it);
        spot::formula f=spot::formula::ap(name);
        result&=bdd_ithvar((dict_->var_map.find(f))->second);
    }
    set<uint16_t> unmarked_place=ss->getUnmarkedPlaces();
    //for (auto it=unmarked_place.begin();it!=unmarked_place.end();it++) {
    /*string name=m_builder->getPlace(*it);
    spot::formula f=spot::formula::ap(name);
    result&=!bdd_ithvar((dict_->var_map.find(f))->second);*/
    //}

  return result;
  }


HybridKripke::~HybridKripke()
{
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int i,v;
    for (i = 0; i < world_size-1; i++) {      
        MPI_Send( &v, 1, MPI_INT, i, TAG_FINISH, MPI_COMM_WORLD);       
    } 
   
}

