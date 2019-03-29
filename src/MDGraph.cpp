
/********              Graph.cpp     *******/
#include "MDGraph.h"
/*#include <conio>*/

bdd * Tab;
/*********                  setInitialState    *****/

void MDGraph::setInitialState(Class_Of_State *c)
{
	currentstate=initialstate=c;

}
/*----------------------find()----------------*/
Class_Of_State * MDGraph::find(Class_Of_State* c)
{
	for(MetaGrapheNodes::const_iterator i=GONodes.begin();!(i==GONodes.end());i++)
	  //if((c->class_state.id()==(*i)->class_state.id())&&(c->blocage==(*i)->blocage)&&(c->boucle==(*i)->boucle))
if(c->class_state.id()==(*i)->class_state.id())
			return *i;
		return NULL;
}


/*----------------------insert() ------------*/
void MDGraph::insert(Class_Of_State *c)
{
	c->Visited=false;
	this->GONodes.push_back(c);
	nbStates++;
}

/*----------------------NbBddNod()------------------------*/
int MDGraph::NbBddNode(Class_Of_State * S, size_t& nb)
{
	if(S->Visited==false)
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
	else
		return 0;
}

/*----------------------Visualisation du graphe------------------------*/
void MDGraph::printCompleteInformation()
{


	cout << "\n\nGRAPH SIZE : \n";
	cout<< "\n\tNB MARKING : "<< nbMarking;
	cout<< "\n\tNB NODES : "<< nbStates;
	cout<<"\n\tNB ARCS : " <<nbArcs<<endl;
	cout<<" \n\nCOMPLETE INFORMATION ?(y/n)\n";
	char c;
	cin>>c;
	//InitVisit(initialstate,n);
	Tab=new bdd[(int)nbStates];
	size_t n=1;
	//cout<<"NB BDD NODE : "<<NbBddNode(initialstate,n)<<endl;
	NbBddNode(initialstate,n);
	cout<<"NB BDD NODE : "<<bdd_anodecount(Tab,(int)nbStates)<<endl;
	//cout<<"Shared Nodes : "<<bdd_anodecount(Tab,nbStates)<<endl;
	InitVisit(initialstate,1);
	//int toto;cin>>toto;
	//bdd Union=UnionMetaState(initialstate,1);
	//cout<<"a titre indicatif taille de l'union : "<<bdd_nodecount(Union)<<endl;
	if(c=='y'||c=='Y')
	{
		size_t n=1;
		 printGraph(initialstate,n);
	}


}
/*----------------------InitVisit()------------------------*/
void MDGraph::InitVisit(Class_Of_State * S,size_t nb)
{

	if(nb<=nbStates)
	{
		S->Visited=false;
		for(Edges::const_iterator i=S->Successors.begin();!(i==S->Successors.end());i++)
		{

				if((*i).first->Visited==true)
				{
					nb++;
					InitVisit((*i).first,nb);
				}
		}

	}
}
/*********                  printGraph    *****/

void MDGraph::printGraph(Class_Of_State *s,size_t &nb)
{
	if(nb<=nbStates)
	{
		cout<<"\nSTATE NUMBER "<<nb<<" : \n";
		s->Visited=true;
		printsuccessors(s);
		getchar();
		printpredecessors(s);
		getchar();
		Edges::const_iterator i;
		for(i=s->Successors.begin();!(i==s->Successors.end());i++)
		{
			if((*i).first->Visited==false)
			{
				nb++;
				printGraph((*i).first, nb);
			}
		}

	}

}


/*---------void print_successors_class(Class_Of_State *)------------*/
void MDGraph::printsuccessors(Class_Of_State *s)
{
	Edges::const_iterator i;
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
	}
}
/*---------void printpredescessors(Class_Of_State *)------------*/
void MDGraph::printpredecessors(Class_Of_State *s)
{
	Edges::const_iterator i;
	cout<<"\n\tSES PREDESCESSEURS SONT  ( "<<s->Predecessors.size()<<" ) :\n\n";
	getchar();
	for(i =s->Predecessors.begin();!(i==s->Predecessors.end());i++)
	{
		cout<<" \t- t"<<(*i).second<<" ->";
		cout<<bddtable<<(*i).first->class_state<<endl;
		getchar();
	}
}

