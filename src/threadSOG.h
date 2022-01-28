#ifndef THREADSOG_H
#define THREADSOG_H
// #include "RdPBDD.h"
#include <stack>
#include <vector>
#include "NewNet.h"
// #include "MDD.h"
//#include "MDGraph.h"
//#include "bvec.h"
#include <pthread.h>
#include <cstdio>
#include <sys/types.h>
#include <unistd.h>
#include <thread>
#include "LDDGraph.h"
#include "TransSylvan.h"
#include "CommonSOG.h"
#include <atomic>

class threadSOG : public CommonSOG {
    public:
        threadSOG ( const NewNet &, int nbThread=2,bool uselace=false,bool init = false );
        void computeDSOG ( LDDGraph &g,uint8_t&& method );
        void computeSeqSOG ( LDDGraph &g );
        virtual ~threadSOG()=default;
        static void *threadHandler ( void *context,const uint8_t& method);
        void *doCompute();
        void *doComputeCanonized();
        void *doComputePOR();
    protected:
    private:
        int minCharge();
        bool isNotTerminated();
        timespec start, finish;
        pile m_st[128];
        int m_charge[128];
        bool m_terminaison[128];
        int m_min_charge;
        int m_init;
        std::atomic<int> m_id_thread;
        pthread_mutex_t m_mutex;
        pthread_spinlock_t m_spin_stack[128];
        std::thread* m_list_thread[128];
    };

#endif
