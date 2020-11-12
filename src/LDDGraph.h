#ifndef LDDGRAPH_H
#define LDDGRAPH_H
#include "LDDState.h"
#include "CommonSOG.h"
#include <mutex>
#include <atomic>
//#include "LDDStateExtend.h"
using namespace std;
#include <iostream>
#include <map>

typedef set<int> Set;
typedef vector<LDDState*> MetaLDDNodes;
class CommonSOG;
class LDDGraph
{
    private:
        mutable std::mutex m_mutex;
        mutable std::mutex m_mutex_sha;
        map<string,uint16_t>* m_transition;
        map<uint16_t,string>* m_places;
		void printGraph(LDDState *, size_t &);
        CommonSOG *m_constructor;
	public:
        CommonSOG* getConstructor() {return m_constructor;}
        string_view getTransition(uint16_t pos);
        string_view getPlace(uint16_t pos);
        void setPlace(map<uint16_t,string>& list_places);
        void setTransition(map<string,uint16_t>& list_transitions);
        MetaLDDNodes m_GONodes;
        LDDState *getLDDStateById(unsigned int id);
		void Reset();
		LDDState *m_initialstate=nullptr;
		LDDState *m_currentstate;
        uint64_t m_nbStates;
		uint64_t m_nbMarking;
		atomic<uint32_t> m_nbArcs;
		LDDState* find(LDDState*);
        LDDState* insertFind(LDDState*);
		LDDState* findSHA(unsigned char*);
		LDDState* insertFindSha(unsigned char*,LDDState*);
        size_t findSHAPos(unsigned char*,bool &res);
		bool cmpSHA(const unsigned char *s1, const unsigned char *s2);
		void insertSHA(LDDState *c);
		LDDEdges& get_successor(LDDState*);
		void printsuccessors(LDDState *);
		int NbBddNode(LDDState*,size_t&);
		void InitVisit(LDDState * S,size_t nb);
		void printpredecessors(LDDState *);
		inline void addArc()  {m_nbArcs++;}
		void insert(LDDState*);
		LDDGraph(CommonSOG *constuctor) {m_nbArcs=m_nbMarking=0;m_constructor=constuctor;}
		void setInitialState(LDDState*);  //Define the initial state of this graph
		//LDDState* getInitialState() const;
        LDDState *getInitialState() const {
            return m_GONodes.at(0);
        }
        LDDState *getInitialAggregate() {
            return m_initialstate;
        }
		void printCompleteInformation();
		virtual ~LDDGraph();
};

#endif // LDDGRAPH_H
