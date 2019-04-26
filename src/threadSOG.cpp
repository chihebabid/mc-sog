//#include "test_assert.h"
//#include "sylvan_common.h"

#define GARBAGE_ENABLED
//#define DEBUG_GC
#include "threadSOG.h"


// #include <vec.h>


// #include "test_assert.h"

#include "sylvan.h"
#include "sylvan_seq.h"
#include <sylvan_sog.h>
#include <sylvan_int.h>

using namespace sylvan;



void write_to_dot(const char *ch,MDD m)
{
    FILE *fp=fopen(ch,"w");
    lddmc_fprintdot(fp,m);
    fclose(fp);
}

/*************************************************/

//#include <boost/thread.hpp>
const vector<class Place> *_places = NULL;

/*void my_error_handler_dist(int errcode) {
    cout<<"errcode = "<<errcode<<endl;
	if (errcode == BDD_RANGE) {
		// Value out of range : increase the size of the variables...
		// but which one???
		bdd_default_errhandler(errcode);
	}
	else {
		bdd_default_errhandler(errcode);
	}
} */



/*************************************************/

threadSOG::threadSOG(const net &R, int BOUND, int nbThread,bool uselace,bool init)
{
    m_nb_thread=nbThread;
    if (uselace)  {
    lace_init(m_nb_thread, 10000000);
    }
    else lace_init(1, 10000000);
    lace_startup(0, NULL, NULL);

    // Simple Sylvan initialization, also initialize MDD support
    //sylvan_set_sizes(1LL<<27, 1LL<<27, 1LL<<20, 1LL<<22);
    sylvan_set_limits(2LL<<30, 16, 3);
    //sylvan_set_sizes(1LL<<30, 2LL<<20, 1LL<<18, 1LL<<20);
    //sylvan_init_bdd(1);
    sylvan_init_package();
    sylvan_init_ldd();
    LACE_ME;
    /*sylvan_gc_hook_pregc(TASK(gc_start));
    sylvan_gc_hook_postgc(TASK(gc_end));
    sylvan_gc_enable();*/
       m_net=R;

    m_init=init;
    int  i;
    vector<Place>::const_iterator it_places;


    init_gc_seq();


    //_______________
    transitions=R.transitions;
    Observable=R.Observable;
    NonObservable=R.NonObservable;
    Formula_Trans=R.Formula_Trans;
    transitionName=R.transitionName;
    InterfaceTrans=R.InterfaceTrans;
    m_nbPlaces=R.places.size();
    cout<<"Nombre de places : "<<m_nbPlaces<<endl;
    cout<<"Derniere place : "<<R.places[m_nbPlaces-1].name<<endl;
    // place domain, place bvect, place initial marking and place name
    // domains


    // bvec


    // Computing initial marking


    uint32_t * liste_marques=new uint32_t[R.places.size()];
    for(i=0,it_places=R.places.begin(); it_places!=R.places.end(); i++,it_places++)
    {
        liste_marques[i] =it_places->marking;
    }

    M0=lddmc_cube(liste_marques,R.places.size());

   // ldd_refs_push(M0);

    // place names
    _places = &R.places;


    uint32_t *prec = new uint32_t[m_nbPlaces];
    uint32_t *postc= new uint32_t [m_nbPlaces];
    // Transition relation
    for(vector<Transition>::const_iterator t=R.transitions.begin();
            t!=R.transitions.end(); t++)
    {
        // Initialisation
        for(i=0; i<m_nbPlaces; i++)
        {
            prec[i]=0;
            postc[i]=0;
        }
        // Calculer les places adjacentes a la transition t
        set<int> adjacentPlace;
        for(vector< pair<int,int> >::const_iterator it=t->pre.begin(); it!=t->pre.end(); it++)
        {
            adjacentPlace.insert(it->first);
            prec[it->first] = prec[it->first] + it->second;
            //printf("It first %d \n",it->first);
            //printf("In prec %d \n",prec[it->first]);
        }
        // arcs post
        for(vector< pair<int,int> >::const_iterator it=t->post.begin(); it!=t->post.end(); it++)
        {
            adjacentPlace.insert(it->first);
            postc[it->first] = postc[it->first] + it->second;

        }
        for(set<int>::const_iterator it=adjacentPlace.begin(); it!=adjacentPlace.end(); it++)
        {
            MDD Precond=lddmc_true;
            Precond = Precond & ((*it) >= prec[*it]);
        }

        MDD _minus=lddmc_cube(prec,m_nbPlaces);
        ldd_refs_push(_minus);
        MDD _plus=lddmc_cube(postc,m_nbPlaces);
        ldd_refs_push(_plus);
        m_tb_relation.push_back(TransSylvan(_minus,_plus));
    }

   // sylvan_gc_seq();


    delete [] prec;
    delete [] postc;


}




