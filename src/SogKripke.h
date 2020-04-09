#ifndef SOGKRIPKE_H_INCLUDED
#define SOGKRIPKE_H_INCLUDED
#include "LDDGraph.h"
#include "SogKripkeIterator.h"
#include <spot/kripke/kripke.hh>
class SogKripke: public spot::kripke {
    public:
        SogKripke ( const spot::bdd_dict_ptr& dict_ptr,LDDGraph *sog );
        SogKripke ( const spot::bdd_dict_ptr& dict_ptr,LDDGraph *sog,set<string> &l_transap,set<string> &l_placeap );
        virtual ~SogKripke();
        spot::state* get_init_state() const;
        SogKripkeIterator* succ_iter ( const spot::state* s ) const override;
        std::string format_state ( const spot::state* s ) const override;
        bdd state_condition ( const spot::state* s ) const override;


    protected:

    private:
        std::map<int, int> place_prop;
        LDDGraph* m_sog;
    };

#endif // SOGKRIPKE_H_INCLUDED
