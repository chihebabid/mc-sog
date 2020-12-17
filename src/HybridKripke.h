#ifndef HYBRIDKRIPKETH_H_INCLUDED
#define HYBRIDKRIPKETH_H_INCLUDED
#include "LDDGraph.h"
#include "HybridKripkeIterator.h"
#include <spot/kripke/kripke.hh>
#include <LDDGraph.h>
#include <NewNet.h>


class HybridKripke: public spot::kripke {
    public:
        HybridKripke(const spot::bdd_dict_ptr& dict_ptr);
        HybridKripke(const spot::bdd_dict_ptr& dict_ptr,set<string> &l_transap,set<string> &l_placeap,NewNet &net_);
        virtual ~HybridKripke();
        spot::state* get_init_state() const override;
        HybridKripkeIterator* succ_iter(const spot::state* s) const override;
        std::string format_state(const spot::state* s) const override;
        bdd state_condition(const spot::state* s) const override;
    protected:

    private:
    NewNet *m_net;
};

#endif // SOGKRIPKE_H_INCLUDED
