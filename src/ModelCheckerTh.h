#ifndef MODELCHECKERTH_H
#define MODELCHECKERTH_H
#include "CommonSOG.h"
typedef pair<LDDState *, int> couple_th;
typedef stack<pair<LDDState *,int>> pile_t;

class ModelCheckerTh : public CommonSOG
{
public:
    ModelCheckerTh(const NewNet &R, int BOUND,int nbThread);
    LDDState * buildInitialMetaState();
    string getTransition(int pos);
    string getPlace(int pos);
    void buildSucc(LDDState *agregate);
    static void *threadHandler(void *context);
    void *Compute_successors();
    void ComputeTh_Succ();
    int minCharge();
    bool isNotTerminated();


private:
    int m_nb_thread;
    MDD m_initalMarking;
    int m_min_charge;
    pile_t m_st[128];
    int m_charge[128];
    bool m_terminaison[128];
    int m_id_thread;


    pthread_mutex_t m_mutex;
    pthread_mutex_t m_graph_mutex;
    pthread_mutex_t m_gc_mutex;
    pthread_mutex_t m_supervise_gc_mutex;
    unsigned int m_gc;

    pthread_mutex_t m_mutex_stack[128];
    pthread_spinlock_t m_spin_stack[128];
    pthread_t m_list_thread[128];
};
#endif