//**************************************************** Version séquentielle en utilisant les LDD********************************************************************
void threadSOG::computeSeqSOG(LDDGraph &g)
{
    // m_graph=&g;
    clock_gettime(CLOCK_REALTIME, &start);

    //-------------------------------------------------------------------
    //-------------------------------------------------------------------
    int nb_failed=0;
   // cout<<"COMPUTE CANONICAL DETERMINISTIC GRAPH_________________________ \n";
    //cout<<"nb MDD var : "<<bdd_varnum()<<endl;
   // double d,tps;
    //d=(double)clock() / (double)CLOCKS_PER_SEC;

    double old_size;

    m_nbmetastate=0;
    m_MaxIntBdd=0;
    typedef pair<LDDState*,MDD> couple;
    typedef pair<couple, Set> Pair;
    typedef stack<Pair> pile;
    pile st;
    m_NbIt=0;
    m_itext=m_itint=0;
    LDDState* reached_class;
    Set fire;
    //FILE *fp=fopen("test.dot","w");
    // size_t max_meta_state_size;
    LDDState *c=new LDDState;


    // cout<<"Marquage initial :\n";
    //cout<<bddtable<<M0<<endl;
    MDD Complete_meta_state=Accessible_epsilon2(M0);
   // write_to_dot("detectDiv.dot",Complete_meta_state);
    //cout<<" div "<<Set_Div(Complete_meta_state)<<endl;
    //lddmc_fprintdot(fp,Complete_meta_state);
    //fclose(fp);

    //cout<<"Apres accessible epsilon \n";
    fire=firable_obs(Complete_meta_state);
   // MDD canonised_initial=Canonize(Complete_meta_state,0);
    //ldd_refs_push(canonised_initial);


    //c->blocage=Set_Bloc(Complete_meta_state);
    //c->boucle=Set_Div(Complete_meta_state);
    //c->m_lddstate=CanonizeR(Complete_meta_state,0);
    //cout<<"Apres CanonizeR nb representant : "<<bdd_pathcount(c->m_lddstate)<<endl;
    //c->m_lddstate=canonised_initial;
    c->m_lddstate=Complete_meta_state;
    //TabMeta[m_nbmetastate]=c->m_lddstate;
    m_nbmetastate++;
    old_size=lddmc_nodecount(c->m_lddstate);
    //max_meta_state_size=bdd_pathcount(Complete_meta_state);
    st.push(Pair(couple(c,Complete_meta_state),fire));

    g.setInitialState(c);
    g.insert(c);
    //LACE_ME;
    g.m_nbMarking+=lddmc_nodecount(c->m_lddstate);
    do
    {
        m_NbIt++;
        //cout<<"in loop"<<endl;
        Pair  e=st.top();
        st.pop();
        m_nbmetastate--;
        while(!e.second.empty())
        {
            //cout<<"in loop while"<<endl;
            int t = *e.second.begin();
            e.second.erase(t);
            reached_class=new LDDState;
            {
                //  cout<<"Avant Accessible epsilon \n";
                MDD Complete_meta_state=Accessible_epsilon2(get_successor(e.first.second,t));
                //MDD reduced_meta=Canonize(Complete_meta_state,0);
                ldd_refs_push(Complete_meta_state);
                //ldd_refs_push(reduced_meta);
                  //sylvan_gc_seq();

                reached_class->m_lddstate=Complete_meta_state;
               // reached_class->m_lddstate=reduced_meta;
                LDDState* pos=g.find(reached_class);
                //nbnode=sylvan_pathcount(reached_class->m_lddstate);
                if(!pos)
                {
                    //  cout<<"not found"<<endl;
                    reached_class->m_boucle=Set_Div(Complete_meta_state);
                  //  cout<<"Divergence "<<Set_Div(Complete_meta_state)<<endl;
                //    cout<<"blocage "<<Set_Bloc(Complete_meta_state)<<endl;

                    //reached_class->blocage=Set_Bloc(Complete_meta_state);
                    //reached_class->boucle=Set_Div(Complete_meta_state);
                    fire=firable_obs(Complete_meta_state);
                    st.push(Pair(couple(reached_class,Complete_meta_state),fire));
                    //TabMeta[nbmetastate]=reached_class->m_lddstate;
                    m_nbmetastate++;
                    //old_size=bdd_anodecount(TabMeta,nbmetastate);
                    e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(reached_class,t));
                    reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(e.first.first,t));
                    g.addArc();
                    g.insert(reached_class);
                }
                else
                {
                    //  cout<<" found"<<endl;
                    nb_failed++;
                    delete reached_class;
                    e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(pos,t));
                    pos->Predecessors.insert(pos->Predecessors.begin(),LDDEdge(e.first.first,t));
                    g.addArc();
                }
            }

        }
    }
    while(!st.empty());


    clock_gettime(CLOCK_REALTIME, &finish);
    //tps=(double)clock() / (double)CLOCKS_PER_SEC-d;
   // cout<<"TIME OF CONSTRUCTION : "<<tps<<" second"<<endl;
   // cout<<" MAXIMAL INTERMEDIARY MDD SIZE \n"<<m_MaxIntBdd<<endl;
    //cout<<"OLD SIZE : "<<old_size<<endl;
    //cout<<"NB SHARED NODES : "<<bdd_anodecount(TabMeta,nbmetastate)<<endl;
    cout<<"NB META STATE DANS CONSTRUCTION : "<<m_nbmetastate<<endl;
   // cout<<"NB ITERATIONS CONSTRUCTION : "<<m_NbIt<<endl;
  //  cout<<"Nb Iteration externes : "<<m_itext<<endl;
   // cout<<"Nb Iteration internes : "<<m_itint<<endl;
   // cout<<"Nb failed :"<<nb_failed<<endl;
    double tps;
    tps = (finish.tv_sec - start.tv_sec);
    tps += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    std::cout << "TIME OF CONSTRUCTION OF THE SOG " << tps << " seconds\n";


}
/************ Non Canonised construction with pthread ***********************************/
void * threadSOG::doCompute()
{
    int id_thread;
    int nb_it=0,nb_failed=0,max_succ=0;
    pthread_mutex_lock(&m_mutex);
    id_thread=m_id_thread++;
    pthread_mutex_unlock(&m_mutex);

    Set fire;
    if (id_thread==0)
    {
        clock_gettime(CLOCK_REALTIME, &start);
      //  printf("*******************PARALLEL*******************\n");
        m_min_charge=0;
        m_nbmetastate=0;
        m_MaxIntBdd=0;
        m_NbIt=0;
        m_itext=m_itint=0;


        LDDState *c=new LDDState;

        //cout<<"Marquage initial is being built..."<<endl;
        // cout<<bddtable<<M0<<endl;

        MDD Complete_meta_state(Accessible_epsilon(M0));
        ldd_refs_push(Complete_meta_state);
       // MDD canonised_initial=Canonize(Complete_meta_state,0);
        //ldd_refs_push(canonised_initial);
        fire=firable_obs(Complete_meta_state);

        //c->blocage=Set_Bloc(Complete_meta_state);
        //c->boucle=Set_Div(Complete_meta_state);
        //c->m_lddstate=CanonizeR(Complete_meta_state,0);
        //cout<<"Apres CanonizeR nb representant : "<<bdd_pathcount(c->m_lddstate)<<endl;
        //c->m_lddstate=canonised_initial;
        c->m_lddstate=Complete_meta_state;
        //m_TabMeta[m_nbmetastate]=c->m_lddstate;
        m_nbmetastate++;
        m_old_size=lddmc_nodecount(c->m_lddstate);
        //max_meta_state_size=bdd_pathcount(Complete_meta_state);

        m_st[0].push(Pair(couple(c,Complete_meta_state),fire));
        m_graph->setInitialState(c);
        m_graph->insert(c);
        //m_graph->nbMarking+=bdd_pathcount(c->m_lddstate);
        m_charge[0]=1;

    }

    LDDState* reached_class;

    // size_t max_meta_state_size;
    // int min_charge;
    do
    {

        while (!m_st[id_thread].empty())
        {

            nb_it++;
            m_terminaison[id_thread]=false;
            pthread_spin_lock(&m_spin_stack[id_thread]);
            Pair  e=m_st[id_thread].top();
            //pthread_spin_lock(&m_accessible);
            m_st[id_thread].pop();

            pthread_spin_unlock(&m_spin_stack[id_thread]);
            m_nbmetastate--;

            m_charge[id_thread]--;
            while(!e.second.empty())
            {
                int t = *e.second.begin();
                e.second.erase(t);
                reached_class=new LDDState();
                if (id_thread)
                {
                    pthread_mutex_lock(&m_supervise_gc_mutex);
                    m_gc++;
                    if (m_gc==1) {
                        pthread_mutex_lock(&m_gc_mutex);
                    }
                    pthread_mutex_unlock(&m_supervise_gc_mutex);
                }


                MDD Complete_meta_state=Accessible_epsilon(get_successor(e.first.second,t));

                ldd_refs_push(Complete_meta_state);

                //MDD reduced_meta=Canonize(Complete_meta_state,0);
                //ldd_refs_push(reduced_meta);

                if (id_thread==0)
                {
                    pthread_mutex_lock(&m_gc_mutex);
               //     #ifdef DEBUG_GC
                 //   displayMDDTableInfo();
                 //   #endif // DEBUG_GC
                    sylvan_gc_seq();
                 //   #ifdef DEBUG_GC
                 //   displayMDDTableInfo();
                 //   #endif // DEBUG_GC
                    pthread_mutex_unlock(&m_gc_mutex);
                }
                reached_class->m_lddstate=Complete_meta_state;
                //reached_class->m_lddstate=reduced_meta;
                //nbnode=bdd_pathcount(reached_class->m_lddstate);

                //pthread_spin_lock(&m_accessible);
                pthread_mutex_lock(&m_graph_mutex);

                LDDState* pos=m_graph->find(reached_class);

                if(!pos)
                {
                    //  cout<<"not found"<<endl;
                    //reached_class->blocage=Set_Bloc(Complete_meta_state);
                    //reached_class->boucle=Set_Div(Complete_meta_state);

                    m_graph->addArc();
                    m_graph->insert(reached_class);
                    pthread_mutex_unlock(&m_graph_mutex);

                    e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(reached_class,t));
                    reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(e.first.first,t));
                    m_nbmetastate++;
                    //pthread_mutex_lock(&m_mutex);
                    fire=firable_obs(Complete_meta_state);
                    //if (max_succ<fire.size()) max_succ=fire.size();
                    //pthread_mutex_unlock(&m_mutex);
                    m_min_charge= minCharge();
                    //m_min_charge=(m_min_charge+1) % m_nb_thread;
                    pthread_spin_lock(&m_spin_stack[m_min_charge]);
                    m_st[m_min_charge].push(Pair(couple(reached_class,Complete_meta_state),fire));
                    pthread_spin_unlock(&m_spin_stack[m_min_charge]);
                    m_charge[m_min_charge]++;
                }
                else
                {
                    nb_failed++;
                    m_graph->addArc();
                    pthread_mutex_unlock(&m_graph_mutex);

                    e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(pos,t));
                    pos->Predecessors.insert(pos->Predecessors.begin(),LDDEdge(e.first.first,t));
                    delete reached_class;

                }
                if (id_thread)
                {
                    pthread_mutex_lock(&m_supervise_gc_mutex);
                    m_gc--;
                    if (m_gc==0) pthread_mutex_unlock(&m_gc_mutex);
                    pthread_mutex_unlock(&m_supervise_gc_mutex);
                }
            }
        }
        m_terminaison[id_thread]=true;



    }
    while (isNotTerminated());
    //cout<<"Thread :"<<id_thread<<"  has performed "<<nb_it<<" itérations avec "<<nb_failed<<" échecs"<<endl;
    //cout<<"Max succ :"<<max_succ<<endl;
}

