//
// Created by chiheb on 06/02/2022.
//
#include <cstdio>
#include <openssl/md5.h>
#include "misc/md5_hash.h"
#include "MCHybridSOGPOR.h"

using namespace std;

MCHybridSOGPOR::MCHybridSOGPOR(const NewNet &R, MPI_Comm &comm_world, bool init) : MCHybridSOG(R,comm_world,init){

}

/*
 * Creation of threads
 */
void MCHybridSOGPOR::computeDSOG(LDDGraph &g) {
    m_graph = &g;
    MPI_Barrier(m_comm_world);
    m_nb_thread = nb_th;
    m_id_thread = 0;
    m_nbrecv = 0;
    m_nbsend = 0;
    m_gc = 0;

    for (uint8_t i = 0; i < m_nb_thread - 1; ++i) {
        m_list_thread[i] = new thread(threadHandlerPOR, this);
        if (m_list_thread[i] == nullptr) {
            cout << "error: thread creation failed..." << endl;
        }
    }

    doCompute();

    for (int i = 0; i < m_nb_thread - 1; ++i) {
        m_list_thread[i]->join();
        delete m_list_thread[i];
    }
}

void *MCHybridSOGPOR::threadHandlerPOR(void *context) {
    return ((MCHybridSOGPOR *) context)->doCompute();
}



void *MCHybridSOGPOR::doCompute() {
    int id_thread {m_id_thread++};
    unsigned char Identif[20];
    m_Terminated = false;
    Set fireObs;
    bool _dead,_div;
    /****************************************** initial state ********************************************/
    if (task_id == 0 && id_thread == 0) {
        string *chaine = new string();
        LDDState *c = new LDDState;
        MDD Complete_meta_state =saturatePOR(m_initialMarking,fireObs,_div,_dead);
        c->m_lddstate = Complete_meta_state;
        //SylvanWrapper::lddmc_refs_push ( Complete_meta_state );
        //   MDD reduced_initialstate=Canonize(Complete_meta_state,0);

        SylvanWrapper::convert_wholemdd_stringcpp(Complete_meta_state, *chaine);
        md5_hash::compute(*chaine, Identif);


        //lddmc_getsha(Complete_meta_state, Identif);
        auto destination  {(uint16_t) (Identif[0] % n_tasks)};
        c->setProcess(destination);
        if (destination == 0) {
            ++m_nbmetastate;
            m_graph->setInitialState(c);
            m_graph->insert(c);
            memcpy(c->m_SHA2, Identif, 16);
            m_common_stack.push(Pair(couple(c, Complete_meta_state), fireObs));
            m_condStack.notify_one();
        } else {
            m_graph->insertSHA(c);
            memcpy(c->m_SHA2, Identif, 16);
            //initialstate = Complete_meta_state;
            SylvanWrapper::convert_wholemdd_stringcpp(Complete_meta_state, *chaine);
            m_tosend_msg.push(MSG(chaine, destination));

        }
    }




    /******************************* envoie et reception des aggregats ************************************/
    string chaine;
    if (id_thread == 0) {
        do {
            send_state_message();
            read_message();
            if (m_waitingBuildSucc==state::waitingBuild) {
                bool res;
                size_t&& pos = m_graph->findSHAPos(m_id_md5, res);
                if (res) {
                    m_waitingBuildSucc=state::waitingSucc;
                    m_aggWaiting = m_graph->getLDDStateById(pos);
                }
            }
            if (m_waitingBuildSucc==state::waitingSucc) {
                if (m_aggWaiting->isCompletedSucc()) {
                    m_waitingBuildSucc = state::waitInitial;
                    sendSuccToMC();
                }
            }
            if (m_waitingAgregate) {
                bool res {false};
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
            std::unique_lock<std::mutex> lk(m_mutexCond);
            m_condStack.wait(lk, [this] { return !m_common_stack.empty() || !m_received_msg.empty() || m_Terminated; });
            lk.unlock();

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
                        MDD&& ldd_reachedclass=saturatePOR(get_successor(e.first.second, t),fireObs,_div,_dead);
                        // SylvanWrapper::lddmc_refs_push ( ldd_reachedclass );

                        //if ( id_thread==1 ) {
                        //displayMDDTableInfo();
                        //if ( SylvanWrapper::isGCRequired() ) {
                        //     m_gc_mutex.lock();
//#ifdef DEBUG_GC

                        // SylvanWrapper::displayMDDTableInfo();
//#endif // DEBUG_GC
                        //SylvanWrapper::sylvan_gc_seq();
//#ifdef DEBUG_GC

                        // SylvanWrapper::displayMDDTableInfo();
//#endif // DEBUG_GC
                        //m_gc_mutex.unlock();
                        // }


                        //pthread_spin_unlock(&m_spin_md5);



                        //MDD Reduced=Canonize(ldd_reachedclass,0);
                        // ldd_refs_push(Reduced);

                        //MDD Reduced=ldd_reachedclass;
                        string *message_to_send1 {new string()};
                        SylvanWrapper::convert_wholemdd_stringcpp(ldd_reachedclass, *message_to_send1);
                        md5_hash::compute(*message_to_send1, Identif);
                        uint16_t&& destination = (uint16_t) (Identif[0] % n_tasks);
                        /**************** construction local ******/
                        // cout<<"debut boucle pile process "<<task_id<< " thread "<< id_thread<<endl;

                        if (destination == task_id) {
                            bool res {false};
                            LDDState *pos {m_graph->insertFindByMDD(ldd_reachedclass, res)};
                            if (!res) {
                                pos->setProcess(task_id);
                                pos->setDiv(_div);
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
                                MDD reached=ldd_reachedclass;

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
                                //cout<<"quit  C"<<endl;
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
                    MDD receivedLDD {decodage_message(popped_msg.first->c_str())};
                    delete popped_msg.first;
                    MDD MState=saturatePOR(receivedLDD,fireObs,_div,_dead);
                    bool res {false};
                    LDDState *Agregate {m_graph->insertFindByMDD(MState, res)};
                    if (!res) {
                        Agregate->setDiv(_div);
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

MCHybridSOGPOR::~MCHybridSOGPOR()=default;