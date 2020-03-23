//#include "test_assert.h"
//#include "sylvan_common.h"

#define GARBAGE_ENABLED
//#define DEBUG_GC
#include "threadSOG.h"

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




/*************************************************/

threadSOG::threadSOG(const NewNet &R, int nbThread,bool uselace,bool init)
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
       m_net=&R;

    m_init=init;
    int  i;
    vector<Place>::const_iterator it_places;


    init_gc_seq();


    //_______________
    transitions=R.transitions;
    m_observable=R.Observable;
    m_place_proposition=R.m_formula_place;
    m_nonObservable=R.NonObservable;

    m_transitionName=R.transitionName;
    m_placeName=R.m_placePosName;

    cout<<"Toutes les Transitions:"<<endl;
    map<string,uint16_t>::iterator it2=m_transitionName.begin();
    for (;it2!=m_transitionName.end();it2++) {
        cout<<(*it2).first<<" : "<<(*it2).second<<endl;}



    cout<<"Transitions observables :"<<endl;
    Set::iterator it=m_observable.begin();
    for (;it!=m_observable.end();it++) {cout<<*it<<"  ";}
    cout<<endl;
    InterfaceTrans=R.InterfaceTrans;
    m_nbPlaces=R.places.size();
    cout<<"Nombre de places : "<<m_nbPlaces<<endl;
    cout<<"Derniere place : "<<R.places[m_nbPlaces-1].name<<endl;

    uint32_t * liste_marques=new uint32_t[R.places.size()];
    for(i=0,it_places=R.places.begin(); it_places!=R.places.end(); i++,it_places++)
    {
        liste_marques[i] =it_places->marking;
    }

    M0=lddmc_cube(liste_marques,R.places.size());





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




//**************************************************** Version s�quentielle en utilisant les LDD********************************************************************
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
    MDD Complete_meta_state=Accessible_epsilon(M0);
   // write_to_dot("detectDiv.dot",Complete_meta_state);
    //cout<<" div "<<Set_Div(Complete_meta_state)<<endl;
    //lddmc_fprintdot(fp,Complete_meta_state);
    //fclose(fp);

    //cout<<"Apres accessible epsilon \n";
    fire=firable_obs(Complete_meta_state);

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
                MDD Complete_meta_state=Accessible_epsilon(get_successor(e.first.second,t));
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
    //cout<<"Thread :"<<id_thread<<"  has performed "<<nb_it<<" it�rations avec "<<nb_failed<<" �checs"<<endl;
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
    //cout<<"Thread :"<<id_thread<<"  has performed "<<nb_it<<" it�rations avec "<<nb_failed<<" �checs"<<endl;
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
    m_graph->setTransition(m_transitionName);
    m_graph->setPlace(m_placeName);
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





//}


threadSOG::~threadSOG()
{
    //dtor
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
    m_graph->setTransition(m_transitionName);
    m_graph->setPlace(m_placeName);
    m_nbmetastate=0;
    m_MaxIntBdd=0;


    LDDState *c=new LDDState;

    //cout<<"Marquage initial is being built..."<<endl;
    LACE_ME;
    MDD initial_meta_state(CALL(Accessible_epsilon_lace,M0,&m_nonObservable,&m_tb_relation));


    fire=firable_obs_lace(initial_meta_state,&m_observable,&m_tb_relation);

    c->m_lddstate=initial_meta_state;

    m_nbmetastate++;
    m_old_size=lddmc_nodecount(c->m_lddstate);

    //max_meta_state_size=bdd_pathcount(Complete_meta_state);
    m_st[0].push(Pair(couple(c,initial_meta_state),fire));
    m_graph->setInitialState(c);
    m_graph->insert(c);
    //m_graph->nbMarking+=bdd_pathcount(c->m_lddstate);
    LDDState* reached_class;


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
            //cout<<"Transition order1: "<<*iter<<endl;
            //e.second.erase(t);
            SPAWN(Accessible_epsilon_lace,get_successor(e.first.second,t),&m_nonObservable,&m_tb_relation);
            onb_it++;iter++;
        }

        for (unsigned int i=0; i<onb_it; i++)
        {
            Set::iterator it = e.second.end();
            it--;
            int t=*it;
            e.second.erase(it);
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
                fire=firable_obs_lace(Complete_meta_state,&m_observable,&m_tb_relation);
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
    m_graph->setTransition(m_transitionName);
    m_graph->setPlace(m_placeName);

    m_nbmetastate=0;
    m_MaxIntBdd=0;


    LDDState *c=new LDDState;

    //cout<<"Marquage initial is being built..."<<endl;
    LACE_ME;
    MDD initial_meta_state(CALL(Accessible_epsilon_lace,M0,&m_nonObservable,&m_tb_relation));

    //lddmc_refs_spawn(SPAWN(lddmc_canonize,initial_meta_state,0,*this));


    fire=firable_obs_lace(initial_meta_state,&m_observable,&m_tb_relation);
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
            SPAWN(Accessible_epsilon_lace,get_successor(e.first.second,t),&m_nonObservable,&m_tb_relation);
            onb_it++;iter++;
        }

        for (unsigned int i=0; i<onb_it; i++)
        {
            Set::iterator it = e.second.end();
            it--;
            int t=*it;
            e.second.erase(it);

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
                fire=firable_obs_lace(Complete_meta_state,&m_observable,&m_tb_relation);
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



