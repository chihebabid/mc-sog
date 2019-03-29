/* -*- C++ -*- */
#ifndef RdPBDD_H
#define RdPBDD_H
#include <math.h>
#include <stack>
#include <vector>
#include "MDGraph.h"
#include "Net.hpp"
#include "bdd.h"
#include "bvec.h"
/***********************************************/
class Trans {
 public:
  Trans(const bdd& var, bddPair* pair, const bdd& rel, const bdd& prerel,
        const bdd& Precond, const bdd& Postcond);
  bdd operator()(const bdd& op) const;
  bdd operator[](const bdd& op) const;

  friend class RdPBDD;

 private:
  bdd var;
  bddPair* pair;
  bdd Precond;
  bdd Postcond;
  bdd rel;
  bdd prerel;
};

class RdPBDD {
 private:
  vector<class Transition> transitions;
  Set Observable;
  Set NonObservable;
  map<string, int> transitionName;
  Set InterfaceTrans;
  Set Formula_Trans;
  unsigned int Nb_places;

 public:
  bdd M0;
  bdd currentvar;
  vector<Trans> relation;
  bdd ReachableBDD1();
  bdd ReachableBDD2();
  bdd ReachableBDD3();
  /* G�n�ration de graphes d'observation */
  void compute_canonical_deterministic_graph_Opt(MDGraph& g);
  bdd Accessible_epsilon(bdd From);
  bdd Accessible_epsilon2(bdd From);
  bdd StepForward(bdd From);
  bdd StepForward2(bdd From);
  bdd StepBackward(bdd From);
  bdd StepBackward2(bdd From);
  bdd EmersonLey(bdd S, bool trace);
  Set firable_obs(bdd State);
  bdd get_successor(bdd From, int t);
  bool Set_Div(bdd& S) const;
  bool Set_Bloc(bdd& S) const;
  bdd FrontiereNodes(bdd From) const;
  bdd CanonizeR(bdd s, unsigned int i);
  RdPBDD(const net&, int BOUND = 32, bool init = false);
  ~RdPBDD(){};
};

#endif
