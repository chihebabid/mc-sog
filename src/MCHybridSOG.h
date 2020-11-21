#ifndef MCHYBRIDSOG_H
#define MCHYBRIDSOG_H

#include <stack>
#include <vector>
#include "NewNet.h"
// #include "MDD.h"
//#include "MDGraph.h"
//#include "bvec.h"
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "LDDGraph.h"
#include "TransSylvan.h"

#include <mpi.h>
#include <sha2.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <cstdlib>  //std::system
#include <sstream>

#include <iostream>
#include <queue>
#include <string>
//#include <boost/serialization/string.hpp>
#include <time.h>
#include <chrono>
#include "CommonSOG.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
// namespace mpi = boost::mpi;

//#define MASTER 0
//#define large 128
extern unsigned int nb_th;
extern int n_tasks, task_id;



typedef pair<string *, unsigned int> MSG;

typedef stack<MSG> pile_msg;

// typedef vector<Trans> vec_trans;
class MCHybridSOG : public CommonSOG
{
public:
    MCHybridSOG(const NewNet &,MPI_Comm &, bool init = false);
    void buildFromNet(int index);
    /// principal functions to construct the SOG
    void computeDSOG(LDDGraph &g);
    virtual ~MCHybridSOG();
    static void *threadHandler(void *context);
    void *doCompute();

protected:
private:
    MPI_Comm m_comm_world;
    void sendSuccToMC();
    void sendPropToMC(size_t pos);
    atomic_bool m_waitingAgregate;// (false);
    atomic_bool m_waitingBuild;
    atomic_bool m_waitingSucc;
    char m_id_md5[16];
    LDDState * m_aggWaiting=nullptr;
    
    
    /// \ hash function
    void get_md5(const string &chaine, unsigned char *md_chaine);
        
    /// minimum charge function for the load balancing between thread
    inline uint8_t minCharge();



    int m_nbmetastate;

    pile m_st[128];
    pile_msg m_msg[128];

    // void DisplayMessage(const char *chaine);

    //        int n_tasks, task_id;

    int m_charge[128];
     int m_init;


    /// Convert a string caracter to an MDD
    MDD decodage_message(const char *chaine);
    /// there is a message to receive?
    void read_message();
    
    /// receive state message
    void read_state_message();
    void send_state_message();

    pthread_t m_list_thread[128];

    atomic<uint8_t> m_id_thread;      
    //pthread_mutex_t m_mutex_stack[128];
    pthread_mutex_t m_spin_stack[128];
    pthread_mutex_t m_spin_msg[128];
   
    pthread_spinlock_t m_spin_md5;
    

    volatile bool m_Terminated = false;
    unsigned long m_size_mess = 0;
    int m_nbsend = 0, m_nbrecv = 0;

    MPI_Status m_status;
    /*set<uint16_t> getUnmarkedPlaces(LDDState *agg);
    set<uint16_t> getMarkedPlaces(LDDState *agg);*/

};

#endif  // MCHybridSOG_H
