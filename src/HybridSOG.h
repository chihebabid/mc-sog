#ifndef HYBRIDSOG_H
#define HYBRIDSOG_H

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

#include <iostream>
#include <queue>
#include <string>
//#include <boost/serialization/string.hpp>
#include <time.h>
#include <chrono>
// namespace mpi = boost::mpi;

//#define MASTER 0
//#define large 128
extern unsigned int nb_th;
extern int n_tasks, task_id;

typedef pair<LDDState *, MDD> couple;
typedef pair<couple, Set> Pair;
typedef stack<Pair> pile;

typedef pair<string *, unsigned int> MSG;

typedef stack<MSG> pile_msg;

// typedef vector<Trans> vec_trans;
class HybridSOG {
 public:
  HybridSOG(const net &, int BOUND = 32, bool init = false);
  void buildFromNet(int index);
  /// principal functions to construct the SOG
  void computeDSOG(LDDGraph &g);
  virtual ~HybridSOG();
  static void printhandler(ostream &o, int var);
  static void *threadHandler(void *context);
  void *doCompute();

 protected:
 private:
  /// \ hash function
  void get_md5(const string &chaine, unsigned char *md_chaine);
  /// Termination Detection functions
  inline void ReceiveTermSignal();
  inline void TermSendMsg();
  inline void startTermDetectionByMaster();
  inline void TermReceivedMsg();
  LDDGraph *m_graph;
  /// saturation
  inline MDD Accessible_epsilon(MDD From);
  inline MDD ImageForward(MDD From);
  /// Observable Transition
  Set firable_obs(MDD State);
  ///
  MDD get_successor(MDD From, int t);

  /// minimum charge function for the load balancing between thread
  inline int minCharge();
  inline bool isNotTerminated();
  /// Copie string of caracter
  void strcpySHA(unsigned char *dest, const unsigned char *source);

  vector<class Transition> transitions;
  Set Observable;
  Set NonObservable;
  map<string, int> transitionName;
  Set InterfaceTrans;
  Set Formula_Trans;
  unsigned int m_nbPlaces;
  MDD M0;
  LDDState m_M0;

  //-----------------
  vector<TransSylvan> m_tb_relation;
  int m_NbIt;
  int m_itext, m_itint;
  int m_MaxIntBdd;

  int m_nbmetastate;

  pile m_st[128];
  pile_msg m_msg[128];

  // void DisplayMessage(const char *chaine);

  //        int n_tasks, task_id;

  int m_charge[128];

  net m_net;
  int m_bound, m_init;

  /// Convert an MDD to a string caracter (for the send)
  void convert_wholemdd_stringcpp(MDD cmark, string &chaine);
  /// Convert a string caracter to an MDD
  MDD decodage_message(const char *chaine);
  /// there is a message to receive?
  void read_message();
  /// receive termination message
  void read_termination();
  void AbortTerm();
  /// receive state message
  void read_state_message();
  /// send state message
  void send_state_message();
  /// canonicalization of an aggregate represented by an MDD
  MDD Canonize(MDD s, unsigned int level);

  int m_nb_thread;
  pthread_t m_list_thread[128];

  int m_id_thread;
  /// Mutex declaration
  pthread_mutex_t m_mutex;
  pthread_mutex_t m_graph_mutex;
  pthread_mutex_t m_gc_mutex;
  pthread_mutex_t m_supervise_gc_mutex;
  unsigned int m_gc;

  pthread_mutex_t m_mutex_stack[128];
  pthread_mutex_t m_spin_stack[128];
  pthread_mutex_t m_spin_msg[128];
  // pthread_mutex_t m_spin_charge;
  pthread_spinlock_t m_spin_md5;
  pthread_mutex_t m_spin_working;
  unsigned int m_working = 0;

  // unsigned int m_count_thread_working;
  bool m_Terminated = false;
  unsigned long m_size_mess = 0;
  int m_nbsend = 0, m_nbrecv = 0;
  int m_total_nb_send = 0, m_total_nb_recv = 0;

  MPI_Status m_status;
  bool m_Terminating = false;

  // named_mutex m_initial_mtx{open_or_create, "initial"};
};

#endif  // HybridSOG_H
