#ifndef DISTRIBUTEDSOG_H
#define DISTRIBUTEDSOG_H
// #include "RdPBDD.h"
#include <stack>
#include <vector>
#include "Net.hpp"
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
//#include <boost/mpi.hpp>
#include <iostream>
#include <string>
//#include <boost/serialization/string.hpp>
//#include <boost/mpi.hpp>
#include <iostream>
#include <queue>
#include <string>
//#include <boost/serialization/string.hpp>
#include <time.h>
#include <chrono>
// namespace mpi = boost::mpi;

#define MASTER 0
#define large 128

extern int n_tasks, task_id;

typedef pair<LDDState *, MDD> couple;
typedef pair<couple, Set> Pair;
typedef stack<Pair> pile;

// typedef vector<Trans> vec_trans;
class DistributedSOG {
 public:
  DistributedSOG(const net &, int BOUND = 32, bool init = false);
  void buildFromNet(int index);
  void computeDSOG(LDDGraph &g);
  void BuildInitialState(LDDState *m_state, MDD mdd);
  void computeSeqSOG(LDDGraph &g);
  virtual ~DistributedSOG();
  static void printhandler(ostream &o, int var);
  static void *threadHandler(void *context);
  void *doCompute();
  void NonEmpty();
  void *MasterProcess();

 protected:
 private:
  LDDGraph *m_graph;
  MDD LDDAccessible_epsilon(MDD *m);
  MDD Accessible_epsilon(MDD From);
  Set firable_obs(MDD State);
  MDD get_successorMDD(MDD From, int t);
  int minCharge();
  bool isNotTerminated();
  int Terminate();
  void strcpySHA(char *dest, const char *source);
  char *concat_string(const char *s1, int longueur1, const char *s2,
                      int longueur2, char *dest);
  void sha256(LDDState *state, char output[65]);
  //-----Original defined as members
  vector<class Transition> transitions;
  Set Observable;
  Set NonObservable;
  map<string, int> transitionName;
  Set InterfaceTrans;
  Set Formula_Trans;
  unsigned int Nb_places;
  MDD M0;
  LDDState m_M0;
  MDD currentvar;
  // vector<TransSylvan> m_relation;
  //        vec_trans m_tb_relation[16];

  //-----------------
  vector<TransSylvan> m_tb_relation;
  int m_NbIt;
  int m_itext, m_itint;
  int m_MaxIntBdd;
  MDD *m_TabMeta;
  int m_nbmetastate;
  double m_old_size;

  int nb_failed;
  int nb_success;

  pile m_st;
  void MarquageString(char *dest, const char *source);

  int tag = 0;
  int m_charge;

  MPI_Status m_status;

  MPI_Request m_request;

  net m_net;
  int m_bound, m_init;
  int tempCanoniz = 0;
  int tempS = 0;
  int tempR = 0;
  long m_sizeMsg = 0;
  long m_sizeMsgCanoniz = 0;

  std::queue<char *> m_queue;  // empty queue

  int nbPlaces;
  void convert_wholemdd_string(MDD cmark, char **result, unsigned int &length);
  MDD decodage_message(char *chaine);
  void read_state_message();
  int nbsend = 0, nbrecv = 0;
  int total_nb_send = 0, total_nb_recv = 0;
  Set fire;
  MDD Canonize(MDD s, unsigned int level);
  MDD ImageForward(MDD From);

  // named_mutex m_initial_mtx{open_or_create, "initial"};
};

#endif  // DISTRIBUTEDSOG_H
