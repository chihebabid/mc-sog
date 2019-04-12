#ifndef CLASS_OF_STATE
#define CLASS_OF_STATE
using namespace std;
#include "bdd.h"
//#include <map.h>
#include<unordered_map>
#include <set>
#include <vector>
//#include <pair>
//#include <ext/hash_map>
typedef set<int> Set;
class Class_Of_State
{
	public:
		Class_Of_State(){boucle=blocage=Visited=0;}
		Set firable;
		bdd class_state;
		bool boucle;
		bool blocage;
		bool Visited;
		void * Class_Appartenance;
		vector<pair<Class_Of_State*,int> > Predecessors, Successors;
		pair<Class_Of_State*,int>  LastEdge;
};
typedef pair<Class_Of_State*, int> Edge;
typedef vector<Edge> Edges;
#endif
