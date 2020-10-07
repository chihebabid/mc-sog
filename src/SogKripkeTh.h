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
        spot::state* get_init_state() const override;
        SogKripkeIteratorTh* succ_iter(const spot::state* s) const override;
        std::string format_state(const spot::state* s) const override;
        bdd state_condition(const spot::state* s) const override;
        ModelCheckBaseMT *m_builder;


};

#endif // SOGKRIPKE_H_INCLUDED
