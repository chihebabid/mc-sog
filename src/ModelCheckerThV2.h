#ifndef MODELCHECKERTHV2_H
#define MODELCHECKERTHV2_H
#include "ModelCheckBaseMT.h"
#include "stacksafe.h"
#include "SafeDequeue.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
typedef pair<LDDState *, int> couple_th;
typedef stack<pair<LDDState *,int>> pile_t;

class ModelCheckerThV2 : public ModelCheckBaseMT
{
public:
    ModelCheckerThV2(const NewNet &R,int nbThread);
    ~ModelCheckerThV2();
    static void threadHandler(void *context);
    void Compute_successors();
    void ComputeTh_Succ();
private:
    void preConfigure();
    bool hasToProcess() const;      
    SafeDequeue<Pair> m_common_stack;       
    atomic<uint8_t> m_id_thread;
    std::mutex m_mutex,m_graph_mutex,m_gc_mutex;    
    pthread_barrier_t m_barrier_builder;    
    volatile bool m_finish=false;
    
    std::condition_variable m_condStack;
    std::mutex m_mutexStack;
    thread* m_list_thread[128];    
};
#endif
