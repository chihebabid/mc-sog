#ifndef HybridKRIPKEITERATORTH_H_INCLUDED
#define HybridKRIPKEITERATORTH_H_INCLUDED
#include "HybridKripkeState.h"
#include "ModelCheckBaseMT.h"
#include <spot/kripke/kripke.hh>
// Iterator for a SOG graph

class HybridKripkeIterator : public spot::kripke_succ_iterator
{
public:
    static LDDState m_deadlock;
    static LDDState m_div;
    static NewNet * m_net;
    static spot::bdd_dict_ptr* m_dict_ptr;
    
  
    HybridKripkeIterator(HybridKripkeState &st, bdd cnd);
    virtual ~HybridKripkeIterator();
    bool first() override;
    bool next() override;
    bool done() const override;
    HybridKripkeState* dst() const override;
    bdd cond() const override final;

   // HybridKripkeState* current_state() const;

    void recycle(HybridKripkeState &st, bdd cond);   
    std::string format_transition() const;
    

private:
    unsigned int m_current_edge=0;
    HybridKripkeState *m_current_state;
    
};


#endif // HybridKRIPKEITERATOR_H_INCLUDED