/************ Canonised construction based on Pthread library ***********************************/
void * threadSOG::doComputeCanonized()
{
    int id_thread;
    int nb_it=0,nb_failed=0,max_succ=0;
    pthread_mutex_lock(&m_mutex);
    id_thread=m_id_thread++;
    pthread_mutex_unlock(&m_mutex);

    Set fire;
    if (id_thread==0)
    {
        clock_gettime(CLOCK_REALTIME, &start);
      //  printf("*******************PARALLEL*******************\n");
        m_min_charge=0;
        m_nbmetastate=0;
        m_MaxIntBdd=0;
        m_NbIt=0;
        m_itext=m_itint=0;


        LDDState *c=new LDDState;

        //cout<<"Marquage initial is being built..."<<endl;
        // cout<<bddtable<<M0<<endl;

        MDD Complete_meta_state(Accessible_epsilon(M0));
        ldd_refs_push(Complete_meta_state);
        MDD canonised_initial=Canonize(Complete_meta_state,0);
        ldd_refs_push(canonised_initial);
        fire=firable_obs(Complete_meta_state);

        //c->blocage=Set_Bloc(Complete_meta_state);
        //c->boucle=Set_Div(Complete_meta_state);
        //c->m_lddstate=CanonizeR(Complete_meta_state,0);
        //cout<<"Apres CanonizeR nb representant : "<<bdd_pathcount(c->m_lddstate)<<endl;
        //c->m_lddstate=canonised_initial;
        c->m_lddstate=canonised_initial;
        //m_TabMeta[m_nbmetastate]=c->m_lddstate;
        m_nbmetastate++;
        m_old_size=lddmc_nodecount(c->m_lddstate);
        //max_meta_state_size=bdd_pathcount(Complete_meta_state);

        m_st[0].push(Pair(couple(c,Complete_meta_state),fire));
        m_graph->setInitialState(c);
        m_graph->insert(c);
        //m_graph->nbMarking+=bdd_pathcount(c->m_lddstate);
        m_charge[0]=1;

    }

    LDDState* reached_class;

    // size_t max_meta_state_size;
    // int min_charge;
    do
    {

        while (!m_st[id_thread].empty())
        {

            nb_it++;
            m_terminaison[id_thread]=false;
            pthread_spin_lock(&m_spin_stack[id_thread]);
            Pair  e=m_st[id_thread].top();
            //pthread_spin_lock(&m_accessible);
            m_st[id_thread].pop();

            pthread_spin_unlock(&m_spin_stack[id_thread]);
            m_nbmetastate--;

            m_charge[id_thread]--;
            while(!e.second.empty())
            {
                int t = *e.second.begin();
                e.second.erase(t);
                reached_class=new LDDState();
                if (id_thread)
                {
                    pthread_mutex_lock(&m_supervise_gc_mutex);
                    m_gc++;
                    if (m_gc==1) {
                        pthread_mutex_lock(&m_gc_mutex);
                    }
                    pthread_mutex_unlock(&m_supervise_gc_mutex);
                }


                MDD Complete_meta_state=Accessible_epsilon(get_successor(e.first.second,t));

                ldd_refs_push(Complete_meta_state);

                MDD reduced_meta=Canonize(Complete_meta_state,0);
                ldd_refs_push(reduced_meta);

                if (id_thread==0)
                {
                    pthread_mutex_lock(&m_gc_mutex);
               //     #ifdef DEBUG_GC
                 //   displayMDDTableInfo();
                 //   #endif // DEBUG_GC
                    sylvan_gc_seq();
                 //   #ifdef DEBUG_GC
                 //   displayMDDTableInfo();
                 //   #endif // DEBUG_GC
                    pthread_mutex_unlock(&m_gc_mutex);
                }
                reached_class->m_lddstate=reduced_meta;
                //reached_class->m_lddstate=reduced_meta;
                //nbnode=bdd_pathcount(reached_class->m_lddstate);

                //pthread_spin_lock(&m_accessible);
                pthread_mutex_lock(&m_graph_mutex);

                LDDState* pos=m_graph->find(reached_class);

                if(!pos)
                {
                    //  cout<<"not found"<<endl;
                    //reached_class->blocage=Set_Bloc(Complete_meta_state);
                    //reached_class->boucle=Set_Div(Complete_meta_state);

                    m_graph->addArc();
                    m_graph->insert(reached_class);
                    pthread_mutex_unlock(&m_graph_mutex);

                    e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(reached_class,t));
                    reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(e.first.first,t));
                    m_nbmetastate++;
                    //pthread_mutex_lock(&m_mutex);
                    fire=firable_obs(Complete_meta_state);
                    //if (max_succ<fire.size()) max_succ=fire.size();
                    //pthread_mutex_unlock(&m_mutex);
                    m_min_charge= minCharge();
                    //m_min_charge=(m_min_charge+1) % m_nb_thread;
                    pthread_spin_lock(&m_spin_stack[m_min_charge]);
                    m_st[m_min_charge].push(Pair(couple(reached_class,Complete_meta_state),fire));
                    pthread_spin_unlock(&m_spin_stack[m_min_charge]);
                    m_charge[m_min_charge]++;
                }
                else
                {
                    nb_failed++;
                    m_graph->addArc();
                    pthread_mutex_unlock(&m_graph_mutex);

                    e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(pos,t));
                    pos->Predecessors.insert(pos->Predecessors.begin(),LDDEdge(e.first.first,t));
                    delete reached_class;

                }
                if (id_thread)
                {
                    pthread_mutex_lock(&m_supervise_gc_mutex);
                    m_gc--;
                    if (m_gc==0) pthread_mutex_unlock(&m_gc_mutex);
                    pthread_mutex_unlock(&m_supervise_gc_mutex);
                }
            }
        }
        m_terminaison[id_thread]=true;



    }
    while (isNotTerminated());
    //cout<<"Thread :"<<id_thread<<"  has performed "<<nb_it<<" itérations avec "<<nb_failed<<" échecs"<<endl;
    //cout<<"Max succ :"<<max_succ<<endl;
}



