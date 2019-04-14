#ifndef SPOTSOGITERATOR_H
#define SPOTSOGITERATOR_H
#include "SpotSogState.h"
#include "LDDGraph.h"
// Iterator for a SOG graph
class SpotSogIterator : public spot::twa_succ_iterator
{
    public:
        static LDDGraph * m_graph;
        static spot::bdd_dict_ptr* m_dict_ptr;
        SpotSogIterator(const LDDState *lddstate);
        virtual ~SpotSogIterator();
        bool first() override;
        bool next() override;
        bool done() const override;
        SpotSogState* dst() const override;
//        const state * 	dst () const override;//
        // a non derived function
 //       std::string format_transition() const;
        bdd cond() const override final;
        spot::acc_cond::mark_t acc() const override final;

    protected:

    private:
        // Associated SOG graph

        LDDState * m_lddstate;
        unsigned int m_current_edge=0;
};

#endif // SPOTSOGITERATOR_H
