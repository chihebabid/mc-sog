#ifndef ModelCheckerCPPThread_H
#define ModelCheckerCPPThread_H
#include "ModelCheckBaseMT.h"
#include "stacksafe.h"
#include "SafeDequeue.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
typedef pair<LDDState *, int> couple_th;
typedef stack<pair<LDDState *,int>> pile_t;
/*
 * Multi-threading with C++14 Library
 */
class ModelCheckerCPPThread : public ModelCheckBaseMT
{
public:
    ModelCheckerCPPThread(const NewNet &R,int nbThread);
    ~ModelCheckerCPPThread();
    static void threadHandler(void *context);
    void Compute_successors();
    void ComputeTh_Succ();
private:
    void preConfigure();
    bool hasToProcess() const;

    SafeDequeue<Pair> m_common_stack;       
    atomic<uint8_t> m_id_thread;
    std::mutex m_mutex; 
    pthread_barrier_t m_barrier_builder;
    std::condition_variable m_condStack;
    std::mutex m_mutexStack;
    thread* m_list_thread[128];    
};
#endif
