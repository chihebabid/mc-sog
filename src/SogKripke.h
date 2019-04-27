#ifndef SOGKRIPKE_H_INCLUDED
#define SOGKRIPKE_H_INCLUDED

class SogKripke : public spot::kripke
{
    public:



        SogKripke(const spot::bdd_dict_ptr& dict_ptr,LDDGraph *sog);
        virtual ~SogKripke();
        spot::state* get_init_state() const;
        SpotSogIterator* succ_iter(const spot::state* s) const override;
        std::string format_state(const spot::state* s) const override;
        MDD state_condition(const spot::state* s) const override;


    protected:

    private:
    std::map<int, int> place_prop;
    LDDGraph* m_sog;
};

#endif // SOGKRIPKE_H_INCLUDED
