#ifndef LDDGRAPH_H
#define LDDGRAPH_H
#include "LDDState.h"
//#include "LDDStateExtend.h"
using namespace std;
#include <iostream>
#include <map>
typedef set<int> Set;
typedef vector<LDDState*> MetaLDDNodes;

class LDDGraph
{
    private:
        map<string,int>* m_transition;
		void printGraph(LDDState *, size_t &);

	public:
        string getTransition(int pos);
        void setTransition(map<string,int>& list_transitions);
        MetaLDDNodes m_GONodes;
        LDDState *getLDDStateById(unsigned int id);
		void Reset();
		LDDState *m_initialstate;
		LDDState *m_currentstate;
		long m_nbStates;
		long m_nbMarking;
		long m_nbArcs;
		LDDState* find(LDDState*);
		LDDState* findSHA(unsigned char*);
		bool cmpSHA(const unsigned char *s1, const unsigned char *s2);
		void insertSHA(LDDState *c);
		LDDEdges& get_successor(LDDState*);
		void printsuccessors(LDDState *);
		int NbBddNode(LDDState*,size_t&);
		void InitVisit(LDDState * S,size_t nb);
		void printpredecessors(LDDState *);
		void addArc(){m_nbArcs++;}
		void insert(LDDState*);
		LDDGraph() {m_nbStates=m_nbArcs=m_nbMarking=0;}
		void setInitialState(LDDState*);  //Define the initial state of this graph
		LDDState* getInitialState() const;
		void printCompleteInformation();
		virtual ~LDDGraph();
};

#endif // LDDGRAPH_H
