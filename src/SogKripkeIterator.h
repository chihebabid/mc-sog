#ifndef SOGKRIPKEITERATOR_H_INCLUDED
#define SOGKRIPKEITERATOR_H_INCLUDED
#include "SogKripkeState.h"
#include "LDDGraph.h"
#include <spot/kripke/kripke.hh>
// Iterator for a SOG graph
class SogKripkeIterator : public spot::kripke_succ_iterator
{
    public:

        static LDDGraph * m_graph;
        static spot::bdd_dict_ptr* m_dict_ptr;
        SogKripkeIterator(const LDDState *lddstate,bdd cond);
        virtual ~SogKripkeIterator();
        bool first() override;
        bool next() override;
        bool done() const override;
        SogKripkeState* dst() const override;
        bdd cond() const override final;
      /*  spot::acc_cond::mark_t acc() const override final;*/

    protected:

    private:
        // Associated SOG graph

        LDDState * m_lddstate;
        vector<pair<LDDState*, int>> m_lsucc;
        unsigned int m_current_edge=0;
};


#endif // SOGKRIPKEITERATOR_H_INCLUDED
