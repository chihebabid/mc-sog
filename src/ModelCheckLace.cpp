#include "ModelCheckLace.h"

#include "sylvan.h"
#include "sylvan_seq.h"
#include <sylvan_sog.h>
#include <sylvan_int.h>

using namespace sylvan;
ModelCheckLace::ModelCheckLace(const NewNet &R,int nbThread):ModelCheckBaseMT(R,nbThread)
{
}
void
print_h(double size)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int i = 0;
    for (;size>1024;size/=1024) i++;
    printf("%.*f %s", i, size, units[i]);
}
size_t
getMaxMemoryTh()
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}
void ModelCheckLace::preConfigure() {   
    lace_init(m_nb_thread, 10000000);
    lace_startup(0, NULL, NULL);
    size_t max = 16LL<<30;
    if (max > getMaxMemoryTh()) max = getMaxMemoryTh()/10*9;
    printf("Memory : ");
    print_h(max);
    printf(" max.\n");
    LACE_ME;
    sylvan_set_limits(max, 16, 1);
    sylvan_init_package();
    sylvan_init_ldd();
    
    //init_gc_seq();
    sylvan_gc_enable();
    printf("%zu of %zu buckets filled!\n", llmsset_count_marked(nodes), llmsset_get_size(nodes));
    int  i;
    vector<Place>::const_iterator it_places;

    m_transitions=m_net->transitions;
    m_observable=m_net->Observable;
    m_place_proposition=m_net->m_formula_place;
    m_nonObservable=m_net->NonObservable;

    m_transitionName=m_net->transitionName;
    m_placeName=m_net->m_placePosName;

   
    InterfaceTrans=m_net->InterfaceTrans;
    
    cout<<"Nombre de places : "<<m_nbPlaces<<endl;
    cout<<"Derniere place : "<<m_net->places[m_nbPlaces-1].name<<endl;

    uint32_t * liste_marques=new uint32_t[m_net->places.size()];
    for(i=0,it_places=m_net->places.begin(); it_places!=m_net->places.end(); i++,it_places++)
    {
        liste_marques[i] =it_places->marking;
    }

    m_initialMarking=lddmc_cube(liste_marques,m_net->places.size());
    lddmc_refs_push(m_initialMarking);
    delete []liste_marques;

    uint32_t *prec = new uint32_t[m_nbPlaces];
    uint32_t *postc= new uint32_t [m_nbPlaces];
    // Transition relation
    for(auto t=m_net->transitions.begin();
            t!=m_net->transitions.end(); t++)
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
        lddmc_refs_push(_minus);
        MDD _plus=lddmc_cube(postc,m_nbPlaces);
        lddmc_refs_push(_plus);
        m_tb_relation.push_back(TransSylvan(_minus,_plus));
    }
    delete [] prec;
    delete [] postc;
}