bool threadSOG::isNotTerminated()
{
    bool res=true;
    int i=0;
    while (i<m_nb_thread && res)
    {
        res=m_terminaison[i];
        i++;
    }
    return !res;
}
void threadSOG::computeDSOG(LDDGraph &g,bool canonised)
{

    cout << "number of threads "<<m_nb_thread<<endl;
    int rc;
    m_graph=&g;
    m_id_thread=0;

    pthread_mutex_init(&m_mutex, NULL);
    pthread_mutex_init(&m_gc_mutex,NULL);
    pthread_mutex_init(&m_graph_mutex,NULL);
    pthread_mutex_init(&m_supervise_gc_mutex,NULL);
    m_gc=0;
    for (int i=0; i<m_nb_thread; i++)
    {
        pthread_spin_init(&m_spin_stack[i], NULL);
        m_charge[i]=0;
        m_terminaison[i]=false;
    }

    for (int i=0; i<m_nb_thread-1; i++)
    {
        if ((rc = pthread_create(&m_list_thread[i], NULL,canonised?threadHandlerCanonized:threadHandler,this)))
        {
            cout<<"error: pthread_create, rc: "<<rc<<endl;
        }
    }

    if (canonised) doComputeCanonized();
    else doCompute();
    for (int i = 0; i < m_nb_thread-1; i++)
    {
        pthread_join(m_list_thread[i], NULL);
    }
    clock_gettime(CLOCK_REALTIME, &finish);

    double tps;

    cout<<"Nb Threads : "<<m_nb_thread<<endl;
    tps = (finish.tv_sec - start.tv_sec);
    tps += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    std::cout << "TIME OF CONSTRUCTION OF THE SOG " << tps << " seconds\n";

}
void * threadSOG::threadHandler(void *context)
{
    return ((threadSOG*)context)->doCompute();
}

