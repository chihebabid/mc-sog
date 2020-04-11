#ifndef MODELCHECKERTH_H
#define MODELCHECKERTH_H
#include "ModelCheckBaseMT.h"
#include <atomic>
typedef pair<LDDState *, int> couple_th;
typedef stack<pair<LDDState *,int>> pile_t;

class ModelCheckerTh : public ModelCheckBaseMT
{
public:
    ModelCheckerTh(const NewNet &R,int nbThread);
    ~ModelCheckerTh();
    LDDState * getInitialMetaState();
    void buildSucc(LDDState *agregate);
    static void *threadHandler(void *context);
    void *Compute_successors();
    void ComputeTh_Succ();
private:
    void preConfigure();
    bool isNotTerminated();
    int minCharge();
    pile m_st[128];
    int m_charge[128];
    bool m_terminaison[128];
    int m_id_thread;
    pthread_mutex_t m_mutex;
    pthread_mutex_t m_graph_mutex;
    pthread_mutex_t m_gc_mutex;
    pthread_mutex_t m_supervise_gc_mutex;

    pthread_barrier_t m_barrier_builder;
     unsigned int m_gc=0; //
    volatile bool m_finish=false;
    bool m_finish_initial=false;
    pthread_mutex_t m_mutex_stack[128];
    pthread_t m_list_thread[128];
    pthread_spinlock_t m_spin_stack[128];
};
#endif
