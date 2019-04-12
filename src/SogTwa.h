#ifndef SOGTWA_H
#define SOGTWA_H
#include "LDDGraph.h"
#include "SpotSogIterator.h"

class SogTwa : public spot::twa
{
    public:
        SogTwa(const spot::bdd_dict_ptr& dict_ptr,LDDGraph *sog);
        virtual ~SogTwa();
        spot::state* get_init_state() const;
        SpotSogIterator* succ_iter(const spot::state* s) const override;
        std::string format_state(const spot::state* s) const override;

    protected:

    private:
    LDDGraph* m_sog;
};

#endif // SOGTWA_H
