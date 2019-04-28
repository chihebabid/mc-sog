#ifndef COMMONSOG_H
#define COMMONSOG_H
#include "LDDGraph.h"
#include "TransSylvan.h"
#include "Net.hpp"
#include <stack>

typedef pair<LDDState *, MDD> couple;
typedef pair<couple, Set> Pair;
typedef stack<Pair> pile;
class CommonSOG
{
    public:
        CommonSOG();
        virtual ~CommonSOG();
        LDDGraph *getGraph() const;
        static void printhandler(ostream &o, int var);
        vector<TransSylvan>* getTBRelation();
        Set * getNonObservable();
        unsigned int getPlacesCount();
    protected:
        net m_net;
        int m_nbPlaces = 0;
        LDDGraph *m_graph;
        vector<TransSylvan> m_tb_relation;
        LDDState m_M0;
        map<string, int> m_transitionName;
        Set m_observable;
        Set m_nonObservable;
        Set InterfaceTrans;
        Set m_place_proposition;

        vector<class Transition> transitions;

        MDD Accessible_epsilon(MDD From);
        Set firable_obs(MDD State);
        MDD get_successor(MDD From, int t);
        MDD ImageForward(MDD From);
        MDD Canonize(MDD s, unsigned int level);
        bool Set_Div(MDD &M) const;
        bool Set_Bloc(MDD &M) const;
        bool is_marked(int p, const MDD & m) const;


    private:
};

#endif // COMMONSOG_H
