#ifndef LDDGRAPH_H
#define LDDGRAPH_H
#include "LDDState.h"
#include "CommonSOG.h"
#include <mutex>
#include <shared_mutex>
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
        mutable std::shared_mutex m_mutex;
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
        LDDState *getLDDStateById(const unsigned int& id);
		LDDState *m_initialstate=nullptr;
        uint64_t m_nbStates;
		uint64_t m_nbMarking;
		atomic<uint32_t> m_nbArcs;
		LDDState* find(LDDState*);
        LDDState* insertFindByMDD(MDD,bool&);
		LDDState* findSHA(unsigned char*);
		LDDState* insertFindSha(unsigned char*,LDDState*);
        size_t findSHAPos(unsigned char* ch,bool &res);
		void insertSHA(LDDState *c);
		void InitVisit(LDDState * S,size_t nb);

		inline void addArc()  {m_nbArcs++;}
		void insert(LDDState*);
		explicit LDDGraph(CommonSOG *constuctor) {m_nbArcs=m_nbMarking=0;m_constructor=constuctor;}
		void setInitialState(LDDState*);  //Define the initial state of this graph
        LDDState *getInitialState() const {
            return m_GONodes[0];
        }
        LDDState *getInitialAggregate() const {
            return m_initialstate;
        }
		void printCompleteInformation();
		virtual ~LDDGraph();
};

#endif // LDDGRAPH_H
