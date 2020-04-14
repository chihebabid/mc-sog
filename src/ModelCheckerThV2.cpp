#include "ModelCheckerThV2.h"
#include "sylvan.h"
#include "sylvan_seq.h"
#include <sylvan_sog.h>
#include <sylvan_int.h>
#include <functional>

using namespace sylvan;
using namespace std;

ModelCheckerThV2::ModelCheckerThV2 ( const NewNet &R, int nbThread ) :
    ModelCheckBaseMT ( R, nbThread )
{
}
size_t
getMaxMemoryV3()
{
    long pages = sysconf ( _SC_PHYS_PAGES );
    long page_size = sysconf ( _SC_PAGE_SIZE );
    return pages * page_size;
}
void ModelCheckerThV2::preConfigure()
{

    lace_init ( 1, 0 );
    lace_startup ( 0, NULL, NULL );
    size_t max = 16LL<<30;
    if ( max > getMaxMemoryV3() ) {
        max = getMaxMemoryV3() /10*9;
    }
    sylvan_set_limits ( max, 8, 0 );

    sylvan_init_package();
    sylvan_init_ldd();
    displayMDDTableInfo();

    vector<Place>::const_iterator it_places;

    init_gc_seq();


    transitions = m_net->transitions;
    m_observable = m_net->Observable;
    m_place_proposition = m_net->m_formula_place;
    m_nonObservable = m_net->NonObservable;

    m_transitionName = m_net->transitionName;
    m_placeName = m_net->m_placePosName;

    InterfaceTrans = m_net->InterfaceTrans;

    cout << "Nombre de places : " << m_nbPlaces << endl;
    cout << "Derniere place : " << m_net->places[m_nbPlaces - 1].name << endl;

    uint32_t *liste_marques = new uint32_t[m_net->places.size()];
    uint32_t i;
    for ( i = 0, it_places = m_net->places.begin(); it_places != m_net->places.end(); i++, it_places++ ) {
        liste_marques[i] = it_places->marking;
    }

    m_initialMarking = lddmc_cube ( liste_marques, m_net->places.size() );
    ldd_refs_push ( m_initialMarking );
    uint32_t *prec = new uint32_t[m_nbPlaces];
    uint32_t *postc = new uint32_t[m_nbPlaces];
    // Transition relation
    for ( vector<Transition>::const_iterator t = m_net->transitions.begin(); t != m_net->transitions.end(); t++ ) {
        // Initialisation
        for ( i = 0; i < m_nbPlaces; i++ ) {
            prec[i] = 0;
            postc[i] = 0;
        }
        // Calculer les places adjacentes a la transition t
        set<int> adjacentPlace;
        for ( vector<pair<int, int> >::const_iterator it = t->pre.begin(); it != t->pre.end(); it++ ) {
            adjacentPlace.insert ( it->first );
            prec[it->first] = prec[it->first] + it->second;
            //printf("It first %d \n",it->first);
            //printf("In prec %d \n",prec[it->first]);
        }
        // arcs post
        for ( vector<pair<int, int> >::const_iterator it = t->post.begin(); it != t->post.end(); it++ ) {
            adjacentPlace.insert ( it->first );
            postc[it->first] = postc[it->first] + it->second;

        }
        for ( set<int>::const_iterator it = adjacentPlace.begin(); it != adjacentPlace.end(); it++ ) {
            MDD Precond = lddmc_true;
            Precond = Precond & ( ( *it ) >= prec[*it] );
        }

        MDD _minus = lddmc_cube ( prec, m_nbPlaces );
        ldd_refs_push ( _minus );
        MDD _plus = lddmc_cube ( postc, m_nbPlaces );
        ldd_refs_push ( _plus );
        m_tb_relation.push_back ( TransSylvan ( _minus, _plus ) );
    }
    delete[] prec;
    delete[] postc;
    ComputeTh_Succ();

}

LDDState* ModelCheckerThV2::getInitialMetaState()
{
    while ( !m_finish_initial )	;
    LDDState *initial_metastate = m_graph->getInitialState();
    if ( !initial_metastate->isVisited() ) {
        initial_metastate->setVisited();
        while ( !initial_metastate->isCompletedSucc() )
            ;
    }
    return initial_metastate;
}

void ModelCheckerThV2::buildSucc ( LDDState *agregate )
{
    if ( !agregate->isVisited() ) {
        agregate->setVisited();
        while ( !agregate->isCompletedSucc() )
            ;
    }
}

