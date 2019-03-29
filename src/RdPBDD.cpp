/* -*- C++ -*- */
#include <string>
#include <iostream>
#include <set>
#include <vector>
//#include <map>
#include <stack>
#include <ext/hash_map>
using namespace std;

#include "RdPBDD.h"
int NbIt;
int itext,itint;
int MaxIntBdd;
bdd *TabMeta;
int nbmetastate;
double old_size;
const vector<class Place> *vplaces = NULL;
void my_error_handler(int errcode) {
    cout<<"errcode = "<<errcode<<endl;
	if (errcode == BDD_RANGE) {
		// Value out of range : increase the size of the variables...
		// but which one???
		bdd_default_errhandler(errcode);
	}
	else {
		bdd_default_errhandler(errcode);
	}
}

/*****************************************************************/
/*                         printhandler                          */
/*****************************************************************/
void printhandler(ostream &o, int var)
{
	o << (*vplaces)[var/2].name;
	if (var%2)
		o << "_p";
}

/*****************************************************************/
/*                         class Trans                           */
/*****************************************************************/
Trans::Trans(const bdd& v, bddPair* p, const bdd& r,const bdd& pr, const bdd& pre, const bdd& post): var(v),pair(p),rel(r),prerel(pr),Precond(pre),Postcond(post) {
}
//Franchissement avant
bdd Trans::operator()(const bdd& n) const {
	bdd res = bdd_relprod(n,rel,var);
	res = bdd_replace(res, pair);
	return res;
}
//Franchissement arrière
bdd Trans::operator[](const bdd& n) const {
	bdd res = bdd_relprod(n,prerel,var);
	res = bdd_replace(res, pair);
	return res;
}

/*****************************************************************/
/*                         class RdPBDD                          */
/*****************************************************************/

