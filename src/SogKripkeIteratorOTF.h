#ifndef SOGKRIPKEITERATOROTF_H_INCLUDED
#define SOGKRIPKEITERATOROTF_H_INCLUDED
#include "SogKripkeStateOTF.h"
#include "ModelCheckLace.h"
#include <spot/kripke/kripke.hh>
// Iterator for a SOG graph
class SogKripkeIteratorOTF : public spot::kripke_succ_iterator
{
public:

    static ModelCheckLace * m_builder;
    static spot::bdd_dict_ptr* m_dict_ptr;
  //  sog_succ_iterator(const RdPBDD& pn, const SogKripkeState& s, const bdd& c);
    SogKripkeIteratorOTF(const LDDState* lddstate, bdd cnd);
    virtual ~SogKripkeIteratorOTF();
    bool first() override;
    bool next() override;
    bool done() const override;
    SogKripkeStateOTF* dst() const override;
    bdd cond() const override final;

    SogKripkeStateOTF* current_state() const;

    bdd current_condition() const;

    int current_transition() const;

    bdd current_acceptance_conditions() const;

    std::string format_transition() const;

private:
    LDDState * m_lddstate;
    vector<pair<LDDState*, int>> m_lsucc;
    unsigned int m_current_edge=0;
};


#endif // SOGKRIPKEITERATOR_H_INCLUDED
