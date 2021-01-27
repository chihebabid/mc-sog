#include "ModelCheckThReq.h"
#include <unistd.h>
#include <thread>

#define LEVEL 3

ModelCheckThReq::ModelCheckThReq(const NewNet &R, int nbThread) : ModelCheckBaseMT(R, nbThread) {
}

void
print_h(double size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int i = 0;
    for (; size > 1024; size /= 1024) i++;
    printf("%.*f %s", i, size, units[i]);
}

size_t
getMaxMemoryTh() {
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}

void ModelCheckThReq::preConfigure() {
    //cout<<"Enter : "<<__func__ <<endl;
    SylvanWrapper::sylvan_set_limits(16LL << 30, 10, 0);
    SylvanWrapper::sylvan_init_package();
    SylvanWrapper::sylvan_init_ldd();
    SylvanWrapper::init_gc_seq();
    SylvanWrapper::lddmc_refs_init();
    SylvanWrapper::displayMDDTableInfo();
    int i;
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
    for (i = 0, it_places = m_net->places.begin(); it_places != m_net->places.end(); i++, it_places++) {
        liste_marques[i] = it_places->marking;
    }

    m_initialMarking = SylvanWrapper::lddmc_cube(liste_marques, m_net->places.size());
    SylvanWrapper::lddmc_refs_push(m_initialMarking);
    delete[]liste_marques;

    uint32_t *prec = new uint32_t[m_nbPlaces];
    uint32_t *postc = new uint32_t[m_nbPlaces];
    // Transition relation
    for (auto t = m_net->transitions.begin();
         t != m_net->transitions.end(); t++) {
        // Initialisation
        for (i = 0; i < m_nbPlaces; i++) {
            prec[i] = 0;
            postc[i] = 0;
        }
        // Calculer les places adjacentes a la transition t
        set<int> adjacentPlace;
        for (vector<pair<int, int> >::const_iterator it = t->pre.begin(); it != t->pre.end(); it++) {
            adjacentPlace.insert(it->first);
            prec[it->first] = prec[it->first] + it->second;
            //printf("It first %d \n",it->first);
            //printf("In prec %d \n",prec[it->first]);
        }
        // arcs post
        for (vector<pair<int, int> >::const_iterator it = t->post.begin(); it != t->post.end(); it++) {
            adjacentPlace.insert(it->first);
            postc[it->first] = postc[it->first] + it->second;

        }
        for (set<int>::const_iterator it = adjacentPlace.begin(); it != adjacentPlace.end(); it++) {
            MDD Precond = lddmc_true;
            Precond = Precond & ((*it) >= prec[*it]);
        }

        MDD _minus = SylvanWrapper::lddmc_cube(prec, m_nbPlaces);
        SylvanWrapper::lddmc_refs_push(_minus);
        MDD _plus = SylvanWrapper::lddmc_cube(postc, m_nbPlaces);
        SylvanWrapper::lddmc_refs_push(_plus);
        m_tb_relation.push_back(TransSylvan(_minus, _plus));
    }
    delete[] prec;
    delete[] postc;
}

/**
 * Creates builder threads
 */
void ModelCheckThReq::ComputeTh_Succ() {
    int rc;
    m_id_thread = 0;
    //cout<<"Enter : "<<__func__ <<endl;
    pthread_barrier_init(&m_barrier_threads, NULL, m_nb_thread + 1);
    for ( int i = 0; i < m_nb_thread; i++ ) {
        m_list_thread[i]= new thread ( threadHandler,this );
        if ( m_list_thread[i]==nullptr ) {
            cout << "error: pthread creation failed"  << endl;
        }
    }

}

/*
 * Build initial aggregate
 */