RdPBDD::RdPBDD(const net &R, int BOUND,bool init){
    int nbPlaces=R.places.size(), i, domain;
	vector<Place>::const_iterator p;

	bvec *v = new bvec[nbPlaces];
	bvec *vp = new bvec[nbPlaces];
	bvec *prec = new bvec[nbPlaces];
	bvec *postc = new bvec[nbPlaces];
	int *idv = new int[nbPlaces];
	int *idvp = new int[nbPlaces];
	int *nbbit = new int[nbPlaces];
	if(!init)
	  bdd_init(1000000,1000000);
	// the error handler
	bdd_error_hook((bddinthandler)my_error_handler);
	//_______________
	transitions=R.transitions;
	Observable=R.Observable;
	NonObservable=R.NonObservable;
	Formula_Trans=R.Formula_Trans;
	transitionName=R.transitionName;
	InterfaceTrans=R.InterfaceTrans;
	Nb_places=R.places.size();
	cout<<"Nombre de places : "<<Nb_places<<endl;
	cout<<"Derniere place : "<<R.places[Nb_places-1].name<<endl;
  /* place domain, place bvect, place initial marking and place name */
	// domains
	i=0;
	for(i=0,p=R.places.begin();p!=R.places.end();i++,p++) {
		if (p->hasCapacity()) {
			domain = p->capacity+1;
		}
		else {
			domain = BOUND+1; // the default domain
		}
		// variables are created one by one (implying contigue binary variables)
		fdd_extdomain(&domain, 1);
		//cout<<"nb created var : "<<bdd_varnum()<<endl;
		fdd_extdomain(&domain, 1);
		//cout<<"nb created var : "<<bdd_varnum()<<endl;
    }

	// bvec
	currentvar = bdd_true();
	for(i=0;i<nbPlaces;i++) {
		nbbit[i] = fdd_varnum(2*i);
		//cout<<"nb var pour "<<2*i<<" = "<<fdd_domainsize(2*i)<<endl;
		v[i]=bvec_varfdd(2*i);
		vp[i]=bvec_varfdd(2*i+1);
		//cout<<"nb var pour "<<2*i+1<<" = "<<fdd_domainsize(2*i+1)<<endl;
		currentvar = currentvar & fdd_ithset(2*i);
	}

	// initial marking
    M0=bdd_true();
	for(i=0,p=R.places.begin();p!=R.places.end();i++,p++)
		M0 = M0 & fdd_ithvar(2*i,p->marking);

	// place names
	vplaces = &R.places;
	fdd_strm_hook(printhandler);


  /* Transition relation */
    for(vector<Transition>::const_iterator t=R.transitions.begin();
	t!=R.transitions.end();t++){
		int np=0;
		bdd rel = bdd_true(), var = bdd_true(),prerel=bdd_true();
		bdd Precond=bdd_true(),Postcond=bdd_true();
		bddPair *p=bdd_newpair();

		for(i=0;i<nbPlaces;i++) {
			prec[i]=bvec_con(nbbit[i],0);
			postc[i]=bvec_con(nbbit[i],0);
		}
		// calculer les places adjacentes a la transition t
		// et la relation rel de la transition t
		set<int> adjacentPlace;

		// arcs pre
		for(vector< pair<int,int> >::const_iterator it=t->pre.begin();it!=t->pre.end(); it++) {
			adjacentPlace.insert(it->first);
			prec[it->first] = prec[it->first] + bvec_con(nbbit[it->first], it->second);
		}
		// arcs post
		for(vector< pair<int,int> >::const_iterator it=t->post.begin();it!=t->post.end(); it++) {
			adjacentPlace.insert(it->first);
			postc[it->first] = postc[it->first] + bvec_con(nbbit[it->first], it->second);
		}
		// arcs pre automodifiants
		for(vector< pair<int,int> >::const_iterator it=t->preAuto.begin();it!=t->preAuto.end(); it++) {
			adjacentPlace.insert(it->first);
			prec[it->first] = prec[it->first] + v[it->second];
		}
		// arcs post automodifiants
		for(vector< pair<int,int> >::const_iterator it=t->postAuto.begin();it!=t->postAuto.end(); it++) {
			adjacentPlace.insert(it->first);
			postc[it->first] = postc[it->first] + v[it->second];
		}
		// arcs reset
		for(vector<int>::const_iterator it=t->reset.begin();it!=t->reset.end(); it++) {
			adjacentPlace.insert(*it);
			prec[*it] = prec[*it] + v[*it];
		}

		np=0;
		for(set<int>::const_iterator it=adjacentPlace.begin();it!=adjacentPlace.end(); it++) {
			idv[np]=2*(*it);
			idvp[np]=2*(*it)+1;
			var = var & fdd_ithset(2*(*it));
			//Image
			// precondition
			rel = rel & (v[*it] >= prec[*it]);
			Precond = Precond & (v[*it] >= prec[*it]);
			// postcondition
			rel = rel & (vp[*it] == (v[*it] - prec[*it] + postc[*it]));
			// Pre image __________
			// precondition
			prerel = prerel & (v[*it] >= postc[*it]);
			// postcondition
			Postcond=Postcond & (v[*it]>=postc[*it]);
			prerel = prerel & (vp[*it] == (v[*it] - postc[*it] + prec[*it]));
			//___________________
			// capacité
			if (R.places[*it].hasCapacity())
				rel = rel & (vp[*it] <= bvec_con(nbbit[*it], R.places[*it].capacity));
			np++;
		}
		fdd_setpairs(p, idvp, idv, np);

		// arcs inhibitor
		for(vector< pair<int,int> >::const_iterator it=t->inhibitor.begin();it!=t->inhibitor.end(); it++)
			rel = rel & (v[it->first]< bvec_con(nbbit[it->first], it->second));

		relation.push_back(Trans(var, p, rel,prerel,Precond,Postcond));
    }
	delete [] v;
	delete [] vp;
	delete [] prec;
	delete [] postc;
	delete [] idv;
	delete [] idvp;
	delete [] nbbit;
}
/*----------------------------------- Reachability space using bdd ----------*/
bdd RdPBDD:: ReachableBDD1()
{
  bdd M1;
  bdd M2=M0;
  double d,tps;
  d=(double)clock() / (double)CLOCKS_PER_SEC;
  NbIt=0;
  MaxIntBdd=bdd_nodecount(M0);
  while(M1!=M2){
		M1=M2;
		for(vector<Trans>::const_iterator i = relation.begin();
				i!=relation.end();i++) {
			bdd succ = (*i)(M2);
			M2=succ|M2;
			//cout << bdd_nodecount(succ) << endl;
			//if(succ!=bddfalse)
		}
		NbIt++;
		int Current_size=bdd_nodecount(M2);
		if(MaxIntBdd<Current_size)
		  MaxIntBdd=Current_size;
		//cout<<"Iteration numero "<<NbIt<<" nb node de reached :"<<bdd_nodecount(M2)<<endl;
		cout << " nb node reached :" << bdd_nodecount(M2) << endl;
	}
	cout << endl;
	tps = ((double)(clock()) / CLOCKS_PER_SEC) - d;
	cout<<"-----------------------------------------------------\n";
	cout << "CONSTRUCTION TIME  " << tps << endl;
	cout<<"Max Intermediary BDD "<<MaxIntBdd<<endl;
	cout<<"Nombre d'iteration : "<<NbIt<<endl;
	//bdd Cycle=EmersonLey(M2,0);
	//cout<<Cycle.id()<<endl;
	return M2;
}
bdd RdPBDD:: ReachableBDD2()
{
    bdd M1;
  bdd M2=M0;
  double d,tps;
  d=(double)clock() / (double)CLOCKS_PER_SEC;
  NbIt=0;
  MaxIntBdd=bdd_nodecount(M0);
  while(M1!=M2){
		M1=M2;
		bdd Reached;
		for(vector<Trans>::const_iterator i = relation.begin();
				i!=relation.end();i++) {
			bdd succ = (*i)(M2);
			Reached=succ|Reached;
			//cout << bdd_nodecount(succ) << endl;
			//if(succ!=bddfalse)
		}
		NbIt++;
		M2=M2|Reached;
		int Current_size=bdd_nodecount(M2);
		if(MaxIntBdd<Current_size)
		  MaxIntBdd=Current_size;
		//cout<<"Iteration numero "<<NbIt<<" nb node de reached :"<<bdd_nodecount(M2)<<endl;
//		cout << bdd_nodecount(M2) << endl;
	}
	cout << endl;
	tps = ((double)(clock()) / CLOCKS_PER_SEC) - d;
	cout<<"-----------------------------------------------------\n";
	cout << "CONSTRUCTION TIME  " << tps << endl;
	cout<<"Max Intermediary BDD "<<MaxIntBdd<<endl;
	cout<<"Nombre d'iteration : "<<NbIt<<endl;
	return M2;
}
bdd RdPBDD:: ReachableBDD3()
{
  double d,tps;
  d=(double)clock() / (double)CLOCKS_PER_SEC;
	bdd New,Reached,From;
	Reached=From=M0;
	NbIt=0;
	do{
	  NbIt++;
	     bdd succ;
	     for(vector<Trans>::const_iterator i = relation.begin();	   	i!=relation.end();i++)
		 succ=(*i)(From)|succ;
  	    New=succ-Reached;
	    From=New;
	    Reached=Reached | New;
	    cout<<"Iteration numero "<<NbIt<<" nb node de reached :"<<bdd_nodecount(Reached)<<endl;
	}while(New!=bddfalse);
	tps=(double)clock() / (double)CLOCKS_PER_SEC-d;
	cout << "TPS CONSTRUCTION : "<<tps<<endl;
	return Reached;
}
/*----------------Fermeture transitive sur les transitions non observées ---*/
bdd RdPBDD::Accessible_epsilon2(bdd Init)
{

  bdd Reached,New,From;
  Reached=From=Init;
  do{
       bdd succ;
	for(Set::const_iterator i=NonObservable.begin();!(i==NonObservable.end());i++)

		 succ= relation[(*i)](From)|succ;
  	    New=succ-Reached;
	    From=New;
	    Reached=Reached | New;
	}while(New!=bddfalse);
	cout << endl;
	return Reached;
}
bdd RdPBDD::Accessible_epsilon(bdd From)
{
  bdd M1;
  bdd M2=From;
  int it=0;
  do{
        M1=M2;
	for(Set::const_iterator i=NonObservable.begin();!(i==NonObservable.end());i++)
	  {

	    bdd succ= relation[(*i)](M2);
	    M2=succ|M2;
	  }

	TabMeta[nbmetastate]=M2;
	int intsize=bdd_anodecount(TabMeta,nbmetastate+1);
	if(MaxIntBdd<intsize)
	    MaxIntBdd=intsize;
	it++;
	//	cout << bdd_nodecount(M2) << endl;
  }while(M1!=M2);
  //cout << endl;
	return M2;
}
/*------------------------  StepForward()  --------------*/
bdd RdPBDD::StepForward2(bdd From)
{
  // cout<<"Debut Step Forward \n";
  bdd Res;
  for(Set::const_iterator i=NonObservable.begin();!(i==NonObservable.end());i++)
  {
       bdd succ = relation[(*i)](From);
       Res=Res|succ;



  }
  //cout<<"Fin Step Forward \n";
  return Res;
}
bdd RdPBDD::StepForward(bdd From)
{
  //cout<<"Debut Step Forward \n";
  bdd Res=From;
  for(Set::const_iterator i=NonObservable.begin();!(i==NonObservable.end());i++)
  {
       bdd succ = relation[(*i)](Res);
       Res=Res|succ;
  }
  //cout<<"Fin Step Forward \n";
  return Res;
}
/*------------------------  StepBackward()  --------------*/
bdd RdPBDD::StepBackward2(bdd From)
{
  bdd Res;
  cout<<"Ici Step Back : From.id() = "<<From.id()<<endl;
   for(vector<Trans>::const_iterator t=relation.begin();
	t!=relation.end();t++){
       bdd succ = (*t)[From];
       Res=Res|succ;
  }
  cout<<"Res.id() = "<<Res.id()<<endl;
  return Res;
}
bdd RdPBDD::StepBackward(bdd From)
{
  bdd Res=From;
   for(vector<Trans>::const_iterator t=relation.begin();
	t!=relation.end();t++){
       bdd succ = (*t)[Res];
       Res=Res|succ;
  }
  return Res;
}
/*---------------------------GetSuccessor()-----------*/
bdd RdPBDD::get_successor(bdd From,int t)
{
  return relation[t](From);
}
/*-------------------------Firable Obs()--------------*/
Set RdPBDD::firable_obs(bdd State)
{
  Set res;
  for(Set::const_iterator i=Observable.begin();!(i==Observable.end());i++)
  {
	{bdd succ =  relation[(*i)](State);
	if(succ!=bddfalse)
	  res.insert(*i);}
   }
  return res;
}

