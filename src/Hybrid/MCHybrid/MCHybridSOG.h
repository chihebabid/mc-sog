#ifndef MCHYBRIDSOG_H
#define MCHYBRIDSOG_H

#include <stack>
#include <vector>
#include "NewNet.h"
#include <pthread.h>
#include <cstdio>
#include <sys/types.h>
#include <unistd.h>
#include "LDDGraph.h"
#include "TransSylvan.h"

#include <mpi.h>
#include <misc/sha2.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <cstdlib>  //std::system
#include <sstream>

#include <iostream>
#include <queue>
#include <string>
#include <ctime>
#include <chrono>
#include "CommonSOG.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "misc/SafeDequeue.h"


//#define MASTER 0
//#define large 128
extern unsigned int nb_th;
extern int n_tasks, task_id;



typedef pair<string *, unsigned int> MSG;

typedef stack<MSG> pile_msg;
enum class state {waitInitial,waitingBuild,waitingSucc};
// typedef vector<Trans> vec_trans;
class MCHybridSOG : public CommonSOG
{
public:
    MCHybridSOG(const NewNet &, MPI_Comm &, bool init = false);
    /// principal functions to construct the SOG
    void computeDSOG(LDDGraph &g);
    ~MCHybridSOG() override;
    static void *threadHandler(void *context);
    virtual void *doCompute();
protected:
    std::condition_variable m_condStack;
    mutable std::mutex m_mutexCond;
    MPI_Comm m_comm_world;
    void sendSuccToMC();
    void sendPropToMC(size_t pos);
    atomic<bool> m_waitingAgregate;// (false);
    atomic<state> m_waitingBuildSucc;
    char m_id_md5[16];
    LDDState * m_aggWaiting=nullptr;
    int m_nbmetastate;
    SafeDequeue<Pair> m_common_stack;
    SafeDequeue<MSG> m_received_msg;
    SafeDequeue<MSG> m_tosend_msg;

    int m_init;
    /// Convert a string caracter to an MDD
    MDD decodage_message(const char *chaine);
    /// there is a message to receive?
    void read_message();
    
    /// receive state message
    void send_state_message();
    thread* m_list_thread[128];
    atomic<uint8_t> m_id_thread;
    volatile bool m_Terminated = false;
    unsigned long m_size_mess = 0;
    int m_nbsend = 0, m_nbrecv = 0;
    MPI_Status m_status;

};

#endif  // MCHybridSOG_H