void * threadSOG::threadHandlerCanonized(void *context)
{
    return ((threadSOG*)context)->doComputeCanonized();
}


void threadSOG::printhandler(ostream &o, int var)
{
    o << (*_places)[var/2].name;
    if (var%2)
        o << "_p";
}










Set threadSOG::firable_obs(MDD State)
{
    Set res;
    for(Set::const_iterator i=Observable.begin(); !(i==Observable.end()); i++)
    {
        //cout<<"firable..."<<endl;
        MDD succ = lddmc_firing_mono(State,m_tb_relation[(*i)].getMinus(),m_tb_relation[(*i)].getPlus());
        if(succ!=lddmc_false)
        {
            res.insert(*i);
        }

    }
    return res;
}


MDD threadSOG::get_successor(MDD From,int t)
{
    return lddmc_firing_mono(From,m_tb_relation[(t)].getMinus(),m_tb_relation[(t)].getPlus());
}

MDD threadSOG::get_successor2(MDD From,int t)
{
  //  LACE_ME;
    return lddmc_firing_mono(From,m_tb_relation[(t)].getMinus(),m_tb_relation[(t)].getPlus());
}

MDD threadSOG::Accessible_epsilon2(MDD From)
{
    MDD M1;
    MDD M2=From;
    int it=0;


    do
    {

        M1=M2;
        for(Set::const_iterator i=NonObservable.begin(); !(i==NonObservable.end()); i++)
        {    LACE_ME;

            MDD succ= lddmc_firing_mono(M2,m_tb_relation[(*i)].getMinus(),m_tb_relation[(*i)].getPlus());
            M2=lddmc_union(succ,M2);

        }

        it++;

        //	cout << bdd_nodecount(M2) << endl;
    }
    while(M1!=M2);

    return M2;
}

MDD threadSOG::Accessible_epsilon(MDD From)
{
    MDD M1;
    MDD M2=From;
    int it=0;


    do
    {

        M1=M2;
        for(Set::const_iterator i=NonObservable.begin(); !(i==NonObservable.end()); i++)
        {

            MDD succ= lddmc_firing_mono(M2,m_tb_relation[(*i)].getMinus(),m_tb_relation[(*i)].getPlus());
            M2=lddmc_union_mono(succ,M2);

        }

        it++;

        //	cout << bdd_nodecount(M2) << endl;
    }
    while(M1!=M2);

    return M2;
}




int threadSOG::minCharge()
{
    int pos=0;
    int min_charge=m_charge[0];
    for (int i=1; i<m_nb_thread; i++)
    {
        if (m_charge[i]<min_charge)
        {
            min_charge=m_charge[i];
            pos=i;
        }
    }
    return pos;

}


/*---------------------------  Set_Bloc()  -------*/
/*bool DistributedSOG::Set_Bloc(MDD &S) const {

 cout<<"Ici detect blocage \n";
int check_deadlocks = 0
MDD cur;
  for(vector<TransSylvan>::const_iterator i = m_tb_relation.begin(); i!=m_tb_relation.end();i++)
  {

      cur=TransSylvan(*i).getMinus();

  }
return ((S&cur)!=lddmc_false);
	//BLOCAGE
}


/*-------------------------Set_Div() à revoir -----------------------------*/
/*bool threadSOG::Set_Div(MDD &S) const
{
	Set::const_iterator i;
	MDD To,Reached;
	//cout<<"Ici detect divergence \n";
	Reached=S;
	do
	{
	        MDD From=Reached;
		for(i=NonObservable.begin();!(i==NonObservable.end());i++)
		{

                To=lddmc_firing_mono(Reached,m_tb_relation[(*i)].getMinus(),m_tb_relation[(*i)].getPlus());
                Reached=lddmc_union_mono(Reached,To);
				//To=m_tb_relation[*i](Reached);
				//Reached=Reached|To; //Reached=To ???
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
*/