/*------------------------------------Observation Graph ----------*/
void  RdPBDD::compute_canonical_deterministic_graph_Opt(MDGraph& g)
{
  cout<<"COMPUTE CANONICAL DETERMINISTIC GRAPH_________________________ \n";
  //cout<<"nb bdd var : "<<bdd_varnum()<<endl;
  double d,tps;
  d=(double)clock() / (double)CLOCKS_PER_SEC;
  TabMeta=new bdd[100000];
  nbmetastate=0;
  MaxIntBdd=0;
  typedef pair<Class_Of_State*,bdd> couple;
  typedef pair<couple, Set> Pair;
  typedef stack<Pair> pile;
  pile st;
  NbIt=0;
  itext=itint=0;
   Class_Of_State* reached_class;
   Set fire;
   // size_t max_meta_state_size;
   Class_Of_State *c=new Class_Of_State;
   {
     // cout<<"Marquage initial :\n";
     //cout<<bddtable<<M0<<endl;
     bdd Complete_meta_state=Accessible_epsilon(M0);
     //cout<<"Apres accessible epsilon \n";
     fire=firable_obs(Complete_meta_state);
     //c->blocage=Set_Bloc(Complete_meta_state);
     c->boucle=Set_Div(Complete_meta_state);
     //c->class_state=CanonizeR(Complete_meta_state,0);
     //cout<<"Apres CanonizeR nb representant : "<<bdd_pathcount(c->class_state)<<endl;
     c->class_state=Complete_meta_state;
     TabMeta[nbmetastate]=c->class_state;
     nbmetastate++;
     old_size=bdd_nodecount(c->class_state);
     //max_meta_state_size=bdd_pathcount(Complete_meta_state);
     st.push(Pair(couple(c,Complete_meta_state),fire));
   }
   g.setInitialState(c);
   g.insert(c);
   g.nbMarking+=bdd_pathcount(c->class_state);
   do
   {
      // cout<<"in loop"<<endl;
     Pair  e=st.top();
     st.pop();
     nbmetastate--;
     while(!e.second.empty())
     {
	int t = *e.second.begin();
	e.second.erase(t);
	double nbnode;
	reached_class=new Class_Of_State;
	{
	  //  cout<<"Avant Accessible epsilon \n";
	  bdd Complete_meta_state=Accessible_epsilon(get_successor(e.first.second,t));
	  //cout<<"Apres accessible epsilon \n";
	  //cout<<"Avant CanonizeR \n";
	  //reached_class->class_state=CanonizeR(Complete_meta_state,0);
	   //cout<<"Apres CanonizeR \n";
	  // cout<<"Apres CanonizeR nb representant : "<<bdd_pathcount(reached_class->class_state)<<endl;
	   reached_class->class_state=Complete_meta_state;
	  Class_Of_State* pos=g.find(reached_class);
	  nbnode=bdd_pathcount(reached_class->class_state);
	  if(!pos)
	  {
        //  cout<<"not found"<<endl;
	    //reached_class->blocage=Set_Bloc(Complete_meta_state);
	    //reached_class->boucle=Set_Div(Complete_meta_state);
	    fire=firable_obs(Complete_meta_state);
	    st.push(Pair(couple(reached_class,Complete_meta_state),fire));
	    //TabMeta[nbmetastate]=reached_class->class_state;
	    nbmetastate++;
	    //old_size=bdd_anodecount(TabMeta,nbmetastate);
	    e.first.first->Successors.insert(e.first.first->Successors.begin(),Edge(reached_class,t));
	    reached_class->Predecessors.insert(reached_class->Predecessors.begin(),Edge(e.first.first,t));
	    g.addArc();
	    g.insert(reached_class);
	  }
	  else
	  {
        //  cout<<" found"<<endl;
	    delete reached_class;
	    e.first.first->Successors.insert(e.first.first->Successors.begin(),Edge(pos,t));
	    pos->Predecessors.insert(pos->Predecessors.begin(),Edge(e.first.first,t));
	    g.addArc();
	  }
	}

     }
  }while(!st.empty());
   tps=(double)clock() / (double)CLOCKS_PER_SEC-d;
   cout<<"TIME OF CONSTRUCTIONR : "<<tps<<endl;
   cout<<" MAXIMAL INTERMEDIARY BDD SIZE \n"<<MaxIntBdd<<endl;
   cout<<"OLD SIZE : "<<old_size<<endl;
   //cout<<"NB SHARED NODES : "<<bdd_anodecount(TabMeta,nbmetastate)<<endl;
   cout<<"NB META STATE DANS CONSTRUCTION : "<<nbmetastate<<endl;
   cout<<"NB ITERATIONS CONSTRUCTION : "<<NbIt<<endl;
   cout<<"Nb Iteration externes : "<<itext<<endl;
   cout<<"Nb Iteration internes : "<<itint<<endl;
}
/*-----------------------CanonizeR()----------------*/
bdd RdPBDD::CanonizeR(bdd s, unsigned int i)
 {
   bdd s1,s2;
   do
   {
	itext++;
   	s1 = s - bdd_nithvar(2*i);
   	s2 = s - s1;
   	if((!(s1==bddfalse))&&(!(s2==bddfalse)))
   	{
   		bdd front = s1;
   		bdd reached = s1;
   		do
   		{
		  //cout<<"premiere boucle interne \n";
   		    itint++;
   		    front=StepForward(front) - reached;
   		    reached = reached | front;
   		    s2 = s2 - front;
   		}while((!(front==bddfalse))&&(!(s2==bddfalse)));
   	}
   	if((!(s1==bddfalse))&&(!(s2==bddfalse)))
   	{
   		bdd front=s2;
   		bdd reached = s2;
   		do
   		{
		  // cout<<"deuxieme boucle interne \n";
		   itint++;
   		   front=StepForward(front) - reached;
   		   reached = reached | front;
   		   s1 = s1 - front;
   		}while((!(front==bddfalse))&&(!(s1==bddfalse)));
   	}
   	s=s1 | s2;
  	i++;
  }while((i<Nb_places) &&((s1==bddfalse)||(s2==bddfalse)));
  if(i>=Nb_places)
   {
     //cout<<"____________oooooooppppppppsssssssss________\n";
       return s;
   }
  else
    {
      //cout<<"________________p a s o o o p p p s s s ______\n";
      return(CanonizeR(s1,i) | CanonizeR(s2,i));
    }
}
/*---------------------------  Set_Bloc()  -------*/
bool RdPBDD::Set_Bloc(bdd &S) const
{
  //cout<<"Ici detect blocage \n";
	int k=0;
	bdd Mt=bddtrue;
	  for(vector<Trans>::const_iterator i = relation.begin();	   	i!=relation.end();i++,k++)
	  {
	    //cout<<"transition :"<<transitions[k].name<<endl;
	    //cout<<"PRECOND :"<<bddtable<<(*i).Precond<<endl;
	    //cout<<"POSTCOND :"<<bddtable<<(*i).Postcond<<endl;
	    //int toto;cin>>toto;
	    Mt=Mt & !((*i).Precond);
	  }
	return ((S&Mt)!=bddfalse);
		//BLOCAGE
}
/*-------------------------Set_Div() à revoir -----------------------------*/
bool RdPBDD::Set_Div(bdd &S) const
{
	Set::const_iterator i;
	bdd To,Reached;
	//cout<<"Ici detect divergence \n";
	Reached=S;
	do
	{
	        bdd From=Reached;
		for(i=NonObservable.begin();!(i==NonObservable.end());i++)
		{

				To=relation[*i](Reached);
				Reached=Reached|To; //Reached=To ???

				//Reached=To;
		}
		if(Reached==From)
			//cout<<"SEQUENCE DIVERGENTE \n";
			return true;
		//From=Reached;
	}while(Reached!=bddfalse);
	 return false;
	//cout<<"PAS DE SEQUENCE DIVERGENTE \n";
}
/*-----------FrontiereNodes() pour bdd ---------*/
bdd RdPBDD::FrontiereNodes(bdd From) const
{
	bdd res=bddfalse;
	for(Set::const_iterator i=Observable.begin();!(i==Observable.end());i++)
			res=res | (From & relation[*i].Precond);
	for(Set::const_iterator i=InterfaceTrans.begin();!(i==InterfaceTrans.end());i++)
			res=res | (From & relation[*i].Precond);
	return res;
}

