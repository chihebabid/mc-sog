#ifndef MODELCHECKERTHV2_H
#define MODELCHECKERTHV2_H
#include "ModelCheckBaseMT.h"
#include "stacksafe.h"
#include <atomic>
#include <thread>
#include <mutex>
typedef pair<LDDState *, int> couple_th;
typedef stack<pair<LDDState *,int>> pile_t;

class ModelCheckerThV2 : public ModelCheckBaseMT
{
public:
    ModelCheckerThV2(const NewNet &R,int nbThread);
    ~ModelCheckerThV2();
    LDDState * getInitialMetaState();
    void buildSucc(LDDState *agregate);
    static void threadHandler(void *context);
    void Compute_successors();
    void ComputeTh_Succ();
private:
    void preConfigure();
    
      
    StackSafe<Pair> m_common_stack;
    bool m_started=false;
    
    int m_id_thread;
    std::mutex m_mutex,m_graph_mutex,m_gc_mutex,m_supervise_gc_mutex;    
    pthread_barrier_t m_barrier_builder;
    atomic<uint32_t> m_gc,m_terminaison; //
    volatile bool m_finish=false;
    bool m_finish_initial=false;
    pthread_mutex_t m_mutex_stack[128];
    thread* m_list_thread[128];    
};
#endif
