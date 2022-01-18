#include "MCThReqPor.h"
#include <unistd.h>
#include <thread>

#define LEVEL 3

MCThReqPor::MCThReqPor(const NewNet &R, int nbThread) : ModelCheckBaseMT(R, nbThread) {
}


void MCThReqPor::preConfigure() {
    //cout<<"Enter : "<<__func__ <<endl;
    initializeLDD();
    loadNetFromFile();
}

/**
 * Creates builder threads
 */
void MCThReqPor::ComputeTh_Succ() {

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
LDDState *MCThReqPor::getInitialMetaState() {

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
void MCThReqPor::buildSucc(LDDState *agregate) {
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


void *MCThReqPor::threadHandler(void *context) {
    return ((MCThReqPor *) context)->Compute_successors();
}


void *MCThReqPor::Compute_successors() {
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


MCThReqPor::~MCThReqPor() {
    //pthread_barrier_wait(&m_barrier_threads);
    for (int i = 0; i < m_nb_thread; i++) {
        m_list_thread[i]->join();
        delete m_list_thread[i];
    }
}

