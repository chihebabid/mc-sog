#ifndef LDDSTATE_H
#define LDDSTATE_H
#include <sylvan.h>
#include <set>
#include <vector>
using namespace std;
using namespace sylvan;
typedef set<int> Set;

class LDDState {
 public:
  LDDState() {
    m_boucle = m_blocage = m_visited = false;
    m_virtual = false;
    m_marked = false;
  }
  virtual ~LDDState();
  Set firable;
  void* Class_Appartenance;
  vector<pair<LDDState*, int>>* getSuccessors();
  vector<pair<LDDState*, int> > Predecessors, Successors;
  pair<LDDState*, int> LastEdge;
  void setLDDValue(MDD m);
  MDD getLDDValue();
  MDD m_lddstate;
  unsigned char m_SHA2[81];
  unsigned char* getSHAValue();
  bool m_boucle;
  bool m_blocage;
  bool m_visited;
  bool isVirtual();
  void setVirtual();
  void setDiv(bool di) {m_boucle=di;}
  bool isDiv() {return m_boucle;}
  void setDeadLock(bool de) {m_blocage=de;}
  bool isDeadLock() {return m_blocage;}
  bool isMarked();
  void setMarked();
 protected:
 private:
  bool m_virtual = false;
};

typedef pair<LDDState*, int> LDDEdge;
typedef vector<LDDEdge> LDDEdges;
#endif  // LDDSTATE_H
