#include "ModelCheckerCPPThread.h"
//#include "sylvan.h"
//#include <sylvan_int.h>
#include <functional>
#include <iostream>
#include <unistd.h>

#include "SylvanWrapper.h"
#define GETNODE(mdd) ((mddnode_t)llmsset_index_to_ptr(nodes, mdd))

using namespace std;

ModelCheckerCPPThread::ModelCheckerCPPThread ( const NewNet &R, int nbThread ) :
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
void ModelCheckerCPPThread::preConfigure()
{

    /*lace_init ( 1, 0 );
    lace_startup ( 0, NULL, NULL );
    size_t max = 16LL<<34;
    if ( max > getMaxMemoryV3() ) {
        max = getMaxMemoryV3() /10*9;
    }*/
    SylvanWrapper::sylvan_set_limits ( 16LL<<30, 8, 0 );

    //sylvan_init_package();
    SylvanWrapper::sylvan_init_package();
    SylvanWrapper::sylvan_init_ldd();
    SylvanWrapper::init_gc_seq();
    SylvanWrapper::displayMDDTableInfo();

    vector<Place>::const_iterator it_places;
    m_transitions = m_net->transitions;
    m_observable = m_net->Observable;
    m_place_proposition = m_net->m_formula_place;
    m_nonObservable = m_net->NonObservable;

    m_transitionName = &m_net->transitionName;
    m_placeName = &m_net->m_placePosName;

    InterfaceTrans = m_net->InterfaceTrans;

    cout << "Nombre de places : " << m_nbPlaces << endl;
    cout << "Derniere place : " << m_net->places[m_nbPlaces - 1].name << endl;

    uint32_t *liste_marques = new uint32_t[m_net->places.size()];
    uint32_t i;
    for ( i = 0, it_places = m_net->places.begin(); it_places != m_net->places.end(); i++, it_places++ ) {
        liste_marques[i] = it_places->marking;
    }

    m_initialMarking = SylvanWrapper::lddmc_cube ( liste_marques, m_net->places.size() );

    SylvanWrapper::lddmc_refs_push ( m_initialMarking );
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

        MDD _minus = SylvanWrapper::lddmc_cube ( prec, m_nbPlaces );
        SylvanWrapper::lddmc_refs_push ( _minus );
        MDD _plus = SylvanWrapper::lddmc_cube ( postc, m_nbPlaces );
        SylvanWrapper::lddmc_refs_push ( _plus );
        m_tb_relation.push_back ( TransSylvan ( _minus, _plus ) );
    }
    delete[] prec;
    delete[] postc;
    ComputeTh_Succ();


}




void ModelCheckerCPPThread::Compute_successors()
{
    int id_thread;
   
   id_thread = m_id_thread++;
   
    //cout<<" I'm here thread id " <<m_id_thread<<endl;
    Set fire;    
    
    if ( id_thread == 0 ) {
        LDDState *c = new LDDState;
        MDD Complete_meta_state=Accessible_epsilon ( m_initialMarking );
        /*SylvanWrapper::getMarksCount(Complete_meta_state);
        exit(0);*/

        SylvanWrapper::lddmc_refs_push ( Complete_meta_state );

        fire = firable_obs ( Complete_meta_state );
        c->m_lddstate = Complete_meta_state;
        c->setDeadLock ( Set_Bloc ( Complete_meta_state ) );
        c->setDiv ( Set_Div ( Complete_meta_state ) );
        m_graph->setInitialState ( c );
        m_graph->insert ( c );
        m_common_stack.push ( Pair ( couple ( c, Complete_meta_state ), fire ) );
        m_condStack.notify_one();
        m_finish_initial = true;
    }
    LDDState *reached_class;
    //pthread_barrier_wait ( &m_barrier_builder );
    Pair e;
    do {        
        std::unique_lock<std::mutex> lk ( m_mutexStack );
        m_condStack.wait(lk,std::bind(&ModelCheckerCPPThread::hasToProcess,this));
        lk.unlock();
                            
            if ( m_common_stack.try_pop ( e ) && !m_finish ) {
                while ( !e.second.empty() && !m_finish ) {
                    int t = *e.second.begin();
                    e.second.erase ( t );
                    MDD reduced_meta = Accessible_epsilon ( get_successor ( e.first.second, t ) );
                    SylvanWrapper::lddmc_refs_push ( reduced_meta );
                    reached_class = new LDDState();
                    reached_class->m_lddstate = reduced_meta;                    
                    LDDState *pos = m_graph->find ( reached_class );
                    if ( !pos ) {
                        m_graph->addArc();
                        m_graph->insert ( reached_class );
                        fire = firable_obs ( reduced_meta );
                        reached_class->setDeadLock ( Set_Bloc ( reduced_meta ) );
                        reached_class->setDiv ( Set_Div ( reduced_meta ) );
                        m_common_stack.push ( Pair ( couple ( reached_class, reduced_meta ), fire ) );
                        m_condStack.notify_one();
                        m_graph_mutex.lock();
                        e.first.first->Successors.insert ( e.first.first->Successors.begin(), LDDEdge ( reached_class, t ) );
                        reached_class->Predecessors.insert ( reached_class->Predecessors.begin(), LDDEdge ( e.first.first, t ) );
                        m_graph_mutex.unlock();
                    } else {
                        delete reached_class;
                        m_graph->addArc();
                        m_graph_mutex.lock();
                        e.first.first->Successors.insert ( e.first.first->Successors.begin(), LDDEdge ( pos, t ) );
                        pos->Predecessors.insert ( pos->Predecessors.begin(), LDDEdge ( e.first.first, t ) );
                        m_graph_mutex.unlock();                        
                    }
                    
                }
                e.first.first->setCompletedSucc();m_condBuild.notify_one();
            } //end advance

    } while ( !m_finish );


}

void ModelCheckerCPPThread::threadHandler ( void *context )
{
    SylvanWrapper::lddmc_refs_init();
    ( ( ModelCheckerCPPThread* ) context )->Compute_successors();
}

void ModelCheckerCPPThread::ComputeTh_Succ()
{

    m_id_thread = 0;

    pthread_barrier_init ( &m_barrier_builder, NULL, m_nb_thread + 1 );
   
    m_finish=false;
    for ( int i = 0; i < m_nb_thread; i++ ) {
        int rc;
        m_list_thread[i]= new thread ( threadHandler,this );
        if ( m_list_thread==nullptr ) {
            cout << "error: pthread_create, rc: " << rc << endl;
        }
    }
}

ModelCheckerCPPThread::~ModelCheckerCPPThread()
{
    m_finish = true;
    m_condStack.notify_all();
    //pthread_barrier_wait ( &m_barrier_builder );
    for ( int i = 0; i < m_nb_thread; i++ ) {
        m_list_thread[i]->join();
        delete m_list_thread[i];
    }

}
bool ModelCheckerCPPThread::hasToProcess() const {
    return m_finish || !m_common_stack.empty();
}
