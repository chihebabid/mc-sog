#ifndef SOGKRIPKEITERATORTH_H_INCLUDED
#define SOGKRIPKEITERATORTH_H_INCLUDED
#include "SogKripkeStateTh.h"
#include "ModelCheckBaseMT.h"
#include <spot/kripke/kripke.hh>
// Iterator for a SOG graph
class SogKripkeIteratorTh : public spot::kripke_succ_iterator
{
public:
    static LDDState m_deadlock;
    static LDDState m_div;
    static ModelCheckBaseMT * m_builder;
    static spot::bdd_dict_ptr* m_dict_ptr;
  //  sog_succ_iterator(const RdPBDD& pn, const SogKripkeState& s, const bdd& c);
    SogKripkeIteratorTh(const LDDState* lddstate, bdd cnd);
    virtual ~SogKripkeIteratorTh();
    bool first() override;
    bool next() override;
    bool done() const override;
    SogKripkeStateTh* dst() const override;
    bdd cond() const override final;

    SogKripkeStateTh* current_state() const;

     void recycle(LDDState *aggregate, bdd cond);

    //int current_transition() const;

    //bdd current_acceptance_conditions() const;

    std::string format_transition() const;

private:
    LDDState * m_lddstate;
    vector<pair<LDDState*, int>> m_lsucc;
    unsigned int m_current_edge=0;
};


#endif // SOGKRIPKEITERATOR_H_INCLUDED
