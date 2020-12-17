#ifndef COMMONSOG_H
#define COMMONSOG_H
#include "LDDState.h"
#include "LDDGraph.h"

#include "TransSylvan.h"
#include "NewNet.h"
#include <stack>
#include <mutex>
#include <atomic>

// #define GCENABLE 0
class LDDState;
typedef pair<LDDState *, MDD> couple;
typedef pair<couple, Set> Pair;
typedef stack<Pair> pile;

class LDDGraph;


class CommonSOG
{
    public:
        CommonSOG();
        virtual ~CommonSOG();
        static LDDGraph *getGraph();

        inline set<uint16_t> & getPlaceProposition() {return m_place_proposition;}
        inline string_view getTransition(int pos) {
            {
                return string_view {m_transitions.at(pos).name};
            }
        }
        static string_view getPlace(int pos);
        void finish() {m_finish=true;}
    protected:
        NewNet* m_net;
        int m_nbPlaces = 0;
        static LDDGraph *m_graph;
        vector<TransSylvan> m_tb_relation;
        MDD m_initialMarking;
        map<string, uint16_t> * m_transitionName;
        map<uint16_t,string> * m_placeName;
        Set m_observable;
        Set m_nonObservable;
        Set InterfaceTrans;
        set<uint16_t> m_place_proposition;
        vector<class Transition> m_transitions;
        MDD Accessible_epsilon(MDD From);
        Set firable_obs(MDD State);
        MDD get_successor(MDD From, int t);
        MDD ImageForward(MDD From);
        MDD Canonize(MDD s, unsigned int level);
        bool Set_Div(MDD &M) const;
        bool Set_Bloc(MDD &M) const;
        uint8_t m_nb_thread;
        std::mutex m_graph_mutex,m_gc_mutex;  
        atomic<uint8_t> m_gc;
        volatile bool m_finish=false;
    //MDD fireTransition(MDD cmark,MDD minus, MDD plus);
    private:

};

#endif // COMMONSOG_H
