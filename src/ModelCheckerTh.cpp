#include "ModelCheckerTh.h"

#include "sylvan.h"
#include "sylvan_seq.h"
#include <sylvan_sog.h>
#include <sylvan_int.h>

using namespace sylvan;
ModelCheckerTh::ModelCheckerTh(const NewNet &R, int BOUND,int nbThread)
{
    m_nb_thread=nbThread;
    lace_init(1, 0);
    lace_startup(0, NULL, NULL);

    sylvan_set_limits(2LL<<30, 16, 3);
    sylvan_init_package();
    sylvan_init_ldd();
//    LACE_ME;
    m_net=R;


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
    map<string,int>::iterator it2=m_transitionName.begin();
    for (; it2!=m_transitionName.end(); it2++)
    {
        cout<<(*it2).first<<" : "<<(*it2).second<<endl;
    }



    cout<<"Transitions observables :"<<endl;
    Set::iterator it=m_observable.begin();
    for (; it!=m_observable.end(); it++)
    {
        cout<<*it<<"  ";
    }
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

    m_initalMarking=lddmc_cube(liste_marques,R.places.size());





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
    delete [] prec;
    delete [] postc;
    m_graph=new LDDGraph(this);
    m_graph->setTransition(m_transitionName);
    m_graph->setPlace(m_placeName);
}


string ModelCheckerTh::getTransition(int pos)
{
    return m_graph->getTransition(pos);
}

string ModelCheckerTh::getPlace(int pos)
{
    return m_graph->getPlace(pos);
}


LDDState * ModelCheckerTh::buildInitialMetaState()
{        Set fire;

        LDDState *c=new LDDState;
        LDDState *reached_class;
         MDD initial_meta_state(Accessible_epsilon(m_initalMarking));
        ldd_refs_push(initial_meta_state);
        fire=firable_obs(initial_meta_state);

        c->m_lddstate=initial_meta_state;
       // m_old_size=lddmc_nodecount(c->m_lddstate);
        m_graph->setInitialState(c);
        m_graph->insert(c);

    // Compute successors

   /* for (unsigned int i=0; i<onb_it; i++)
    {
        Set::iterator it = fire.end();
        it--;
        int t=*it;
        fire.erase(it);
        MDD Complete_meta_state=Accessible_epsilon(it);
        reached_class=new LDDState;
        reached_class->m_lddstate=Complete_meta_state;
        m_graph->addArc();
        m_graph->insert(reached_class);
        c->Successors.insert(c->Successors.begin(),LDDEdge(reached_class,t));
        reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(c,t));
    } */

    return c;
}

void ModelCheckerTh::buildSucc(LDDState *agregate)
{
   if (!agregate->isVisited()) {
   agregate->setVisited();
   LDDState *reached_class=nullptr;

}
}

/*void * ModelCheckerTh::Compute_successors()
{
    int id_thread;
    int nb_it=0,nb_failed=0,max_succ=0;
    pthread_mutex_lock(&m_mutex);
    id_thread=m_id_thread++;
    pthread_mutex_unlock(&m_mutex);
    Set fire;
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

                if (id_thread==0)
                {
                    pthread_mutex_lock(&m_gc_mutex);
                    sylvan_gc_seq();

                    pthread_mutex_unlock(&m_gc_mutex);
                }
                reached_class->m_lddstate=Complete_meta_state;

                pthread_mutex_lock(&m_graph_mutex);

                LDDState* pos=m_graph->find(reached_class);

                if(!pos)
                {

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


void * ModelCheckerTh::threadHandler(void *context)
{
    return ((ModelCheckerTh*)context)->Compute_successors();
} */
