#ifndef LDDSTATE_H
#define LDDSTATE_H

#include <set>
#include <vector>
#include <string>
#include <atomic>
#include "SylvanWrapper.h"
using namespace std;

typedef set<int> Set;

class LDDState {
 public:

  LDDState() {
    m_boucle = m_blocage = m_visited = false;m_completed=false;
    m_currentLevel=0;m_nbSuccessorsToBeProcessed=0;
    m_virtual = false;
  }
  virtual ~LDDState();
  vector<pair<LDDState*, int>>* getSuccessors();
  vector<pair<LDDState*, int> > Predecessors, Successors;

  void setLDDValue(const MDD& m);
  MDD getLDDValue();
  MDD m_lddstate=0;
  unsigned char m_SHA2[81];
  unsigned char* getSHAValue();
  bool m_boucle=false;
  bool m_blocage=false;
  bool isVirtual() {return m_virtual;}
  void setVirtual(){m_virtual=true;}
  void setDiv(const bool& di) {m_boucle=di;}
  inline bool isDiv() {return m_boucle;}
  void setDeadLock(const bool& de) {m_blocage=de;}
  bool isDeadLock() {return m_blocage;}
  inline void setVisited() {m_visited=true;}
  [[nodiscard]] inline bool isVisited() const {return m_visited;}
  void setCompletedSucc() {m_completed=true;}
  bool isCompletedSucc() {return m_completed;}
  vector<uint16_t> getMarkedPlaces(set<uint16_t>& lplacesAP);
  vector<uint16_t> getUnmarkedPlaces(set<uint16_t>& lplacesAP);
  void setProcess(const uint16_t& v) {m_process=v;}
  [[nodiscard]] uint16_t getProcess() const {return m_process;}
  inline void decNbSuccessors() {m_nbSuccessorsToBeProcessed--;}
 protected:
 private:
  bool m_virtual = false;
  bool m_visited=false;
  volatile bool m_completed=false;
  uint16_t m_process=0;
  uint8_t  m_currentLevel;

public:
    atomic<uint32_t> m_nbSuccessorsToBeProcessed;

public:
    [[nodiscard]] uint8_t getMCurrentLevel() const;
    void setMCurrentLevel(uint8_t mCurrentLevel);
};

typedef pair<LDDState*, int> LDDEdge;
typedef vector<LDDEdge> LDDEdges;
#endif  // LDDSTATE_H