MDD threadSOG::Canonize(MDD s,unsigned int level)
{
    if (level>m_nbPlaces || s==lddmc_false)
        return lddmc_false;
    if(isSingleMDD(s))
        return s;
    MDD s0=lddmc_false,s1=lddmc_false;

    bool res=false;
    do
    {
        if (get_mddnbr(s,level)>1)
        {
            s0=ldd_divide_rec(s,level);
            s1=ldd_minus(s,s0);
            res=true;
        }
        else
            level++;
    }
    while(level<m_nbPlaces && !res);


    if (s0==lddmc_false && s1==lddmc_false)
        return lddmc_false;
    // if (level==nbPlaces) return lddmc_false;
    MDD Front,Reach;
    if (s0!=lddmc_false && s1!=lddmc_false)
    {
        Front=s1;
        Reach=s1;
        do
        {
            // cout<<"premiere boucle interne \n";
            Front=ldd_minus(ImageForward(Front),Reach);
            Reach = lddmc_union_mono(Reach,Front);
            s0 = ldd_minus(s0,Front);
        }
        while((Front!=lddmc_false)&&(s0!=lddmc_false));
    }
    if((s0!=lddmc_false)&&(s1!=lddmc_false))
    {
        Front=s0;
        Reach = s0;
        do
        {
            //  cout<<"deuxieme boucle interne \n";
            Front=ldd_minus(ImageForward(Front),Reach);
            Reach = lddmc_union_mono(Reach,Front);
            s1 = ldd_minus(s1,Front);
        }
        while( Front!=lddmc_false && s1!=lddmc_false );
    }



    MDD Repr=lddmc_false;

    if (isSingleMDD(s0))
    {
        Repr=s0;
    }
    else
    {

        Repr=Canonize(s0,level);

    }

    if (isSingleMDD(s1))
        Repr=lddmc_union_mono(Repr,s1);
    else
        Repr=lddmc_union_mono(Repr,Canonize(s1,level));


    return Repr;


}
MDD threadSOG::ImageForward(MDD From)
{
    MDD Res=lddmc_false;
    for(Set::const_iterator i=NonObservable.begin(); !(i==NonObservable.end()); i++)
    {
        MDD succ= lddmc_firing_mono(From,m_tb_relation[(*i)].getMinus(),m_tb_relation[(*i)].getPlus());
        //        bdd succ = relation[(*i)](From);
        Res=lddmc_union_mono(Res,succ);
        // Res=Res|succ;

    }
    return Res;
}


bool threadSOG::Set_Bloc(MDD &M) const
{
  //bdd Mt = lddmc_true;
  //for(vector<Trans>::const_iterator i = relation.begin(); i != relation.end(); ++i) {
   // Mt = Mt & !(i->Precond);
  //}
  //return ((S & Mt) != bddfalse);
   cout<<"Ici detect blocage \n";
//int check_deadlocks = 0
   MDD cur=lddmc_true;
  for(vector<TransSylvan>::const_iterator i = m_tb_relation.begin(); i!=m_tb_relation.end();i++)
  {

      cur=cur&(TransSylvan(*i).getMinus());

  }
return ((M&cur)!=lddmc_false);
	//BLOCAGE
}
bool threadSOG::Set_Div2( const MDD &M) const
{
    if (NonObservable.empty())
    return false;
	Set::const_iterator i;
	MDD Reached,From;
	//cout<<"Ici detect divergence \n";
	Reached=M;
	do
	{
        From=Reached;
		for(i=NonObservable.begin();!(i==NonObservable.end());i++)
		{
		// To=relation[*i](Reached);
        // Reached=Reached|To; //Reached=To ???
        MDD To= lddmc_firing_mono(From,m_tb_relation[(*i)].getMinus(),m_tb_relation[(*i)].getPlus());
        Reached=lddmc_union_mono(Reached,To);
				//Reached=To;
		}
		//From=Reached;
	}while(Reached!=lddmc_false && Reached != From);
   //  cout<<"PAS DE SEQUENCE DIVERGENTE \n";
	 return Reached != lddmc_false;
}

//}
bool threadSOG::Set_Div(MDD &M) const
{
    if (NonObservable.empty())
    return false;
	Set::const_iterator i;
	MDD Reached,From;
	//cout<<"Ici detect divergence \n";
	Reached=M;
	do
	{
        From=Reached;
		for(i=NonObservable.begin();!(i==NonObservable.end());i++)
		{
		// To=relation[*i](Reached);
        // Reached=Reached|To; //Reached=To ???
        MDD To= lddmc_firing_mono(From,m_tb_relation[(*i)].getMinus(),m_tb_relation[(*i)].getPlus());
        Reached=lddmc_union_mono(Reached,To);
				//Reached=To;
		}
		if(Reached==From)
		//{
        //cout << " sequence divergente "<<endl;
        return true;
		//}
		//From=Reached;
	}while(Reached!=lddmc_false && Reached != From);
   //  cout<<"PAS DE SEQUENCE DIVERGENTE \n";
	 return false;
}

threadSOG::~threadSOG()
{
    //dtor
}

/*********Returns the count of places******************************************/
unsigned int threadSOG::getPlacesCount() {
    return m_nbPlaces;
}




/******************* Functions computing SOG with Lace *****************/

/***** Saturation with Lace *********/

TASK_3 (MDD, ImageForwardLace,MDD, From, Set* , NonObservable, vector<TransSylvan>*, tb_relation)
{
    MDD Res=lddmc_false;

    for(Set::const_iterator i=NonObservable->begin(); !(i==NonObservable->end()); i++)
    {
         lddmc_refs_spawn(SPAWN(lddmc_firing_lace,From,(*tb_relation)[(*i)].getMinus(),(*tb_relation)[(*i)].getPlus()));
    }
    for(Set::const_iterator i=NonObservable->begin(); !(i==NonObservable->end()); i++)
    {
        MDD succ=lddmc_refs_sync(SYNC(lddmc_firing_lace));
        Res=lddmc_union(Res,succ);
    }
    return Res;
}

