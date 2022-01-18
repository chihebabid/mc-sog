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
        virtual void computeDSOG(LDDGraph &g) {  };
    protected:
        void initializeLDD();
        void loadNetFromFile();
        const NewNet* m_net;
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
        MDD get_successor(const MDD& From,const int& t);
        MDD ImageForward(MDD From);
        MDD Canonize(MDD s, unsigned int level);
        bool Set_Div(MDD &M) const;
        bool Set_Bloc(MDD &M) const;
        uint8_t m_nb_thread;
        std::mutex m_graph_mutex,m_gc_mutex;  
        atomic<uint8_t> m_gc;
        volatile bool m_finish=false;
        // Functions for POR
        /*
         * Determine ample set in S for transition
         */
        void AddConflict(const MDD& S,const int &transition,Set& ample);
    private:

};

#endif // COMMONSOG_H
