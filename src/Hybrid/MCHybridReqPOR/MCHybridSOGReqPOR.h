#ifndef MCHybridSOGReqPOR_H
#define MCHybridSOGReqPOR_H

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
#include <cstdlib>
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
#include "../MCHybridReq/MCHybridSOGReq.h"
// namespace mpi = boost::mpi;
//#define MASTER 0
//#define large 128
extern unsigned int nb_th;
extern int n_tasks, task_id;



typedef pair<string *, unsigned int> MSG;

typedef stack<MSG> pile_msg;

class MCHybridSOGReqPOR : public MCHybridSOGReq
{
public:
    MCHybridSOGReqPOR(const NewNet &, MPI_Comm &, bool init = false);
    ~MCHybridSOGReqPOR() override;
    void *doCompute() override;
};

#endif  // MCHybridSOGReqPOR_H
