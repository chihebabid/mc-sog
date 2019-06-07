#include "ModelCheckLace.h"

#include "sylvan.h"
#include "sylvan_seq.h"
#include <sylvan_sog.h>
#include <sylvan_int.h>

using namespace sylvan;
ModelCheckLace::ModelCheckLace(const NewNet &R, int BOUND,int nbThread):ModelCheckBaseMT(R,BOUND,nbThread)
{
}
void ModelCheckLace::preConfigure() {
    cout<<__func__<<endl;
    lace_init(m_nb_thread, 10000000);
    lace_startup(0, NULL, NULL);
    sylvan_set_limits(2LL<<30, 16, 3);
    sylvan_init_package();
    sylvan_init_ldd();
    LACE_ME;
    
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
    for(auto t=m_net.transitions.begin();
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




TASK_3 (MDD, Aggregate_epsilon_lace, MDD, From, Set*, nonObservable, vector<TransSylvan>*, tb_relation)
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

TASK_3 (Set, fire_obs_lace,MDD, State, Set*, observable, vector<TransSylvan>*, tb_relation)
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

#define fire_obs_lace(state,obser,tb) CALL(fire_obs_lace, state, obser,tb)

LDDState * ModelCheckLace::buildInitialMetaState()
{

    LDDState *initalAggregate=new LDDState;
    LDDState *reached_class;
    LACE_ME;
    MDD initial_meta_state(CALL(Aggregate_epsilon_lace,m_initialMarking,&m_nonObservable,&m_tb_relation));
    Set fire=fire_obs_lace(initial_meta_state,&m_observable,&m_tb_relation);

    // c->m_lddstate=CALL(lddmc_canonize,initial_meta_state,0,*this);
    initalAggregate->m_lddstate=initial_meta_state;
    initalAggregate->setVisited();
    m_graph->setInitialState(initalAggregate);
    m_graph->insert(initalAggregate);
    // Compute successors
    unsigned int onb_it=0;
    Set::const_iterator iter=fire.begin();
    while(iter!=fire.end())
    {
        int t = *iter;

        SPAWN(Aggregate_epsilon_lace,get_successor(initial_meta_state,t),&m_nonObservable,&m_tb_relation);
        onb_it++;
        iter++;
    }

    for (unsigned int i=0; i<onb_it; i++)
    {
        Set::iterator it = fire.end();
        it--;
        int t=*it;
        fire.erase(it);
        MDD Complete_meta_state=SYNC(Aggregate_epsilon_lace);
        reached_class=new LDDState;
        reached_class->m_lddstate=Complete_meta_state;
        m_graph->addArc();
        m_graph->insert(reached_class);
        initalAggregate->Successors.insert(initalAggregate->Successors.begin(),LDDEdge(reached_class,t));
        reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(initalAggregate,t));
    }

    return initalAggregate;
}

void ModelCheckLace::buildSucc(LDDState *agregate)
{

   if (!agregate->isVisited()) {
   // It's first time to visit agregate, then we have to build its successors
        agregate->setVisited();
        LDDState *reached_class=nullptr;
        LACE_ME;
        MDD meta_state(CALL(Aggregate_epsilon_lace,agregate->getLDDValue(),&m_nonObservable,&m_tb_relation));

        Set fire=fire_obs_lace(meta_state,&m_observable,&m_tb_relation);
        unsigned int onb_it=0;
        Set::const_iterator iter=fire.begin();
        while(iter!=fire.end())
        {
            int t = *iter;
            SPAWN(Aggregate_epsilon_lace,get_successor(meta_state,t),&m_nonObservable,&m_tb_relation);
            onb_it++;iter++;
        }

        for (unsigned int i=0; i<onb_it; i++)
        {
            Set::iterator it = fire.end();
            it--;
            int t=*it;
            fire.erase(it);
            MDD Complete_meta_state=SYNC(Aggregate_epsilon_lace);
            reached_class=new LDDState;
            reached_class->m_lddstate=Complete_meta_state;
            LDDState* pos=m_graph->find(reached_class);

            if(!pos)
            {
                m_graph->addArc();
                m_graph->insert(reached_class);
                agregate->Successors.insert(agregate->Successors.begin(),LDDEdge(reached_class,t));
                reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(agregate,t));

            }
            else
            {
                m_graph->addArc();
                agregate->Successors.insert(agregate->Successors.begin(),LDDEdge(pos,t));
                pos->Predecessors.insert(pos->Predecessors.begin(),LDDEdge(agregate,t));
                delete reached_class;
            }
        }


   }

}


