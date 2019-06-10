#ifndef SOGKRIPKETH_H_INCLUDED
#define SOGKRIPKETH_H_INCLUDED
#include "LDDGraph.h"
#include "SogKripkeIteratorTh.h"
#include <spot/kripke/kripke.hh>
#include <LDDGraph.h>
#include <NewNet.h>


class SogKripkeTh: public spot::kripke {
    public:

        SogKripkeTh(const spot::bdd_dict_ptr& dict_ptr,ModelCheckBaseMT *builder);
        SogKripkeTh(const spot::bdd_dict_ptr& dict_ptr,ModelCheckBaseMT *builder,set<string> &l_transap,set<string> &l_placeap);
        virtual ~SogKripkeTh();
        spot::state* get_init_state() const;
        SogKripkeIteratorTh* succ_iter(const spot::state* s) const override;
        std::string format_state(const spot::state* s) const override;
        bdd state_condition(const spot::state* s) const override;

          /// \brief Get the graph associated to the automaton.
       /* const LDDGraph& get_graph() const {
        return *m_sog;
  }*/
        ModelCheckerTh *m_builder;

    protected:

    private:
    std::map<int, int> place_prop;
    //LDDGraph* m_sog;
};

#endif // SOGKRIPKE_H_INCLUDED
