#include "MCHybridSOG.h"
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

MCHybridSOG::MCHybridSOG(const NewNet &R, MPI_Comm &comm_world, bool init) {
    m_comm_world = comm_world;
    initializeLDD();
    m_net = &R;
    m_init = init;
    m_nbPlaces = R.places.size();
    loadNetFromFile();
}

void *MCHybridSOG::doCompute() {

    int id_thread;
    id_thread = m_id_thread++;
    unsigned char Identif[20];
    m_Terminated = false;

    /****************************************** initial state ********************************************/
    if (task_id == 0 && id_thread == 0) {
        string *chaine = new string();
        LDDState *c = new LDDState;
        MDD Complete_meta_state = Accessible_epsilon(m_initialMarking);
        c->m_lddstate = Complete_meta_state;
        //SylvanWrapper::lddmc_refs_push ( Complete_meta_state );
        //   MDD reduced_initialstate=Canonize(Complete_meta_state,0);

        SylvanWrapper::convert_wholemdd_stringcpp(Complete_meta_state, *chaine);
        md5_hash::compute(*chaine, Identif);


        //lddmc_getsha(Complete_meta_state, Identif);
        auto destination = (uint16_t) (Identif[0] % n_tasks);
        c->setProcess(destination);
        if (destination == 0) {
            m_nbmetastate++;
            Set fire = firable_obs(Complete_meta_state);
            m_graph->setInitialState(c);
            m_graph->insert(c);
            memcpy(c->m_SHA2, Identif, 16);
            m_common_stack.push(Pair(couple(c, Complete_meta_state), fire));
            m_condStack.notify_one();
        } else {
            MDD initialstate = Complete_meta_state;
            m_graph->insertSHA(c);
            memcpy(c->m_SHA2, Identif, 16);
            initialstate = Canonize(Complete_meta_state, 0);
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
                if (m_waitingBuildSucc==state::waitingBuild) {
                    bool res;
                    size_t pos = m_graph->findSHAPos(m_id_md5, res);
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
                        MDD ldd_reachedclass = Accessible_epsilon(get_successor(e.first.second, t));
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
                        string *message_to_send1 = new string();
                        SylvanWrapper::convert_wholemdd_stringcpp(ldd_reachedclass, *message_to_send1);
                        md5_hash::compute(*message_to_send1, Identif);
                        uint16_t destination = (uint16_t) (Identif[0] % n_tasks);
                        /**************** construction local ******/
                        // cout<<"debut boucle pile process "<<task_id<< " thread "<< id_thread<<endl;

                        if (destination == task_id) {
                            bool res = false;
                            LDDState *pos = m_graph->insertFindByMDD(ldd_reachedclass, res);
                            if (!res) {
                                pos->setProcess(task_id);
                                pos->setDiv(Set_Div(ldd_reachedclass));
                                pos->setDeadLock(Set_Bloc(ldd_reachedclass));
                                memcpy(pos->m_SHA2, Identif, 16);
                                Set fire = firable_obs(pos->m_lddstate);
                                m_common_stack.push(Pair(couple(pos, pos->m_lddstate), fire));
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
                                reached = Canonize(ldd_reachedclass, 0);
#ifndef REDUCE
                                reached = Canonize(ldd_reachedclass, 0);
                                // SylvanWrapper::lddmc_refs_push( reached );
#endif

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
                    MDD receivedLDD = decodage_message(popped_msg.first->c_str());
                    delete popped_msg.first;
                    MDD MState = Accessible_epsilon(receivedLDD);
                    bool res = false;
                    LDDState *Agregate = m_graph->insertFindByMDD(MState, res);
                    if (!res) {
                        Agregate->setDiv(Set_Div(MState));
                        Agregate->setDeadLock(Set_Bloc(MState));
                        Agregate->setProcess(task_id);
                        SylvanWrapper::convert_wholemdd_stringcpp(MState, chaine);
                        md5_hash::compute(chaine, Identif);
                        memcpy(Agregate->m_SHA2, Identif, 16);
                        Set fire = firable_obs(MState);
                        m_common_stack.push(Pair(couple(Agregate, MState), fire));
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


void MCHybridSOG::read_message() {
    int flag = 0;

    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &m_status); // exist a msg to receiv?
//    int nbytes;
//    MPI_Get_count(&m_status, MPI_CHAR, &nbytes);
//    cout<<"Count of bytes "<<nbytes<<endl;
    while (flag != 0) {

        if (m_status.MPI_TAG == TAG_ASK_STATE) {
            //cout<<"TAG ASKSTATE received by task "<<task_id<<" from "<<m_status.MPI_SOURCE<<endl;
            char mess[17];
            MPI_Recv(mess, 16, MPI_BYTE, m_status.MPI_SOURCE, m_status.MPI_TAG, MPI_COMM_WORLD, &m_status);
            bool res;
            m_waitingAgregate = false;
            size_t pos = m_graph->findSHAPos(mess, res);
            if (!res) {
                m_waitingAgregate = true;
                memcpy(m_id_md5, mess, 16);
            } else {
                sendPropToMC(pos);
            }
            //cout<<"TAG ASKSTATE completed by task "<<task_id<<" from "<<m_status.MPI_SOURCE<<endl;


        } else if (m_status.MPI_TAG == TAG_ASK_SUCC) {
            //cout<<"=============================TAG_ASK_SUCC received by task "<<task_id<<endl;
            char mess[17];
            MPI_Recv(mess, 16, MPI_BYTE, m_status.MPI_SOURCE, m_status.MPI_TAG, MPI_COMM_WORLD, &m_status);
            m_waitingBuildSucc=state::waitInitial;
            bool res;

            size_t pos = m_graph->findSHAPos(mess, res);

            if (res) {
                m_aggWaiting = m_graph->getLDDStateById(pos);
                if (m_aggWaiting->isCompletedSucc()) {
                    sendSuccToMC();
                } else {
                    m_waitingBuildSucc=state::waitingSucc;
                }
            } else {
                m_waitingBuildSucc=state::waitingBuild;
                memcpy(m_id_md5, mess, 16);
            }
        }
        else if (m_status.MPI_TAG == TAG_STATE) {
            int nbytes;
            MPI_Get_count(&m_status, MPI_CHAR, &nbytes);
            char inmsg[nbytes + 1];
            MPI_Recv(inmsg, nbytes, MPI_CHAR, m_status.MPI_SOURCE, TAG_STATE, MPI_COMM_WORLD, &m_status);
            m_nbrecv++;
            string *msg_receiv = new string(inmsg, nbytes);
            m_received_msg.push(MSG(msg_receiv, 0));
            m_condStack.notify_one();
        }
        else if (m_status.MPI_TAG == TAG_FINISH) {
            int v;
            MPI_Recv(&v, 1, MPI_INT, m_status.MPI_SOURCE, m_status.MPI_TAG, MPI_COMM_WORLD, &m_status);
            m_Terminated = true;
            m_condStack.notify_all();
        }
        else if    (m_status.MPI_TAG == TAG_INITIAL) {
            int v;
                MPI_Recv(&v, 1, MPI_INT, m_status.MPI_SOURCE, m_status.MPI_TAG, MPI_COMM_WORLD, &m_status);
                LDDState *i_agregate = m_graph->getInitialState();
                char message[22];
                memcpy(message, i_agregate->getSHAValue(), 16);
                uint16_t id_p = i_agregate->getProcess();
                memcpy(message + 16, &id_p, 2);
                MPI_Send(message, 22, MPI_BYTE, m_status.MPI_SOURCE, TAG_ACK_INITIAL, MPI_COMM_WORLD);

        }
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &m_status);
    }

}



/**
 * send a message containing an aggregate
 */
void MCHybridSOG::send_state_message() {
    MSG s;
    while (m_tosend_msg.try_pop(s) && !m_Terminated) {
        int message_size;
        message_size = s.first->size() + 1;
        int destination = s.second;
        //MPI_Request request;
        read_message();
        MPI_Send(s.first->c_str(), message_size, MPI_CHAR, destination, TAG_STATE, MPI_COMM_WORLD);//, &request);
        /*int flag;
        do {
            read_message();
            MPI_Test(&request, &flag, &m_status);
        } while (!flag);*/
        m_nbsend++;
        m_size_mess += message_size;
        delete s.first;
    }
}

/*
 * Creation of threads
 */
void MCHybridSOG::computeDSOG(LDDGraph &g) {
    m_graph = &g;
    MPI_Barrier(m_comm_world);
    m_nb_thread = nb_th;
    m_id_thread = 0;
    m_nbrecv = 0;
    m_nbsend = 0;
    m_gc = 0;

    for (uint8_t i = 0; i < m_nb_thread - 1; ++i) {
        m_list_thread[i] = new thread(threadHandler, this);
        if (m_list_thread[i] == nullptr) {
            cout << "error: thread creation failed..." << endl;
        }
    }

    doCompute();

    for (int i = 0; i < m_nb_thread - 1; i++) {
        m_list_thread[i]->join();
        delete m_list_thread[i];
    }
}


MCHybridSOG::~MCHybridSOG()=default;

/******************************************convert char ---> MDD ********************************************/

MDD MCHybridSOG::decodage_message(const char *chaine) {
    MDD M = lddmc_false;
    unsigned int nb_marq = (((unsigned char) chaine[1] - 1)<<7) | ((unsigned char) chaine[0] >> 1);
    //nb_marq = (nb_marq << 7) | ((unsigned char) chaine[0] >> 1);
    unsigned int index = 2;
    uint32_t list_marq[m_nbPlaces];
    for (unsigned int i = 0; i < nb_marq; ++i) {
        for (unsigned int j = 0; j < m_nbPlaces; ++j) {
            list_marq[j] = (uint32_t) ((unsigned char) chaine[index] - 1);
            index++;
        }
        MDD N = SylvanWrapper::lddmc_cube(list_marq, m_nbPlaces);
        M = SylvanWrapper::lddmc_union_mono(M, N);
    }
    return M;
}

void *MCHybridSOG::threadHandler(void *context) {
    return ((MCHybridSOG *) context)->doCompute();
}


void MCHybridSOG::sendSuccToMC() {
    uint32_t nb_succ = m_aggWaiting->getSuccessors()->size();
    uint32_t message_size = nb_succ * 20 + 4;
    char mess_tosend[message_size];
    memcpy(mess_tosend, &nb_succ, 4);
    uint32_t i_message = 4;
    //cout<<"***************************Number of succesors to send :"<<nb_succ<<endl;
    for (uint32_t i = 0; i < nb_succ; i++) {
        pair<LDDState *, int> elt;
        elt = m_aggWaiting->getSuccessors()->at(i);
        memcpy(mess_tosend + i_message, elt.first->getSHAValue(), 16);
        i_message += 16;
        uint16_t pcontainer = elt.first->getProcess();
        memcpy(mess_tosend + i_message, &pcontainer, 2);
        i_message += 2;
        memcpy(mess_tosend + i_message, &(elt.second), 2);
        i_message += 2;
    }
    read_message();
    MPI_Send(mess_tosend, message_size, MPI_BYTE, n_tasks, TAG_ACK_SUCC, MPI_COMM_WORLD);

}

// Send propositions to Model checker
void MCHybridSOG::sendPropToMC(size_t pos) {
    LDDState *agg = m_graph->getLDDStateById(pos);
    vector<uint16_t> marked_places = agg->getMarkedPlaces(m_place_proposition);
    vector<uint16_t> unmarked_places = agg->getUnmarkedPlaces(m_place_proposition);
    uint16_t s_mp = marked_places.size();
    uint16_t s_up = unmarked_places.size();
    char mess_to_send[8 + s_mp * 2 + s_up * 2 + 5];
    memcpy(mess_to_send, &pos, 8);
    size_t indice = 8;
    memcpy(mess_to_send + indice, &s_mp, 2);
    indice += 2;
    for (auto it = marked_places.begin(); it != marked_places.end();++it) {
        memcpy(mess_to_send + indice, &(*it), 2);
        indice += 2;
    }
    memcpy(mess_to_send + indice, &s_up, 2);
    indice += 2;
    for (auto it : unmarked_places) {
        memcpy(mess_to_send + indice, &(it), 2);
        indice += 2;
    }
    uint8_t divblock = agg->isDiv();
    divblock = divblock | (agg->isDeadLock() << 1);
    memcpy(mess_to_send + indice, &divblock, 1);
    indice++;
    read_message();
    MPI_Send(mess_to_send, indice, MPI_BYTE, n_tasks, TAG_ACK_STATE, MPI_COMM_WORLD);
}