/*------------------------EmersonLey ----------------------------*/
bdd RdPBDD::EmersonLey(bdd S,bool trace)
{
	cout<<"ICI EMERSON LEY \n";
	double		TpsInit, TpsDetect;
	double debitext,finitext;
	TpsInit = (double)(clock()) / CLOCKS_PER_SEC;
	bdd b=S;
	bdd Fair=bdd_ithvar(2*Nb_places-1);
	cout<<"PLACE TEMOIN \n";
	//cout<<places[places.size()-1].name<<endl;
	bdd oldb;
	bdd oldd,d;
	int extit=0;
	int init=0;
	do
	{

		extit++;
		if(trace)
		{

			cout<<"ITERATION EXTERNES NUMERO :"<<extit<<endl;
			debitext=(double)(clock()) / CLOCKS_PER_SEC;
			cout<<"TAILLE DE B AVANT IT INTERNE : "<<bdd_nodecount(b)<<endl;
			cout<<endl<<endl;
		}
		oldb=b;
		//cout<<"Fair : "<<Fair.id()<<endl;
		d=b & Fair;
		//cout<<"d : "<<d.id()<<endl;
		//init=0;
		do
		{
			init++;
			if(trace)
			{

				cout<<"ITERATION INTERNES NUMERO :"<<init<<endl;
				cout<<"HEURE : "<<(double)(clock()) / CLOCKS_PER_SEC<<endl;
				cout<<"TAILLE DE D : "<<bdd_nodecount(d)<<endl;
			}
			oldd=d;
			bdd inter=b & StepForward2(d);
			//cout<<"Tille de inter :"<<bdd_nodecount(inter)<<endl;
			d=d | inter;
		}while(!(oldd==d));
		if(trace)
			cout<<"\nTAILLE DE D APRES ITs INTERNES : "<<bdd_nodecount(d)<<endl;
		b=b & StepBackward2(d);
		init++;
		if(trace)
		{
			cout<<"\n\nTAILLE DE B APRES ELEMINER LES PRED DE D : "<<bdd_nodecount(b)<<endl;
			finitext=(double)(clock()) / CLOCKS_PER_SEC;
			cout<<"DUREE DE L'ITERATION EXTERNE NUMERO "<<extit<<"  :  "<<finitext-debitext<<endl;
			cout<<endl<<"_________________________________________________\n\n";
		}
	}while(!(b==oldb));
	cout<<"NOMBRE D'ITERATIONS EXTERNES -----:"<<extit<<endl;
	cout<<"NOMBRE D'ITERATIONS INTERNES -----:"<<init<<endl;
	TpsDetect = ((double)(clock()) / CLOCKS_PER_SEC) - TpsInit;
	cout << "DETECTION DE CYCLE TIME  " << TpsDetect << endl;
	return b;
}

