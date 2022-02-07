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
    ModelCheckerTh(NewNet &R,int nbThread);
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
    pthread_barrier_t m_barrier_builder;
    volatile bool m_finish=false;    
    pthread_mutex_t m_mutex_stack[MAXT];
    pthread_t m_list_thread[MAXT];
    pthread_spinlock_t m_spin_stack[MAXT];
};
#endif
