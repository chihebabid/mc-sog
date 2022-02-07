#include "MCCPPThPor.h"
//#include "sylvan.h"
//#include <sylvan_int.h>
#include <functional>
#include <iostream>


#include "SylvanWrapper.h"

#define GETNODE(mdd) ((mddnode_t)llmsset_index_to_ptr(nodes, mdd))

using namespace std;

MCCPPThPor::MCCPPThPor(NewNet &R, int nbThread) :
        ModelCheckBaseMT(R, nbThread) {
}


void MCCPPThPor::preConfigure() {
    initializeLDD();
    loadNetFromFile();
    ComputeTh_Succ();
}

/*
 * Compute SOG with POR reduction
 */
void MCCPPThPor::Compute_successors() {
    uint16_t id_thread;
    id_thread=m_id_thread++;
    Set fireObs;
    bool _div,_dead;
    if (id_thread == 0) {
        auto *initialNode = new LDDState;
        initialNode->m_lddstate=saturatePOR(m_initialMarking,fireObs,_div,_dead);
        //initialNode->setDiv(_div);
        initialNode->setDiv(Set_Div(initialNode->m_lddstate));
        initialNode->setDeadLock(_dead);
        m_graph->setInitialState(initialNode);
        m_graph->insert(initialNode);
        m_common_stack.push(Pair(couple(initialNode, initialNode->m_lddstate), fireObs));
        m_condStack.notify_one();
        m_finish_initial = true;
    }

    Pair e;
    do {
        std::unique_lock<std::mutex> lk(m_mutexStack);
        m_condStack.wait(lk, std::bind(&MCCPPThPor::hasToProcess, this));
        lk.unlock();
        if (m_common_stack.try_pop(e) && !m_finish) {
            while (!e.second.empty() && !m_finish) {
                int t = *e.second.begin();
                e.second.erase(t);
                MDD reducedMS = saturatePOR(get_successor(e.first.second, t),fireObs,_div,_dead);
                SylvanWrapper::lddmc_refs_push(reducedMS);
                bool res;
                LDDState *pos = m_graph->insertFindByMDD(reducedMS, res);
                m_graph->addArc();
                if (!res) {
                    pos->setDeadLock(_dead);
                    //pos->setDiv(Set_Div(pos->m_lddstate));
                    pos->setDiv(_div);
                    m_common_stack.push(Pair(couple(pos, reducedMS), fireObs));
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


void MCCPPThPor::threadHandler(void *context) {
    SylvanWrapper::lddmc_refs_init();
    ((MCCPPThPor *) context)->Compute_successors();
}

void MCCPPThPor::ComputeTh_Succ() {
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

MCCPPThPor::~MCCPPThPor() {
    m_finish = true;
    m_condStack.notify_all();
    for (int i = 0; i < m_nb_thread; i++) {
        m_list_thread[i]->join();
        delete m_list_thread[i];
    }
}

bool MCCPPThPor::hasToProcess() const {
    return m_finish || !m_common_stack.empty();
}
