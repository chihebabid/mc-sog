#ifndef MODELCHECKLACE_H
#define MODELCHECKLACE_H
#include "ModelCheckBaseMT.h"
#include <atomic>
#include <mutex>

typedef pair<LDDState *, int> couple_th;
typedef stack<pair<LDDState *,int>> pile_t;
class ModelCheckThReq : public ModelCheckBaseMT {
public:
    ModelCheckThReq(const NewNet &R,int nbThread);
    ~ModelCheckThReq();

    void buildSucc(LDDState *agregate)  override;
    static void *threadHandler(void *context);
    void *Compute_successors();
    void ComputeTh_Succ();
    LDDState * getInitialMetaState();
private:
    void preConfigure();
    pile_t m_st[128];
    atomic<uint8_t> m_id_thread;
    pthread_mutex_t m_mutex;
    pthread_mutex_t m_graph_mutex;
    pthread_barrier_t m_barrier_threads,m_barrier_builder;
    pthread_t m_list_thread[128];
    volatile atomic<bool> m_finish=false;

};
#endif