void ModelCheckerThV2::Compute_successors()
{
    int id_thread;
    {
        std::scoped_lock  guard ( m_mutex );
        id_thread = m_id_thread++;
    }
    //cout<<" I'm here thread id " <<m_id_thread<<endl;
    Set fire;
    int min_charge = 0;
    m_started=false;
    if ( id_thread == 0 ) {
        LDDState *c = new LDDState;
        MDD Complete_meta_state=Accessible_epsilon ( m_initialMarking );
        ldd_refs_push ( Complete_meta_state );
        fire = firable_obs ( Complete_meta_state );
        c->m_lddstate = Complete_meta_state;
        c->setDeadLock ( Set_Bloc ( Complete_meta_state ) );
        c->setDiv ( Set_Div ( Complete_meta_state ) );
        m_common_stack.push ( Pair ( couple ( c, Complete_meta_state ), fire ) );
        m_condStack.notify_one();
        m_started=true;
        m_graph->setInitialState ( c );
        m_graph->insert ( c );
        m_finish_initial = true;
    }

    LDDState *reached_class;
    //pthread_barrier_wait ( &m_barrier_builder );
    Pair e;
    do {
        
        
        std::unique_lock<std::mutex> lk ( m_mutexStack );
        m_condStack.wait(lk,std::bind(&ModelCheckerThV2::hasToProcess,this));
        lk.unlock();
        
        if (!m_finish ) {
            m_terminaison++;
            bool advance=true;
            try {
                m_common_stack.pop ( e );                
            } catch ( ... ) {
                advance=false;
            }

            if ( advance ) {
                while ( !e.second.empty() && !m_finish ) {
                    int t = *e.second.begin();
                    e.second.erase ( t );
                    if ( id_thread ) {
                        std::scoped_lock supervise ( m_supervise_gc_mutex );
                        m_gc++;
                        if ( m_gc == 1 ) {
                            m_gc_mutex.lock();
                        }
                    }

                    //MDD reduced_meta = Complete_meta_state;            //Canonize(Complete_meta_state,0);
                    /*ldd_refs_push(reduced_meta);*/
                    if ( id_thread == 0 ) {
                        std::scoped_lock guard ( m_gc_mutex );
                        //     #ifdef DEBUG_GC
                        //displayMDDTableInfo();
                        //   #endif // DEBUG_GC
                        sylvan_gc_seq();
                        //   #ifdef DEBUG_GC
                        //displayMDDTableInfo();
                        //   #endif // DEBUG_GC
                    }

                    //pthread_spin_lock(&m_accessible);

                    MDD reduced_meta = Accessible_epsilon ( get_successor ( e.first.second, t ) );
                    ldd_refs_push ( reduced_meta );
                    reached_class = new LDDState();
                    reached_class->m_lddstate = reduced_meta;
                    m_graph_mutex.lock();
                    LDDState *pos = m_graph->find ( reached_class );
                    if ( !pos ) {
                        //  cout<<"not found"<<endl;

                        m_graph->addArc();
                        m_graph->insert ( reached_class );
                        m_graph_mutex.unlock();
                        //reached_class->setDiv(Set_Div(reduced_meta));
                        //reached_class->setDeadLock(Set_Bloc(reduced_meta));
                        fire = firable_obs ( reduced_meta );
                        reached_class->setDeadLock ( Set_Bloc ( reduced_meta ) );
                        reached_class->setDiv ( Set_Div ( reduced_meta ) );

                        m_common_stack.push ( Pair ( couple ( reached_class, reduced_meta ), fire ) );
                        m_condStack.notify_one();

                        e.first.first->Successors.insert ( e.first.first->Successors.begin(), LDDEdge ( reached_class, t ) );
                        reached_class->Predecessors.insert ( reached_class->Predecessors.begin(), LDDEdge ( e.first.first, t ) );

                    } else {
                        m_graph->addArc();
                        m_graph_mutex.unlock();
                        e.first.first->Successors.insert ( e.first.first->Successors.begin(), LDDEdge ( pos, t ) );
                        pos->Predecessors.insert ( pos->Predecessors.begin(), LDDEdge ( e.first.first, t ) );
                        delete reached_class;
                    }
                    if ( id_thread ) {
                        std::scoped_lock guard ( m_supervise_gc_mutex );
                        m_gc--;
                        if ( m_gc == 0 ) {
                            m_gc_mutex.unlock();
                        }

                    }
                }
                e.first.first->setCompletedSucc();
            } //end advance
            m_terminaison--;
        }

    } while ( !m_finish );
    pthread_barrier_wait ( &m_barrier_builder );

}

void ModelCheckerThV2::threadHandler ( void *context )
{
    ( ( ModelCheckerThV2* ) context )->Compute_successors();
}

void ModelCheckerThV2::ComputeTh_Succ()
{

    m_id_thread = 0;



    /*   pthread_barrier_init(&m_barrier_threads, NULL, m_nb_thread+1);*/
    pthread_barrier_init ( &m_barrier_builder, NULL, m_nb_thread + 1 );


    m_gc=0;
    m_terminaison=0;
    m_finish=false;
    for ( int i = 0; i < m_nb_thread; i++ ) {
        int rc;
        m_list_thread[i]= new thread ( threadHandler,this );
        if ( m_list_thread==nullptr ) {
            cout << "error: pthread_create, rc: " << rc << endl;
        }
    }
}

ModelCheckerThV2::~ModelCheckerThV2()
{
    m_finish = true;
    m_condStack.notify_all();
    pthread_barrier_wait ( &m_barrier_builder );
    for ( int i = 0; i < m_nb_thread; i++ ) {
        m_list_thread[i]->join();
        delete m_list_thread[i];
    }

}
bool ModelCheckerThV2::hasToProcess() const {
    return m_finish || !m_common_stack.empty();
}

