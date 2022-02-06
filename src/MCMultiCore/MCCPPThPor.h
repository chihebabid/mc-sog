#ifndef MCCPPThPor_H
#define MCCPPThPor_H
#include "ModelCheckBaseMT.h"
#include "misc/stacksafe.h"
#include "misc/SafeDequeue.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
typedef pair<LDDState *, int> couple_th;

/*
 * Multi-threading with C++14 Library
 */
class MCCPPThPor : public ModelCheckBaseMT
{
public:
    MCCPPThPor(const NewNet &R,int nbThread);
    ~MCCPPThPor();
    static void threadHandler(void *context);
    void Compute_successors();
    void ComputeTh_Succ();
private:
    void preConfigure();
    bool hasToProcess() const;

    SafeDequeue<Pair> m_common_stack;       
    atomic<uint8_t> m_id_thread;
    pthread_barrier_t m_barrier_builder;
    std::condition_variable m_condStack;
    std::mutex m_mutexStack;
    thread* m_list_thread[128];    
};
#endif
