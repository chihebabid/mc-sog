#ifndef THREADSOG_H
#define THREADSOG_H
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
#include "CommonSOG.h"




// typedef vector<Trans> vec_trans;
extern unsigned int nb_th;

class threadSOG : public CommonSOG{
 public:
  threadSOG(const net &, int BOUND = 32, int nbThread=2,bool uselace=false,bool init = false);
  void buildFromNet(int index);
  void computeDSOG(LDDGraph &g,bool canonised);
  void computeSeqSOG(LDDGraph &g);
  virtual ~threadSOG();
  static void *threadHandler(void *context);
  static void *threadHandlerCanonized(void *context);
  void *doCompute();
  void *doComputeCanonized();


  void computeSOGLace(LDDGraph &g);
  void computeSOGLaceCanonized(LDDGraph &g);



 protected:
 private:
 ////////////////////////////////




  int minCharge();
  bool Set_Bloc(MDD &M) const;
  bool Set_Bloc2(const MDD &M) const;
  bool Set_Div(MDD &M) const;
  bool Set_Div2(const MDD &M) const;
  bool isNotTerminated();

  timespec start, finish;

  MDD M0;

    //-----------------

  int m_NbIt;
  int m_itext, m_itint;
  int m_MaxIntBdd;
  MDD *m_TabMeta;
  int m_nbmetastate;
  double m_old_size;

  pile m_st[128];
  int m_charge[128];
  bool m_terminaison[128];

  int m_nb_thread;

  int m_min_charge;


  int m_bound, m_init;
  int m_id_thread;
  pthread_mutex_t m_mutex;
  pthread_mutex_t m_graph_mutex;
  pthread_mutex_t m_gc_mutex;
  pthread_mutex_t m_supervise_gc_mutex;
  unsigned int m_gc;

  pthread_mutex_t m_mutex_stack[128];
  pthread_spinlock_t m_spin_stack[128];
  pthread_t m_list_thread[128];
};

#endif  // DISTRIBUTEDSOG_H
