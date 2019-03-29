

/******************    Graph.hpp  **************************/

#ifndef _MDGRAPH_

#define _MDGRAPH_
using namespace std;
#include <iostream>
/*#include <time>*/
#include <vector>
#include "Class_of_state.h"
#include <list>
typedef set<int> Set;
typedef vector<Class_Of_State*> MetaGrapheNodes;
class MDGraph
{
	private:
		void printGraph(Class_Of_State *, size_t &);
		MetaGrapheNodes GONodes;
	public:
		void Reset();
		Class_Of_State *initialstate;
		Class_Of_State *currentstate;
		double nbStates;
		double nbMarking;
		double nbArcs;
		Class_Of_State* find(Class_Of_State*);
		Edges& get_successor(Class_Of_State*);
		void printsuccessors(Class_Of_State *);
		int NbBddNode(Class_Of_State*,size_t&);
		void InitVisit(Class_Of_State * S,size_t nb);
		void printpredecessors(Class_Of_State *);
		void addArc(){nbArcs++;}
		void insert(Class_Of_State*);
		MDGraph(){nbStates=nbArcs=nbMarking=0;}
		void setInitialState(Class_Of_State*);  //Define the initial state of this graph
		void printCompleteInformation();



};
#endif
