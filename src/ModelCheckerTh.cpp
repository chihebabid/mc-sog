#include "ModelCheckerTh.h"

#include "sylvan.h"
#include "sylvan_seq.h"
#include <sylvan_sog.h>
#include <sylvan_int.h>

using namespace sylvan;

ModelCheckerTh::ModelCheckerTh(const NewNet &R, int BOUND,int nbThread):ModelCheckBaseMT(R,BOUND,nbThread)  {
}
void ModelCheckerTh::preConfigure()
{
   
    lace_init(1, 0);
    lace_startup(0, NULL, NULL);

    sylvan_set_limits(2LL<<30, 16, 3);
    sylvan_init_package();
    sylvan_init_ldd();

    int  i;
    vector<Place>::const_iterator it_places;


    init_gc_seq();


    //_______________
    transitions=m_net.transitions;
    m_observable=m_net.Observable;
    m_place_proposition=m_net.m_formula_place;
    m_nonObservable=m_net.NonObservable;

    m_transitionName=m_net.transitionName;
    m_placeName=m_net.m_placePosName;

   /* cout<<"Toutes les Transitions:"<<endl;
    map<string,int>::iterator it2=m_transitionName.begin();
    for (; it2!=m_transitionName.end(); it2++)
    {
        cout<<(*it2).first<<" : "<<(*it2).second<<endl;
    }*/


    InterfaceTrans=m_net.InterfaceTrans;
   
    cout<<"Nombre de places : "<<m_nbPlaces<<endl;
    cout<<"Derniere place : "<<m_net.places[m_nbPlaces-1].name<<endl;

    uint32_t * liste_marques=new uint32_t[m_net.places.size()];
    for(i=0,it_places=m_net.places.begin(); it_places!=m_net.places.end(); i++,it_places++)
    {
        liste_marques[i] =it_places->marking;
    }

    m_initialMarking=lddmc_cube(liste_marques,m_net.places.size());

    uint32_t *prec = new uint32_t[m_nbPlaces];
    uint32_t *postc= new uint32_t [m_nbPlaces];
    // Transition relation
    for(vector<Transition>::const_iterator t=m_net.transitions.begin();
            t!=m_net.transitions.end(); t++)
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
    delete [] prec;
    delete [] postc;
   
}





LDDState * ModelCheckerTh::buildInitialMetaState()
{
    ComputeTh_Succ();
    LDDState *c=new LDDState;
    MDD initial_meta_state=Accessible_epsilon(m_initialMarking);
    ldd_refs_push(initial_meta_state);
    c->m_lddstate=initial_meta_state;
    m_graph->setInitialState(c);
    m_graph->insert(c);
    Set fire=firable_obs(initial_meta_state);
    int j=0;
    for(auto it=fire.begin(); it!=fire.end(); it++,j++)
        m_st[j%m_nb_thread].push(couple_th(c,*it));
    pthread_barrier_wait(&m_barrier_threads);
    pthread_barrier_wait(&m_barrier_builder);
    c->setVisited();
    return c;
}

void ModelCheckerTh::buildSucc(LDDState *agregate)
{
    if (!agregate->isVisited())
    {
        agregate->setVisited();
        Set fire=firable_obs(agregate->getLDDValue());
        int j=0;
        for(auto it=fire.begin(); it!=fire.end(); it++,j++)
            m_st[j%m_nb_thread].push(couple_th(agregate,*it));
        pthread_barrier_wait(&m_barrier_threads);
        pthread_barrier_wait(&m_barrier_builder);
        //Wait for all threads to finish their work

    }
}

void * ModelCheckerTh::Compute_successors()
{
    int id_thread;
    int nb_it=0,nb_failed=0,max_succ=0;
    pthread_mutex_lock(&m_mutex);
    id_thread=m_id_thread++;
    pthread_mutex_unlock(&m_mutex);
    LDDState* reached_class=nullptr;

    do
    {
        pthread_barrier_wait(&m_barrier_threads);
        while (!m_st[id_thread].empty())
        {
            couple_th elt=m_st[id_thread].top();
            m_st[id_thread].pop();
            LDDState *aggregate=elt.first;
            MDD meta_state=aggregate->getLDDValue();
            int transition=elt.second;
            MDD complete_state=Accessible_epsilon(get_successor(meta_state,transition));
            reached_class=new LDDState;
            reached_class->m_lddstate=complete_state;
            pthread_mutex_lock(&m_graph_mutex);
            LDDState* pos=m_graph->find(reached_class);
            if(!pos)
            {
                m_graph->addArc();
                m_graph->insert(reached_class);
                pthread_mutex_unlock(&m_graph_mutex);
                aggregate->Successors.insert(aggregate->Successors.begin(),LDDEdge(reached_class,transition));
                reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(aggregate,transition));
            }
            else
            {
                m_graph->addArc();
                pthread_mutex_unlock(&m_graph_mutex);
                aggregate->Successors.insert(aggregate->Successors.begin(),LDDEdge(pos,transition));
                pos->Predecessors.insert(pos->Predecessors.begin(),LDDEdge(aggregate,transition));
                delete reached_class;
            }

        }
        pthread_barrier_wait(&m_barrier_builder);
    }
    while (!m_finish);
    cout<<"exit thread i"<<id_thread<<endl;
}



void * ModelCheckerTh::threadHandler(void *context)
{
    return ((ModelCheckerTh*)context)->Compute_successors();
}

void ModelCheckerTh::ComputeTh_Succ()
{


    int rc;
//   m_graph=&g;
//   m_graph->setTransition(m_transitionName);
//   m_graph->setPlace(m_placeName);
    m_id_thread=0;

    pthread_mutex_init(&m_mutex, NULL);
    //pthread_mutex_init(&m_gc_mutex,NULL);
    pthread_mutex_init(&m_graph_mutex,NULL);
    //pthread_mutex_init(&m_supervise_gc_mutex,NULL);
    pthread_barrier_init(&m_barrier_threads, NULL, m_nb_thread+1);
    pthread_barrier_init(&m_barrier_builder, NULL, m_nb_thread+1);

    for (int i=0; i<m_nb_thread; i++)
    {
        if ((rc = pthread_create(&m_list_thread[i], NULL,threadHandler,this)))
        {
            cout<<"error: pthread_create, rc: "<<rc<<endl;
        }
    }
    /* Compute_successors();
     for (int i = 0; i < m_nb_thread-1; i++)
     {
         pthread_join(m_list_thread[i], NULL);
     }*/


}
ModelCheckerTh::~ModelCheckerTh() {
    m_finish=true;    
    pthread_barrier_wait(&m_barrier_threads);    
    pthread_barrier_wait(&m_barrier_builder);    
    for (int i = 0; i < m_nb_thread; i++)
     {
         cout<<"thread "<<i<<endl;
         pthread_join(m_list_thread[i], NULL);
     }
     
}
