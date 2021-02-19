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
    //cout<<__func__<<endl;
    HybridKripkeIterator::m_dict_ptr=&dict_ptr;
    /*HybridKripkeIterator::m_deadlock.setLDDValue(1);
    HybridKripkeIterator::m_deadlock.setVisited();
    HybridKripkeIterator::m_deadlock.setCompletedSucc();
    HybridKripkeIterator::m_div.setLDDValue(0);
    HybridKripkeIterator::m_div.setVisited();
    HybridKripkeIterator::m_div.setCompletedSucc();
    HybridKripkeIterator::m_div.Successors.push_back(pair<LDDState*,int>(&HybridKripkeIterator::m_div,-1));*/
}

HybridKripke::HybridKripke(const spot::bdd_dict_ptr& dict_ptr, set<string> &l_transap, set<string> &l_placeap, NewNet &net_): HybridKripke(dict_ptr) {
    m_net=&net_;
    HybridKripkeIterator::m_net=&net_;
    for (auto it:l_transap) {
        register_ap(it);
    }
    
    for (auto it:l_placeap)
        register_ap(it);
    

}


state* HybridKripke::get_init_state() const {
    int v;
    MPI_Send( &v, 1, MPI_INT, 0, TAG_INITIAL, MPI_COMM_WORLD); 
     char message[23];
    MPI_Status status;
    MPI_Probe(MPI_ANY_SOURCE, TAG_ACK_INITIAL, MPI_COMM_WORLD, &status);
    MPI_Recv(message, 22, MPI_BYTE,MPI_ANY_SOURCE,TAG_ACK_INITIAL,MPI_COMM_WORLD, &status);
    succ_t* elt=new succ_t;    
    memcpy(elt->id,message,16);   
    memcpy(&elt->pcontainer,message+16,2);    
    elt->_virtual=false;
    return new HybridKripkeState(*elt);    

}
// Allows to print state label representing its id
std::string HybridKripke::format_state(const spot::state* s) const
  {
    //cout<<__func__<<endl;
    auto ss = static_cast<const HybridKripkeState*>(s);
    std::ostringstream out;
    out << "( " << ss->hash() <<  ")"; 
    return out.str();
  }

HybridKripkeIterator* HybridKripke::succ_iter(const spot::state* s) const {
    //cout<<__func__<<endl;
    auto ss = static_cast<const HybridKripkeState*>(s);
    bdd cond = state_condition(ss);
    HybridKripkeState* st=ss;
   if (iter_cache_)
    {
     /* auto it = static_cast<HybridKripkeIterator*>(iter_cache_);
      iter_cache_ = nullptr;    // empty the cache
      it->recycle(*st,cond);
      return it;*/
    }
#ifdef TESTENABLE
    SylvanWrapper::m_agg++;
     SylvanWrapper::m_Size+=ss->getSize();
    cout<<"Expolored states : "<<SylvanWrapper::m_Size<<" , Explored agg : "<<SylvanWrapper::m_agg<<" , Avg per agg : " <<SylvanWrapper::m_Size/SylvanWrapper::m_agg<<endl;
#endif
  return new HybridKripkeIterator(*st,cond);

}

bdd HybridKripke::state_condition(const spot::state* s) const
  {
   
    auto ss = static_cast<const HybridKripkeState*>(s);
    list<uint16_t>* marked_place=ss->getMarkedPlaces();
    bdd result=bddtrue;     
    for (auto it=marked_place->begin();it!=marked_place->end();it++) {
        string name=string(m_net->getPlaceName(*it));
        spot::formula f=spot::formula::ap(name);
        result&=bdd_ithvar((dict_->var_map.find(f))->second);
    }
    list<uint16_t>* unmarked_place=ss->getUnmarkedPlaces();
    for (auto it=unmarked_place->begin();it!=unmarked_place->end();it++) {
        string name=string(m_net->getPlaceName(*it));
        spot::formula f=spot::formula::ap(name);
        result&=!bdd_ithvar((dict_->var_map.find(f))->second);
    }

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