LDDState *ModelCheckThReq::getInitialMetaState() {

    if (m_graph->getInitialAggregate() != nullptr) {
        return m_graph->getInitialAggregate();
    }
    ComputeTh_Succ();
    LDDState *c = new LDDState;
    MDD initial_meta_state = Accessible_epsilon(m_initialMarking);
    SylvanWrapper::lddmc_refs_push(initial_meta_state);
    c->m_lddstate = initial_meta_state;
    m_graph->setInitialState(c);
    m_graph->insert(c);
    c->setVisited();
    Set fire = firable_obs(initial_meta_state);
    c->m_nbSuccessorsToBeProcessed = fire.size();
    c->setMCurrentLevel(LEVEL);

    c->setDiv(Set_Div(initial_meta_state));
    c->setDeadLock(Set_Bloc(initial_meta_state));
    if (!fire.empty()) {

        for (auto it = fire.begin(); it != fire.end(); it++)
            m_common_stack.push(couple_th(c, *it));
        std::unique_lock<std::mutex> lk(m_mutexBuildCompleted);
        m_datacondBuildCompleted.wait(lk, [c] { return c->isCompletedSucc(); });
        lk.unlock();
    } else c->setCompletedSucc();
    return c;
}

/*
 * Build successors of @agregate
 */
void ModelCheckThReq::buildSucc(LDDState *agregate) {
    if (!agregate->isVisited()) {
        agregate->setVisited();
        if (agregate->getMCurrentLevel() == 0) {
            Set fire = firable_obs(agregate->m_lddstate);
            agregate->setMCurrentLevel(LEVEL);
            agregate->m_nbSuccessorsToBeProcessed = fire.size();
            if (!fire.empty()) {
                agregate->m_nbSuccessorsToBeProcessed = fire.size();
                for (auto it = fire.begin(); it != fire.end(); it++)
                    m_common_stack.push(couple_th(agregate, *it));
                std::unique_lock<std::mutex> lk(m_mutexBuildCompleted);
                m_datacondBuildCompleted.wait(lk, [agregate] { return agregate->isCompletedSucc(); });
                lk.unlock();

            } else agregate->setCompletedSucc();
        } else {
            std::unique_lock<std::mutex> lk(m_mutexBuildCompleted);
            m_datacondBuildCompleted.wait(lk, [agregate] { return agregate->isCompletedSucc(); });
            lk.unlock();
        }
    }
}


void *ModelCheckThReq::threadHandler(void *context) {
    return ((ModelCheckThReq *) context)->Compute_successors();
}


void *ModelCheckThReq::Compute_successors() {

    couple_th elt;
    LDDState *pos, *aggregate;
    do {
        while (!m_finish && m_common_stack.try_pop(elt)) {
            aggregate = elt.first;
            int transition = elt.second;
            MDD meta_state = aggregate->m_lddstate;
            MDD complete_state = Accessible_epsilon(get_successor(meta_state, transition));
            bool res;
            pos = m_graph->insertFindByMDD(complete_state, res);
            {
                std::unique_lock guard(m_graph_mutex);
                aggregate->Successors.insert(aggregate->Successors.begin(), LDDEdge(pos, transition));
                pos->Predecessors.insert(pos->Predecessors.begin(),
                                         LDDEdge(aggregate, transition));
            }
            if (!res) {
                pos->setDiv(Set_Div(complete_state));
                pos->setDeadLock(Set_Bloc(complete_state));
            }
            aggregate->decNbSuccessors();
            if (aggregate->m_nbSuccessorsToBeProcessed == 0) {
                aggregate->setCompletedSucc();
                m_datacondBuildCompleted.notify_one();
            }

            if (aggregate->getMCurrentLevel() > 0 && pos->getMCurrentLevel() == 0) {
                pos->setMCurrentLevel(aggregate->getMCurrentLevel() - 1);
                if (aggregate->getMCurrentLevel() > 1) {
                    Set fire = firable_obs(pos->getLDDValue());
                    pos->m_nbSuccessorsToBeProcessed = fire.size();
                    if (!fire.empty()) {
                        for (auto it = fire.begin(); it != fire.end(); it++)
                            m_common_stack.push(couple_th(pos, *it));
                    } else {
                        pos->setCompletedSucc();
                        m_datacondBuildCompleted.notify_one();
                    }

                }

            }
            m_graph->addArc();

        }
    } while (!m_finish);


}


ModelCheckThReq::~ModelCheckThReq() {
    //pthread_barrier_wait(&m_barrier_threads);
    for (int i = 0; i < m_nb_thread; i++) {
        m_list_thread[i]->join();
        delete m_list_thread[i];
    }
}

