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


class CommonSOG {
public:
    CommonSOG();
    virtual ~CommonSOG();

    static LDDGraph *getGraph();

    inline set<uint16_t> &getPlaceProposition() { return m_place_proposition; }

    inline string_view getTransition(int pos) {
        {
            return string_view{m_net->transitions[pos].name};
        }
    }

    static string_view getPlace(int pos);
    void finish() { m_finish = true; }
    virtual void computeDSOG(LDDGraph &g) {};
protected:
    static void initializeLDD();
    void loadNetFromFile();
    NewNet *m_net;
    int m_nbPlaces = 0;
    static LDDGraph *m_graph;
    vector<TransSylvan> m_tb_relation;
    MDD m_initialMarking;
    map<string, uint16_t> *m_transitionName;
    map<uint16_t, string> *m_placeName;
    Set m_nonObservable;
    set<uint16_t> m_place_proposition;
    //vector<class Transition> m_transitions;
    MDD Accessible_epsilon(const MDD& From);
    Set firable_obs(const MDD& State);
    MDD get_successor(const MDD &From, const int &t);
    MDD get_pred(const MDD &From, const int &t);
    MDD ImageForward(const MDD& From);

    MDD Canonize(const MDD& s, unsigned int level);

    bool Set_Div(const MDD &M) const;

    bool Set_Bloc(const MDD &M) const;
    uint8_t m_nb_thread;
    std::mutex m_graph_mutex, m_gc_mutex;
    atomic<uint8_t> m_gc;
    volatile bool m_finish = false;

    /*
    * Saturate
    */
    MDD saturatePOR(const MDD &s, Set& tObs,bool &div,bool &dead);
private:
    /*
     * Compute ample set @ample for state @s
     */
    Set computeAmple(const MDD &s);
    Set computeAmple2(const MDD &s);

    /*
     * Determine ample set in S for transition
     */
    void AddConflict(const MDD &S, const int &transition, Set &ample);
    /*
     * New version
     */
    void AddConflict2(const MDD &S, const int &transition, Set &ample);

};

#endif // COMMONSOG_H
