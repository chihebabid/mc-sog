#ifndef LDDSTATE_H
#define LDDSTATE_H
#include <sylvan.h>
#include <set>
#include <vector>
#include <string>

using namespace std;
using namespace sylvan;
typedef set<int> Set;

class LDDState {
 public:

  LDDState() {
    m_boucle = m_blocage = m_visited = false;m_completed=false;
    m_virtual = false;
  }
  virtual ~LDDState();
  Set firable;
  void* Class_Appartenance;
  vector<pair<LDDState*, int>>* getSuccessors();
  vector<pair<LDDState*, int> > Predecessors, Successors;
  pair<LDDState*, int> LastEdge;
  void setLDDValue(MDD m);
  MDD getLDDValue();
  MDD m_lddstate=0;
  unsigned char m_SHA2[81];
  unsigned char* getSHAValue();
  bool m_boucle=false;
  bool m_blocage=false;


  bool isVirtual();
  void setVirtual();
  void setDiv(bool di) {m_boucle=di;}
  bool isDiv() {return m_boucle;}
  void setDeadLock(bool de) {m_blocage=de;}
  bool isDeadLock() {return m_blocage;}
  void setVisited() {m_visited=true;}
  bool isVisited() {return m_visited;}
  void setCompletedSucc() {m_completed=true;}
  bool isCompletedSucc() {return m_completed;}
  vector<uint16_t> getMarkedPlaces(set<uint16_t>& lplacesAP);
  vector<uint16_t> getUnmarkedPlaces(set<uint16_t>& lplacesAP);
  void setProcess(uint16_t v) {m_process=v;}
  uint16_t getProcess() {return m_process;}
 protected:
 private:
  bool m_virtual = false;
  bool m_visited=false;
  bool m_completed=false;
  uint16_t m_process=0;
};

typedef pair<LDDState*, int> LDDEdge;
typedef vector<LDDEdge> LDDEdges;
#endif  // LDDSTATE_H