TASK_3 (MDD, Aggregate_epsilon_lace, MDD, From, Set*, nonObservable, vector<TransSylvan>*, tb_relation)
{
    MDD M1;
    MDD M2=From;
      
    do
    {
        M1=M2;
        for(Set::iterator i=nonObservable->begin(); !(i==nonObservable->end()); i++)
        {
            lddmc_refs_spawn(SPAWN(lddmc_firing_lace,M2,(*tb_relation)[(*i)].getMinus(),(*tb_relation)[(*i)].getPlus()));
        }
      
        for(Set::iterator i=nonObservable->begin(); !(i==nonObservable->end()); i++)
        {
            lddmc_refs_push(M1);
            lddmc_refs_push(M2);
            MDD succ=lddmc_refs_sync(SYNC(lddmc_firing_lace));
            lddmc_refs_push(succ);
            M2=lddmc_union(succ,M2);
            lddmc_refs_pop(3);
        }
    }
    while (M1!=M2);
   /* lddmc_refs_popptr(2);*/
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

/// Compute divergence 

TASK_3 (bool, SetDivL, MDD, M, Set*, nonObservable, vector<TransSylvan>*, tb_relation) 
{
    
    if (nonObservable->empty()) return false;
	Set::iterator i;
	MDD Reached,From;
	
	Reached=lddmc_false;
    From=M;
	do
	{
        
		for(i=nonObservable->begin();!(i==nonObservable->end());i++)
            SPAWN(lddmc_firing_lace,From,(*tb_relation)[(*i)].getMinus(),(*tb_relation)[(*i)].getPlus());
        
        
		for(i=nonObservable->begin();!(i==nonObservable->end());i++) 
        {
            MDD To=SYNC(lddmc_firing_lace);
            Reached=lddmc_union(Reached,To);
        }
		if(Reached==From) return true;
        From=Reached;
        Reached=lddmc_false;

	} while(Reached!=lddmc_false && Reached != lddmc_true);
     
	 return false;
}

#define SetDivL(M, nonObservable,tb) CALL(SetDivL, M, nonObservable,tb)


LDDState * ModelCheckLace::getInitialMetaState()
{
    if (!m_built_initial) {
    LDDState *initalAggregate=new LDDState();
    LDDState *reached_class;
    LACE_ME;
    MDD initial_meta_state(CALL(Aggregate_epsilon_lace,m_initialMarking,&m_nonObservable,&m_tb_relation));
   
    lddmc_refs_push(initial_meta_state);
    Set fire=fire_obs_lace(initial_meta_state,&m_observable,&m_tb_relation);

    // c->m_lddstate=CALL(lddmc_canonize,initial_meta_state,0,*this);
    initalAggregate->m_lddstate=initial_meta_state;
    initalAggregate->setVisited();
    m_graph->setInitialState(initalAggregate);
    m_graph->insert(initalAggregate);
    
    initalAggregate->setDiv(SetDivL(initial_meta_state,&m_nonObservable,&m_tb_relation));
    initalAggregate->setDeadLock(Set_Bloc(initial_meta_state));
    // Compute successors
    unsigned int onb_it=0;
    Set::iterator iter=fire.begin();
    
    while(iter!=fire.end())
    {
        
        int t = *iter;
        SPAWN(Aggregate_epsilon_lace,get_successor(initial_meta_state,t),&m_nonObservable,&m_tb_relation);
        onb_it++;
        iter++;
    }
    MDD Complete_meta_state;
    for (unsigned int i=0; i<onb_it; i++)
    {
        Set::iterator it = fire.end();
        it--;
        int t=*it;
        fire.erase(it);
        Complete_meta_state=SYNC(Aggregate_epsilon_lace);
        reached_class=new LDDState();
        reached_class->m_lddstate=Complete_meta_state;
        lddmc_refs_push(Complete_meta_state);
        reached_class->setDiv(SetDivL(Complete_meta_state,&m_nonObservable,&m_tb_relation));
        m_graph->addArc();
        m_graph->insert(reached_class);
        initalAggregate->Successors.insert(initalAggregate->Successors.begin(),LDDEdge(reached_class,t));
        reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(initalAggregate,t));
    }
    m_built_initial=true;
    }

    return m_graph->getInitialState();
}

void ModelCheckLace::buildSucc(LDDState *agregate)
{
   //cout<<__func__<<endl;
   if (!agregate->isVisited()) {
   // It's first time to visit agregate, then we have to build its successors
        agregate->setVisited();         
        LDDState *reached_class=nullptr;
        LACE_ME;
        //displayMDDTableInfo();
        sylvan_gc();
        //displayMDDTableInfo();
        MDD meta_state=agregate->getLDDValue();//(CALL(Aggregate_epsilon_lace,agregate->getLDDValue(),&m_nonObservable,&m_tb_relation));

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
            reached_class=new LDDState();
            reached_class->m_lddstate=Complete_meta_state;
            LDDState* pos=m_graph->find(reached_class);

            if(!pos)
            {
                lddmc_refs_push(Complete_meta_state);
                reached_class->setDiv(SetDivL(Complete_meta_state,&m_nonObservable,&m_tb_relation));
                reached_class->setDeadLock(Set_Bloc(Complete_meta_state));
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





