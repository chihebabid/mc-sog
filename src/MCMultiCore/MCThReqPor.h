#ifndef MCTHREQPOR_H
#define MCTHREQPOR_H
#include "ModelCheckBaseMT.h"
#include <atomic>
#include <mutex>
#include <thread>
#include "misc/SafeDequeue.h"

typedef pair<LDDState *, int> couple_th;
//typedef stack<pair<LDDState *,int>> pile_t;
class MCThReqPor : public ModelCheckBaseMT {
public:
    MCThReqPor(NewNet &R,int nbThread);
    ~MCThReqPor() override;
    void buildSucc(LDDState *agregate)  override;
    static void *threadHandler(void *context);
    void *Compute_successors();
    void ComputeTh_Succ();
    LDDState * getInitialMetaState() override;
private:
    void preConfigure() override;
    SafeDequeue<couple_th> m_common_stack;
    atomic<uint8_t> m_id_thread;
    thread* m_list_thread[128];
    mutable std::mutex m_mutexBuildCompleted;
    std::condition_variable m_datacondBuildCompleted;
    pthread_barrier_t m_barrier_threads;
};
#endif