#define ImageForwardLace(state,nonobser,tb) CALL(ImageForwardLace, state, nonobser,tb)

TASK_3 (Set, firable_obs_lace,MDD, State, Set*, observable, vector<TransSylvan>*, tb_relation)
{
    Set res;
    for(Set::const_iterator i=observable->begin(); !(i==observable->end()); i++)
    {
        MDD succ = lddmc_firing_lace(State,(*tb_relation)[(*i)].getMinus(),(*tb_relation)[(*i)].getPlus());
        if(succ!=lddmc_false)
        {
            res.insert(*i);
        }

    }
    return res;
}

#define firable_obs_lace(state,obser,tb) CALL(firable_obs_lace, state, obser,tb)

TASK_3 (MDD, Accessible_epsilon_lace, MDD, From, Set*, nonObservable, vector<TransSylvan>*, tb_relation)
{
    MDD M1;
    MDD M2=From;
    int it=0;
    //cout<<"worker "<<lace_get_worker()->worker<<endl;
    do
    {
        M1=M2;
        for(Set::const_iterator i=nonObservable->begin(); !(i==nonObservable->end()); i++)
        {
            SPAWN(lddmc_firing_lace,M2,(*tb_relation)[(*i)].getMinus(),(*tb_relation)[(*i)].getPlus());
        }
        for(Set::const_iterator i=nonObservable->begin(); !(i==nonObservable->end()); i++)
        {
            MDD succ=SYNC(lddmc_firing_lace);
            M2=lddmc_union(succ,M2);
        }

    }
    while(M1!=M2);
    return M2;
}


/********************* Compute SOG with lace *********************************************/
void threadSOG::computeSOGLace(LDDGraph &g)
{
    clock_gettime(CLOCK_REALTIME, &start);
    Set fire;
    m_graph=&g;
    m_nbmetastate=0;
    m_MaxIntBdd=0;


    LDDState *c=new LDDState;

    //cout<<"Marquage initial is being built..."<<endl;
    LACE_ME;
    MDD initial_meta_state(CALL(Accessible_epsilon_lace,M0,&NonObservable,&m_tb_relation));


    fire=firable_obs_lace(initial_meta_state,&Observable,&m_tb_relation);

    c->m_lddstate=initial_meta_state;

    m_nbmetastate++;
    m_old_size=lddmc_nodecount(c->m_lddstate);

    //max_meta_state_size=bdd_pathcount(Complete_meta_state);
    m_st[0].push(Pair(couple(c,initial_meta_state),fire));
    m_graph->setInitialState(c);
    m_graph->insert(c);
    //m_graph->nbMarking+=bdd_pathcount(c->m_lddstate);
    LDDState* reached_class;
    //  MDD Complete_meta_state;*/

    // size_t max_meta_state_size;
    // int min_charge;

    while (!m_st[0].empty())
    {

        Pair  e=m_st[0].top();
        m_st[0].pop();
        m_nbmetastate--;
        unsigned int onb_it=0;
        Set::const_iterator iter=e.second.begin();
        while(iter!=e.second.end())
        {
            int t = *iter;
            //e.second.erase(t);
            SPAWN(Accessible_epsilon_lace,get_successor(e.first.second,t),&NonObservable,&m_tb_relation);
            onb_it++;iter++;
        }

        for (unsigned int i=0; i<onb_it; i++)
        {
            int t = *e.second.end();
            e.second.erase(t);
            MDD Complete_meta_state=SYNC(Accessible_epsilon_lace);
            reached_class=new LDDState;
            reached_class->m_lddstate=Complete_meta_state;
            LDDState* pos=m_graph->find(reached_class);

            if(!pos)
            {

                m_graph->addArc();
                m_graph->insert(reached_class);
                e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(reached_class,t));
                reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(e.first.first,t));
                m_nbmetastate++;
                fire=firable_obs_lace(Complete_meta_state,&Observable,&m_tb_relation);
                m_st[0].push(Pair(couple(reached_class,Complete_meta_state),fire));
            }
            else
            {
                m_graph->addArc();
                e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(pos,t));
                pos->Predecessors.insert(pos->Predecessors.begin(),LDDEdge(e.first.first,t));
                delete reached_class;
            }
        }
    }
    clock_gettime(CLOCK_REALTIME, &finish);
    double tps;
    tps = (finish.tv_sec - start.tv_sec);
    tps += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    std::cout << "TIME OF CONSTRUCTION OF THE SOG " << tps << " seconds\n";
}

Set * threadSOG::getNonObservable() {
    return &NonObservable;
}
vector<TransSylvan>* threadSOG::getTBRelation() {
    return &m_tb_relation;
}
/********************* Compute canonized SOG with lace *********************************************/

/******************************Canonizer with lace framework  **********************************/
TASK_DECL_3(MDD, lddmc_canonize,MDD, unsigned int, threadSOG &)
#define lddmc_canonize(s,level,ds) CALL(lddmc_canonize, s, level,ds)


