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
    static ModelCheckBaseMT * m_builder;
    static spot::bdd_dict_ptr* m_dict_ptr;
  //  sog_succ_iterator(const RdPBDD& pn, const SogKripkeState& s, const bdd& c);
    HybridKripkeIterator(const HybridKripkeState &kstate, bdd cnd);
    virtual ~HybridKripkeIterator();
    bool first() override;
    bool next() override;
    bool done() const override;
    HybridKripkeState* dst() const override;
    bdd cond() const override final;

    HybridKripkeState* current_state() const;

    void recycle(LDDState *aggregate, bdd cond);

   
    std::string format_transition() const;

private:
    HybridKripkeState *m_current_state;
    LDDState * m_lddstate;
    vector<pair<LDDState*, int>> m_lsucc;
    unsigned int m_current_edge=0;
};


#endif // HybridKRIPKEITERATOR_H_INCLUDED
