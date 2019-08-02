#ifndef HybridKRIPKEITERATORTH_H_INCLUDED
#define HybridKRIPKEITERATORTH_H_INCLUDED
#include "HybridKripkeState.h"
#include "ModelCheckBaseMT.h"
#include <spot/kripke/kripke.hh>
// Iterator for a SOG graph
typedef struct {
    char id[17];
    int16_t transition;
    uint16_t pcontainer;
} succ_t;
class HybridKripkeIterator : public spot::kripke_succ_iterator
{
public:
    static LDDState m_deadlock;
    static LDDState m_div;
    static NewNet * m_net;
    static spot::bdd_dict_ptr* m_dict_ptr;
  //  sog_succ_iterator(const RdPBDD& pn, const SogKripkeState& s, const bdd& c);
    HybridKripkeIterator(HybridKripkeState &kstate, bdd cnd);
    virtual ~HybridKripkeIterator();
    bool first() override;
    bool next() override;
    bool done() const override;
    HybridKripkeState* dst() const override;
    bdd cond() const override final;

   // HybridKripkeState* current_state() const;

    void recycle(HybridKripkeState *aggregate, bdd cond);

   
    std::string format_transition() const;
    

private:
    HybridKripkeState *m_current_state;
    
    
    unsigned int m_current_edge=0;
    vector<succ_t> m_succ;
};


#endif // HybridKRIPKEITERATOR_H_INCLUDED