TASK_IMPL_3(MDD, lddmc_canonize,MDD, s,unsigned int, level, threadSOG & ,ds)
{

    if (level>ds.getPlacesCount() || s==lddmc_false)
        return lddmc_false;
    if(isSingleMDD(s))
        return s;
    MDD s0=lddmc_false,s1=lddmc_false;

    bool res=false;
    do
    {
        if (get_mddnbr(s,level)>1)
        {
            s0=ldd_divide_rec(s,level);
            s1=lddmc_minus(s,s0);
            res=true;
        }
        else
            level++;
    }
    while(level<ds.getPlacesCount() && !res);


    if (s0==lddmc_false && s1==lddmc_false)
        return lddmc_false;
    // if (level==nbPlaces) return lddmc_false;
    MDD Front,Reach;
    if (s0!=lddmc_false && s1!=lddmc_false)
    {
        Front=s1;
        Reach=s1;
        do
        {
            // cout<<"premiere boucle interne \n";
            Front=lddmc_minus(ImageForwardLace(Front,ds.getNonObservable(),ds.getTBRelation()),Reach);
            lddmc_refs_spawn(SPAWN(lddmc_union,Reach,Front));
            s0=CALL(lddmc_minus,s0,Front);
            Reach=lddmc_refs_sync(SYNC(lddmc_union));

        }
        while((Front!=lddmc_false)&&(s0!=lddmc_false));
    }
    if((s0!=lddmc_false)&&(s1!=lddmc_false))
    {
        Front=s0;
        Reach = s0;
        do
        {
            //  cout<<"deuxieme boucle interne \n";
            Front=lddmc_minus(ImageForwardLace(Front,ds.getNonObservable(),ds.getTBRelation()),Reach);
            lddmc_refs_spawn(SPAWN(lddmc_union,Reach,Front));
            s1=CALL(lddmc_minus,s1,Front);
            Reach=lddmc_refs_sync(SYNC(lddmc_union));

        }
        while( Front!=lddmc_false && s1!=lddmc_false );
    }
    MDD Repr=lddmc_false;

    if (isSingleMDD(s0))
    {
        Repr=s0;
        if (isSingleMDD(s1))
            Repr=lddmc_union(Repr,s1);
        else
            Repr=lddmc_union(Repr,lddmc_canonize(s1,level,ds));
    }
    else
    {

        if (isSingleMDD(s1)) {
            /*lddmc_refs_spawn(SPAWN(lddmc_canonize,s0,level,ds));
            Repr=lddmc_refs_sync(SYNC(lddmc_canonize));*/
            Repr=CALL(lddmc_canonize,s0,level,ds);
            Repr=lddmc_union(Repr,s1);
            }
        else {
            //lddmc_refs_spawn(SPAWN(lddmc_canonize,s0,level,ds));
            Repr=CALL(lddmc_canonize,s0,level,ds);
            MDD temp=lddmc_canonize(s1,level,ds);
            //MDD temp2=lddmc_refs_sync(SYNC(lddmc_canonize));
            Repr=lddmc_union(Repr,temp);
            }

    }
    return Repr;
}





void threadSOG::computeSOGLaceCanonized(LDDGraph &g)
{
    clock_gettime(CLOCK_REALTIME, &start);
    Set fire;
    m_graph=&g;


    m_nbmetastate=0;
    m_MaxIntBdd=0;


    LDDState *c=new LDDState;

    //cout<<"Marquage initial is being built..."<<endl;
    LACE_ME;
    MDD initial_meta_state(CALL(Accessible_epsilon_lace,M0,&NonObservable,&m_tb_relation));

    //lddmc_refs_spawn(SPAWN(lddmc_canonize,initial_meta_state,0,*this));


    fire=firable_obs_lace(initial_meta_state,&Observable,&m_tb_relation);
    m_nbmetastate++;
    //m_old_size=lddmc_nodecount(c->m_lddstate);
    //reduced_initial=lddmc_refs_sync(SYNC(lddmc_canonize));

    c->m_lddstate=CALL(lddmc_canonize,initial_meta_state,0,*this);
    //max_meta_state_size=bdd_pathcount(Complete_meta_state);
    m_st[0].push(Pair(couple(c,initial_meta_state),fire));
    m_graph->setInitialState(c);
    m_graph->insert(c);
    //m_graph->nbMarking+=bdd_pathcount(c->m_lddstate);
    LDDState* reached_class;
    //  MDD Complete_meta_state;*/
cout<<"\n=========================================\n";

    while (!m_st[0].empty())
    {

        Pair  e=m_st[0].top();
        m_st[0].pop();
        m_nbmetastate--;
        unsigned int onb_it=0;
        Set::const_iterator iter=e.second.begin();
        while(iter!=e.second.end())
        {
            int t = *iter;
            //e.second.erase(t);
            SPAWN(Accessible_epsilon_lace,get_successor(e.first.second,t),&NonObservable,&m_tb_relation);
            onb_it++;iter++;
        }

        for (unsigned int i=0; i<onb_it; i++)
        {
            int t = *e.second.end();
            e.second.erase(t);
            MDD Complete_meta_state=SYNC(Accessible_epsilon_lace);
            lddmc_refs_push(Complete_meta_state);


            reached_class=new LDDState;

            reached_class->m_lddstate=lddmc_canonize(Complete_meta_state,0,*this);

            LDDState* pos=m_graph->find(reached_class);

            if(!pos)
            {

                m_graph->addArc();
                m_graph->insert(reached_class);
                e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(reached_class,t));
                reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(e.first.first,t));
                m_nbmetastate++;
                fire=firable_obs_lace(Complete_meta_state,&Observable,&m_tb_relation);
                m_st[0].push(Pair(couple(reached_class,Complete_meta_state),fire));
            }
            else
            {
                m_graph->addArc();
                e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(pos,t));
                pos->Predecessors.insert(pos->Predecessors.begin(),LDDEdge(e.first.first,t));
                delete reached_class;
            }
            lddmc_refs_pop(1);
        }
    }
    clock_gettime(CLOCK_REALTIME, &finish);
    double tps;
    tps = (finish.tv_sec - start.tv_sec);
    tps += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    std::cout << "TIME OF CONSTRUCTION OF THE SOG " << tps << " seconds\n";
}


