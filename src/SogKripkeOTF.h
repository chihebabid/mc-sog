#ifndef SOGKRIPKEOTF_H_INCLUDED
#define SOGKRIPKEOTF_H_INCLUDED
#include "LDDGraph.h"
#include "SogKripkeIteratorOTF.h"
#include <spot/kripke/kripke.hh>
#include <LDDGraph.h>
#include <NewNet.h>


class SogKripkeOTF: public spot::kripke {
    public:

        //SogKripkeOTF(const spot::bdd_dict_ptr& dict_ptr,LDDGraph *sog);
        SogKripkeOTF(const spot::bdd_dict_ptr& dict_ptr,ModelCheckLace *builder,set<string> &l_transap,set<string> &l_placeap);
        virtual ~SogKripkeOTF();
        spot::state* get_init_state() const;
        SogKripkeIteratorOTF* succ_iter(const spot::state* s) const override;
        std::string format_state(const spot::state* s) const override;
        bdd state_condition(const spot::state* s) const override;

          /// \brief Get the graph associated to the automaton.
       /* const LDDGraph& get_graph() const {
        return *m_sog;
  }*/
        ModelCheckLace *m_builder;

    protected:

    private:
    std::map<int, int> place_prop;
    //LDDGraph* m_sog;
};

#endif // SOGKRIPKE_H_INCLUDED
