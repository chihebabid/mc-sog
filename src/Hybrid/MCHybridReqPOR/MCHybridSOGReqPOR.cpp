#include "MCHybridSOGReqPOR.h"
//#define DEBUG_GC

#include <cstdio>
#include <openssl/md5.h>
#include "misc/md5_hash.h"

#define LENGTH_ID 16
//#define LENGTH_MSG 180
#define TAG_STATE 1
#define TAG_FINISH 2
#define TAG_INITIAL 3
#define TAG_ASK_SUCC 4


#define TAG_ACK_INITIAL 8
#define TAG_ASK_STATE 9
#define TAG_ACK_STATE 10
#define TAG_ACK_SUCC 11

//#define DEBUG_GC 1
#define REDUCE 1
using namespace std;

MCHybridSOGReqPOR::MCHybridSOGReqPOR(const NewNet &R, MPI_Comm &comm_world, bool init):MCHybridSOGReq(R,comm_world,init) {

}

void *MCHybridSOGReqPOR::doCompute() {

    int id_thread;
    id_thread = m_id_thread++;
    unsigned char Identif[20];
    m_Terminated = false;
    Set fireObs;
    bool _div,_dead;
    /****************************************** initial state ********************************************/
    if (task_id == 0 && id_thread == 0) {
        string *chaine = new string();
        LDDState *c = new LDDState;
        MDD Complete_meta_state=saturatePOR(m_initialMarking,fireObs,_div,_dead);
        c->m_lddstate = Complete_meta_state;
        SylvanWrapper::convert_wholemdd_stringcpp(Complete_meta_state, *chaine);
        md5_hash::compute(*chaine, Identif);
        auto destination = (uint16_t) (Identif[0] % n_tasks);
        c->setProcess(destination);
        if (destination == 0) {
            m_nbmetastate++;
            m_graph->setInitialState(c);
            m_graph->insert(c);
            memcpy(c->m_SHA2, Identif, 16);
            m_common_stack.push(Pair(couple(c, Complete_meta_state), fireObs));
            m_condStack.notify_one();

        } else {
            MDD initialstate = Complete_meta_state;
            m_graph->insertSHA(c);
            memcpy(c->m_SHA2, Identif, 16);
            //initialstate = Complete_meta_state;
            SylvanWrapper::convert_wholemdd_stringcpp(initialstate, *chaine);
            m_tosend_msg.push(MSG(chaine, destination));
        }
    }
    /******************************* envoie et reception des aggregats ************************************/
    string chaine;
    if (id_thread == 0) {
        do {
            send_state_message();
            read_message();
            if (m_waitingBuildSucc == statereq::waitingBuild) {
                bool res;
                size_t pos = m_graph->findSHAPos(m_id_md5, res);
                if (res) {
                    m_waitingBuildSucc = statereq::waitingSucc;
                    m_aggWaiting = m_graph->getLDDStateById(pos);
                }
            }
            if (m_waitingBuildSucc == statereq::waitingSucc) {
                if (m_aggWaiting->isCompletedSucc()) {
                    m_waitingBuildSucc = statereq::waitInitial;
                    sendSuccToMC();
                }
            }
            if (m_waitingAgregate) {
                bool res = false;
                size_t pos = m_graph->findSHAPos(m_id_md5, res);
                if (res) {
                    m_waitingAgregate = false;
                    sendPropToMC(pos);
                }
            }

        } while (!m_Terminated);

    }
        /******************************* Construction des aggregats ************************************/

    else {
        while (!m_Terminated) {
            {
                std::unique_lock lk(m_mutexCond);
                m_condStack.wait(lk,
                                 [this] { return !m_common_stack.empty() || !m_received_msg.empty() || m_Terminated; });
            }
            /*if ( id_thread>1 ) {
                if ( ++m_gc==1 ) {
                    m_gc_mutex.lock();
                }
            }*/
            if (!m_Terminated) {
                Pair e;
                if (m_common_stack.try_pop(e)) {
                    while (!e.second.empty() && !m_Terminated) {
                        int t = *(e.second.begin());
                        e.second.erase(t);
                        MDD ldd_reachedclass=saturatePOR(get_successor(e.first.second, t),fireObs,_div,_dead);
                        string *message_to_send1 = new string();
                        SylvanWrapper::convert_wholemdd_stringcpp(ldd_reachedclass, *message_to_send1);
                        md5_hash::compute(*message_to_send1, Identif);
                        uint16_t destination = (uint16_t) (Identif[0] % n_tasks);
                        /**************** construction local ******/
                        if (destination == task_id) {
                            bool res = false;
                            LDDState *pos = m_graph->insertFindByMDD(ldd_reachedclass, res);
                            if (!res) {
                                pos->setProcess(task_id);
                                pos->setDiv(Set_Div(ldd_reachedclass));
                                pos->setDeadLock(_dead);
                                memcpy(pos->m_SHA2, Identif, 16);
                                m_common_stack.push(Pair(couple(pos, pos->m_lddstate), fireObs));
                                m_condStack.notify_one();
                            }
                            m_graph->addArc();
                            {
                                std::scoped_lock<std::mutex> guard(m_graph_mutex);
                                e.first.first->Successors.insert(e.first.first->Successors.begin(),
                                                                 LDDEdge(pos, t));
                                pos->Predecessors.insert(pos->Predecessors.begin(), LDDEdge(e.first.first, t));
                            }
                        }
                            /**************** construction externe ******/
                        else { // send to another process
                            LDDState *reached_class = new LDDState;
                            reached_class->m_lddstate = ldd_reachedclass;
                            reached_class->setProcess(destination);
                            LDDState *posV = m_graph->insertFindSha(Identif, reached_class);
                            if (!posV) {
                                MDD reached{ldd_reachedclass};
                                string *message_to_send2 = new string();
                                SylvanWrapper::convert_wholemdd_stringcpp(reached, *message_to_send2);
                                m_graph->addArc();
                                m_graph_mutex.lock();
                                e.first.first->Successors.insert(e.first.first->Successors.begin(),
                                                                 LDDEdge(reached_class, t));
                                reached_class->Predecessors.insert(reached_class->Predecessors.begin(),
                                                                   LDDEdge(e.first.first, t));
                                m_graph_mutex.unlock();
                                m_tosend_msg.push(MSG(message_to_send2, destination));

                            } else {
                                //cout<<"sha found "<<endl;
                                m_graph->addArc();
                                {
                                    std::scoped_lock<std::mutex> guard(m_graph_mutex);
                                    e.first.first->Successors.insert(e.first.first->Successors.begin(),
                                                                     LDDEdge(posV, t));
                                    posV->Predecessors.insert(posV->Predecessors.begin(), LDDEdge(e.first.first, t));
                                }
                                delete reached_class;
                            }
                        }
                    }
                    e.first.first->setCompletedSucc();
                }

                /******************************* Construction des aggregats � partir de pile de messages ************************************/
                MSG popped_msg;
                if (m_received_msg.try_pop(popped_msg)) {
                    MDD receivedLDD = decodage_message(popped_msg.first->c_str());
                    delete popped_msg.first;
                    MDD MState =saturatePOR(receivedLDD,fireObs,_div,_dead);
                    bool res = false;
                    LDDState *Agregate = m_graph->insertFindByMDD(MState, res);
                    if (!res) {
                        Agregate->setDiv(Set_Div(MState));
                        Agregate->setDeadLock(_dead);
                        Agregate->setProcess(task_id);
                        SylvanWrapper::convert_wholemdd_stringcpp(MState, chaine);
                        md5_hash::compute(chaine, Identif);
                        memcpy(Agregate->m_SHA2, Identif, 16);
                        m_common_stack.push(Pair(couple(Agregate, MState), fireObs));
                        m_condStack.notify_one();
                    }
                }

                /*    if ( id_thread>1 ) {
                        if ( --m_gc==0 ) {
                            m_gc_mutex.unlock();
                        }

                    }*/
                // }

            }
        }    /// End else

        // cout<<"Working process "<<task_id<<" leaved...thread "<<id_thread<<endl;

    }

    //  cout<<"Finnnnnn...."<<endl;
}










MCHybridSOGReqPOR::~MCHybridSOGReqPOR() = default;


