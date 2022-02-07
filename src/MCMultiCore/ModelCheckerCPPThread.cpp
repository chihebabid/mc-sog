#include "ModelCheckerCPPThread.h"
//#include "sylvan.h"
//#include <sylvan_int.h>
#include <functional>
#include <iostream>
#include <unistd.h>

#include "SylvanWrapper.h"

#define GETNODE(mdd) ((mddnode_t)llmsset_index_to_ptr(nodes, mdd))

using namespace std;

ModelCheckerCPPThread::ModelCheckerCPPThread(NewNet &R, int nbThread) :
        ModelCheckBaseMT(R, nbThread) {
}

size_t
getMaxMemoryV3() {
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}

void ModelCheckerCPPThread::preConfigure() {
    initializeLDD();
    loadNetFromFile();
    ComputeTh_Succ();
}

/*
 * Compute SOG without reduction
 */
void ModelCheckerCPPThread::Compute_successors() {
    int id_thread;

    id_thread = m_id_thread++;

    //cout<<" I'm here thread id " <<m_id_thread<<endl;
    Set fire;

    if (id_thread == 0) {
        auto *c = new LDDState;
        MDD Complete_meta_state = Accessible_epsilon(m_initialMarking);
        SylvanWrapper::lddmc_refs_push(Complete_meta_state);

        fire = firable_obs(Complete_meta_state);
        c->m_lddstate = Complete_meta_state;
        c->setDeadLock(Set_Bloc(Complete_meta_state));
        c->setDiv(Set_Div(Complete_meta_state));
        m_graph->setInitialState(c);
        m_graph->insert(c);
        m_common_stack.push(Pair(couple(c, Complete_meta_state), fire));
        m_condStack.notify_one();
        m_finish_initial = true;
    }


    Pair e;
    do {
        std::unique_lock<std::mutex> lk(m_mutexStack);
        m_condStack.wait(lk, std::bind(&ModelCheckerCPPThread::hasToProcess, this));
        lk.unlock();

        if (m_common_stack.try_pop(e) && !m_finish) {
            while (!e.second.empty() && !m_finish) {
                int t = *e.second.begin();
                e.second.erase(t);
                MDD reduced_meta = Accessible_epsilon(get_successor(e.first.second, t));
                SylvanWrapper::lddmc_refs_push(reduced_meta);
                bool res;
                LDDState *pos = m_graph->insertFindByMDD(reduced_meta, res);
                m_graph->addArc();
                if (!res) {
                    fire = firable_obs(reduced_meta);
                    pos->setDeadLock(Set_Bloc(reduced_meta));
                    pos->setDiv(Set_Div(reduced_meta));
                    m_common_stack.push(Pair(couple(pos, reduced_meta), fire));
                    m_condStack.notify_one();
                }
                m_graph_mutex.lock();
                e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(pos, t));
                pos->Predecessors.insert(pos->Predecessors.begin(), LDDEdge(e.first.first, t));
                m_graph_mutex.unlock();
            }
            e.first.first->setCompletedSucc();
            m_condBuild.notify_one();
        } //end advance

    } while (!m_finish);
}

void ModelCheckerCPPThread::threadHandler(void *context) {
    SylvanWrapper::lddmc_refs_init();
    ((ModelCheckerCPPThread *) context)->Compute_successors();
}

void ModelCheckerCPPThread::ComputeTh_Succ() {
    m_id_thread = 0;
    pthread_barrier_init(&m_barrier_builder, nullptr, m_nb_thread + 1);
    m_finish = false;
    for (int i = 0; i < m_nb_thread; i++) {
        m_list_thread[i] = new thread(threadHandler, this);
        if (m_list_thread[i] == nullptr) {
            cout << "error: pthread creation failed. "  << endl;
        }
    }
}

ModelCheckerCPPThread::~ModelCheckerCPPThread() {
    m_finish = true;
    m_condStack.notify_all();
    for (int i = 0; i < m_nb_thread; i++) {
        m_list_thread[i]->join();
        delete m_list_thread[i];
    }
}

bool ModelCheckerCPPThread::hasToProcess() const {
    return m_finish || !m_common_stack.empty();
}
