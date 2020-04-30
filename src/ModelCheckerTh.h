#ifndef MODELCHECKERTH_H
#define MODELCHECKERTH_H
#include "ModelCheckBaseMT.h"
#include <atomic>
#include <mutex>
typedef pair<LDDState *, int> couple_th;
typedef stack<pair<LDDState *,int>> pile_t;
#define MAXT 64
class ModelCheckerTh : public ModelCheckBaseMT
{
public:
    ModelCheckerTh(const NewNet &R,int nbThread);
    ~ModelCheckerTh();   
    static void *threadHandler(void *context);
    void *Compute_successors();
    void ComputeTh_Succ();
private:
    void preConfigure();
    bool isNotTerminated();
    uint8_t minCharge();
    pile m_st[MAXT];
    int m_charge[MAXT];
    bool m_terminaison[MAXT];
    atomic<uint8_t> m_id_thread;    
    std::mutex m_graph_mutex;
    pthread_mutex_t m_gc_mutex;
    pthread_barrier_t m_barrier_builder;
#ifdef GCENABLE
    atomic<uint8_t> m_gc; 
#endif
    volatile bool m_finish=false;    
    pthread_mutex_t m_mutex_stack[128];
    pthread_t m_list_thread[128];
    pthread_spinlock_t m_spin_stack[128];
};
#endif
