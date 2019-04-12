#include "LDDGraph.h"
#include <sylvan.h>
#include <stdio.h>
#include <string.h>
#include <map>
LDDGraph::~LDDGraph()
{
    //dtor
}


void LDDGraph::setInitialState(LDDState *c)
{
	m_currentstate=m_initialstate=c;

}

LDDState* LDDGraph::getInitialState() const {
    return m_GONodes.at(0);
}
/*----------------------find()----------------*/
LDDState * LDDGraph::find(LDDState* c)
{
    for(MetaLDDNodes::const_iterator i=m_GONodes.begin();!(i==m_GONodes.end());i++)
    //if((c->class_state.id()==(*i)->class_state.id())&&(c->blocage==(*i)->blocage)&&(c->boucle==(*i)->boucle))
        if(c->m_lddstate==(*i)->m_lddstate)
            return *i;
    return NULL;
}
/*----------------------find()----------------*/
bool LDDGraph::cmpSHA(const unsigned char *s1, const unsigned char *s2) {
    bool res=true;
    for (int i=0;i<16 && res;i++) {
        res=(s1[i]==s2[i]);
    }
    return !res;

}
LDDState * LDDGraph::findSHA(unsigned char* c)
{
    for(MetaLDDNodes::const_iterator i=m_GONodes.begin();!(i==m_GONodes.end());i++)
       if ((*i)->isVirtual()==true)
            if((cmpSHA(c,(unsigned char*)(*i)->m_SHA2)==0))
                return *i;
    return NULL;
}


/*----------------------insert() ------------*/
void LDDGraph::insert(LDDState *c)
{
	c->m_visited=false;
	this->m_GONodes.push_back(c);
	m_nbStates++;
}


/*----------------------insert() ------------*/
void LDDGraph::insertSHA(LDDState *c)
{
	c->m_visited=false;
	c->setVirtual();
	this->m_GONodes.push_back(c);


}

/*----------------------NbBddNod()------------------------*/
int LDDGraph::NbBddNode(LDDState * S, size_t& nb)
{
	/*if(S->m_visited==false)
	{
		//cout<<"insertion du meta etat numero :"<<nb<<"son id est :"<<S->class_state.id()<<endl;
		//cout<<"sa taille est :"<<bdd_nodecount(S->class_state)<<" noeuds \n";
		Tab[nb-1]=S->class_state;
		S->Visited=true;
		int bddnode=bdd_nodecount(S->class_state);
		int size_succ=0;
		for(Edges::const_iterator i=S->Successors.begin();!(i==S->Successors.end());i++)
		{
			if((*i).first->Visited==false)
			{
				nb++;
				size_succ+=NbBddNode((*i).first,nb);
			}
		}
		return size_succ+bddnode;

	}
	else*/
	cout<<"Not implemented yet...."<<endl;
		return 0;
}

/*----------------------Visualisation du graphe------------------------*/
void LDDGraph::printCompleteInformation()
{
    long count_ldd=0L;
     for(MetaLDDNodes::const_iterator i=m_GONodes.begin();!(i==m_GONodes.end());i++) {
        count_ldd+=lddmc_nodecount((*i)->m_lddstate);
        }
	cout << "\n\nGRAPH SIZE : \n";
	cout<<"\n\tNB LDD NODES : "<<count_ldd;
	cout<< "\n\tNB MARKING : "<< m_nbMarking;
	cout<< "\n\tNB NODES : "<< m_nbStates;
	cout<<"\n\tNB ARCS : " <<m_nbArcs<<endl;
	cout<<" \n\nCOMPLETE INFORMATION ?(y/n)\n";
	char c;
	cin>>c;
	//InitVisit(initialstate,n);

	size_t n=1;
	//cout<<"NB BDD NODE : "<<NbBddNode(initialstate,n)<<endl;
	NbBddNode(m_initialstate,n);
	// cout<<"NB BDD NODE : "<<bdd_anodecount(m_Tab,(int)m_nbStates)<<endl;
	//cout<<"Shared Nodes : "<<bdd_anodecount(Tab,nbStates)<<endl;
	InitVisit(m_initialstate,1);
	//int toto;cin>>toto;
	//bdd Union=UnionMetaState(initialstate,1);
	//cout<<"a titre indicatif taille de l'union : "<<bdd_nodecount(Union)<<endl;
	if(c=='y'||c=='Y')
	{
		size_t n=1;
		 printGraph(m_initialstate,n);
	}


}
/*----------------------InitVisit()------------------------*/
void LDDGraph::InitVisit(LDDState * S,size_t nb)
{

	if(nb<=m_nbStates)
	{
		S->m_visited=false;
		for(LDDEdges::const_iterator i=S->Successors.begin();!(i==S->Successors.end());i++)
		{

				if((*i).first->m_visited==true)
				{
					nb++;
					InitVisit((*i).first,nb);
				}
		}

	}
}
/*********                  printGraph    *****/

void LDDGraph::printGraph(LDDState *s,size_t &nb)
{
	if(nb<=m_nbStates)
	{
		cout<<"\nSTATE NUMBER "<<nb<<" sha : "<<s->getSHAValue()<<" LDD v :"<<s->getLDDValue()<<endl;

		s->m_visited=true;
		/*printsuccessors(s);
		getchar();
		printpredecessors(s);
		getchar();*/
		LDDEdges::const_iterator i;
		for(i=s->Successors.begin();!(i==s->Successors.end());i++)
		{
			if((*i).first->m_visited==false)
			{
				nb++;
				printGraph((*i).first, nb);
			}
		}

	}

}


/*---------void print_successors_class(Class_Of_State *)------------*/
void LDDGraph::printsuccessors(LDDState *s)
{
	/*Edges::const_iterator i;
	cout<<bddtable<<s->class_state<<endl;
	if(s->boucle)
		cout<<"\n\tON BOUCLE DESSUS AVEC EPSILON\n";
	if(s->blocage)
		cout<<"\n\tEXISTENCE D'UN ETAT BLOCANT\n";
	cout<<"\n\tSES SUCCESSEURS SONT  ( "<<s->Successors.size()<<" ) :\n\n";
	getchar();
	for(i =s->Successors.begin();!(i==s->Successors.end());i++)
	{
		cout<<" \t- t"<<(*i).second<<" ->";
		cout<<bddtable<<(*i).first->class_state<<endl;
		getchar();
	}*/
	cout<<"Not implemented yet!"<<endl;
}
/*---------void printpredescessors(Class_Of_State *)------------*/
void LDDGraph::printpredecessors(LDDState *s)
{
	/*Edges::const_iterator i;
	cout<<"\n\tSES PREDESCESSEURS SONT  ( "<<s->Predecessors.size()<<" ) :\n\n";
	getchar();
	for(i =s->Predecessors.begin();!(i==s->Predecessors.end());i++)
	{
		cout<<" \t- t"<<(*i).second<<" ->";
		cout<<bddtable<<(*i).first->class_state<<endl;
		getchar();
	}*/
	cout<<"Not implemented yet!"<<endl;
}
/*** Giving a position in m_GONodes Returns an LDDState ****/
LDDState *LDDGraph::getLDDStateById(unsigned int id) {
    return m_GONodes.at(id);
}

string LDDGraph::getTransition(int pos) {
    map<string,int>::iterator it=m_transition->begin();
    while(it != m_transition->end())
    {
        if(it->second == pos)
        return it->first;
        it++;
    }
    return "";
}

void LDDGraph::setTransition(map<string,int>& list_transitions) {
    m_transition=&list_transitions;
}

//void LDDGraph::setTransition(vector<string> list)

