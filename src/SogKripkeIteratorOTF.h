#ifndef SOGKRIPKEITERATOROTF_H_INCLUDED
#define SOGKRIPKEITERATOROTF_H_INCLUDED
#include "SogKripkeStateOTF.h"
#include "LDDGraph.h"
#include <spot/kripke/kripke.hh>
// Iterator for a SOG graph
class SogKripkeIteratorOTF : public spot::kripke_succ_iterator
{
public:

  //  static LDDGraph * m_graph;
    static spot::bdd_dict_ptr* m_dict_ptr;
  //  sog_succ_iterator(const RdPBDD& pn, const SogKripkeState& s, const bdd& c);
    SogKripkeIteratorOTF(const LDDGraph& m_gr, const SogKripkeStateOTF& lddstate, bdd cnd);
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
    //const threadSOG& pn_sog; ///< The petri net.
    MDD from; ///< The source state.
    bool dead; ///< The source div attribut.
    bool div; ///< The source div attribut.
   // bdd cond; ///< The condition which must label all successors.

    std::set<int>::const_iterator it;
    /// \brief Designate the associated Petri net.
   // const SogKripkeOTF& pn_sog;

    /// \brief Point to the marking for which we iterate.
    //MDD mark;
    // Associated SOG graph

    LDDState * m_lddstate;
    LDDGraph *m_sog;
    vector<pair<SogKripkeStateOTF, int>> m_lsucc;
    unsigned int m_current_edge=0;
    LDDGraph * SogKripkeIteratorOTF::m_graph;
   // spot::bdd_dict_ptr* SogKripkeIteratorOTF::m_dict_ptr;
};


#endif // SOGKRIPKEITERATOR_H_INCLUDED
