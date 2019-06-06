#ifndef MODELCHECKERTH_H
#define MODELCHECKERTH_H
#include "CommonSOG.h"
typedef pair<LDDState *, int> couple_th;
typedef stack<pair<LDDState *,int>> pile_t;

class ModelCheckerTh : public CommonSOG
{
public:
    ModelCheckerTh(const NewNet &R, int BOUND,int nbThread);
    ~ModelCheckerTh();
    LDDState * buildInitialMetaState();
    void buildSucc(LDDState *agregate);
    static void *threadHandler(void *context);
    void *Compute_successors();
    void ComputeTh_Succ();
private:
    int m_nb_thread;

    pile_t m_st[128];
    int m_id_thread;
    pthread_mutex_t m_mutex;
    pthread_mutex_t m_graph_mutex;
    //pthread_mutex_t m_gc_mutex;
    //pthread_mutex_t m_supervise_gc_mutex;

    pthread_barrier_t m_barrier_threads,m_barrier_builder;
    //unsigned int m_gc;
    bool m_finish=false;

    pthread_mutex_t m_mutex_stack[128];
    pthread_t m_list_thread[128];
};
#endif