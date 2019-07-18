#include "MCHybridSOG.h"
//#define DEBUG_GC
#define REDUCE
using namespace std;
#include <stdio.h>

#include "sylvan_seq.h"
#include <sylvan_int.h>

using namespace sylvan;
#include <openssl/md5.h>
#define GETNODE(mdd) ((mddnode_t)llmsset_index_to_ptr(nodes, mdd))

#define LENGTH_ID 16
//#define LENGTH_MSG 180
#define TAG_STATE 1
#define TAG_FINISH 2
#define TAG_INITIAL 3
#define TAG_ASK_SUCC 4

#define TAG_AGREGATE 5
#define TAG_ACK_INITIAL 8

MCHybridSOG::MCHybridSOG(const NewNet &R,MPI_Comm &comm_world, int BOUND,bool init)
{

    m_comm_world=comm_world;
    lace_init(1, 0);
    lace_startup(0, NULL, NULL);

    // Simple Sylvan initialization, also initialize MDD support
    sylvan_set_limits(2LL<<31, 2, 1);   // sylvan_set_limits(2LL<<31, 2, 1);
    //sylvan_set_sizes(1LL<<27, 1LL<<31, 1LL<<20, 1LL<<22);

    sylvan_init_package();
    sylvan_init_ldd();
    sylvan_gc_enable();
    m_net=R;

    m_init=init;
    m_nbPlaces=R.places.size();
    int i, domain;
    vector<Place>::const_iterator it_places;

    init_gc_seq();
    //_______________
    transitions=R.transitions;
    m_observable=R.Observable;
    m_nonObservable=R.NonObservable;
    m_place_proposition=R.m_formula_place;
    m_transitionName=R.transitionName;
    m_placeName=R.m_placePosName;
    InterfaceTrans=R.InterfaceTrans;

    m_nbPlaces=R.places.size();
    cout<<"Nombre de places : "<<m_nbPlaces<<endl;
    cout<<"Derniere place : "<<R.places[m_nbPlaces-1].name<<endl;
    // place domain, place bvect, place initial marking and place name

    uint32_t * liste_marques=new uint32_t[R.places.size()];
    for(i=0,it_places=R.places.begin(); it_places!=R.places.end(); i++,it_places++)
    {
        liste_marques[i] =it_places->marking;
    }
    M0=lddmc_cube(liste_marques,R.places.size());
    ldd_refs_push(M0);

    delete []liste_marques;
    // place names



    uint32_t *prec = new uint32_t[m_nbPlaces];
    uint32_t *postc= new uint32_t [m_nbPlaces];
    // Transition relation

    for(vector<Transition>::const_iterator t=R.transitions.begin();
            t!=R.transitions.end(); t++)
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
            
        }
        // arcs post
        for(vector< pair<int,int> >::const_iterator it=t->post.begin(); it!=t->post.end(); it++)
        {
            adjacentPlace.insert(it->first);
            postc[it->first] = postc[it->first] + it->second;
        }

        MDD _minus=lddmc_cube(prec,m_nbPlaces);
        ldd_refs_push(_minus);

        MDD _plus=lddmc_cube(postc,m_nbPlaces);
        ldd_refs_push(_plus);
        m_tb_relation.push_back(TransSylvan(_minus,_plus));
    }
    //sylvan_gc_seq();
    delete [] prec;
    delete [] postc;

}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// Version distribu�e en utilisant les LDD - MPI/////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void *MCHybridSOG::doCompute()
{
    int min_charge=1;
    int id_thread;

    pthread_mutex_lock(&m_mutex);
    id_thread=m_id_thread++;
    pthread_mutex_unlock(&m_mutex);

    unsigned char Identif[20];
    m_Terminated=false;
    m_Terminating=false;
    /****************************************** initial state ********************************************/
    if (task_id==0 && id_thread==0)
    {
        string* chaine=new string();


        LDDState *c=new LDDState;
        MDD Complete_meta_state=Accessible_epsilon(M0);
        c->m_lddstate=Complete_meta_state;
        ldd_refs_push(Complete_meta_state);
     //   MDD reduced_initialstate=Canonize(Complete_meta_state,0);

        convert_wholemdd_stringcpp(Complete_meta_state,*chaine);
        get_md5(*chaine,Identif);
        //lddmc_getsha(Complete_meta_state, Identif);
        int destination=(int)(Identif[0]%n_tasks);

        if(destination==0)
        {

            m_nbmetastate++;
            Set fire=firable_obs(Complete_meta_state);
            m_st[1].push(Pair(couple(c,Complete_meta_state),fire));
            m_graph->setInitialState(c);
            m_graph->insert(c);
            m_charge[1]++;
            strcpySHA(c->m_SHA2,Identif);
        }
        else
        {
            MDD initialstate=Complete_meta_state;
            //cout<<"Initial marking is sent"<<endl;
            m_graph->insertSHA(c);
            strcpySHA(c->m_SHA2,Identif);
           // MDD initialstate=Complete_meta_state;

            //pthread_spin_lock(&m_spin_md5);
            //pthread_spin_unlock(&m_spin_md5);
            //write_to_dot("reduced.dot",Reduced);
            //pthread_spin_unlock(&m_spin_md5);
            //#ifndef
//            if (strcmp(red,'C')==0)
            {
            initialstate=Canonize(Complete_meta_state,0);
            }
            //#endif // REDUCE
            convert_wholemdd_stringcpp(initialstate,*chaine);


            pthread_mutex_lock(&m_spin_msg[0]);
            m_msg[0].push(MSG(chaine,destination));
            pthread_mutex_unlock(&m_spin_msg[0]);
            delete c;
        }
    }


    while (m_Terminated==false)
    {

        /******************************* envoie et reception des aggregats ************************************/

        if (id_thread==0)
        {

            do
            {
                send_state_message();
                read_message();
            }
            while (isNotTerminated()); 


            

        }
        /******************************* Construction des aggregats ************************************/

        else
        {

                if (!m_st[id_thread].empty() || !m_msg[id_thread].empty())
                {
                pthread_mutex_lock(&m_spin_working);
                m_working++;
                pthread_mutex_unlock(&m_spin_working);

                    if (id_thread>1)
                    {
                        pthread_mutex_lock(&m_supervise_gc_mutex);
                        m_gc++;
                        if (m_gc==1) pthread_mutex_lock(&m_gc_mutex);
                        pthread_mutex_unlock(&m_supervise_gc_mutex);
                    }

                    if (!m_st[id_thread].empty())
                    {

                        Pair  e;
                        pthread_mutex_lock(&m_spin_stack[id_thread]);
                        e=m_st[id_thread].top();
                        m_st[id_thread].pop();
                        pthread_mutex_unlock(&m_spin_stack[id_thread]);
                        m_charge[id_thread]--;

                        LDDState *reached_class=NULL;
                        

                        while(!e.second.empty())
                        {
                            /*#ifdef DEBUG_GC
                            displayMDDTableInfo();
                                #endif // DEBUG_GC*/

                            int t = *(e.second.begin());
                            e.second.erase(t);
                            reached_class=new LDDState();


                            MDD ldd_reachedclass=Accessible_epsilon(get_successor(e.first.second,t));

                            ldd_refs_push(ldd_reachedclass);

                            if (id_thread==1)
                                if (isGCRequired())
                                {
                                    pthread_mutex_lock(&m_gc_mutex);
#ifdef DEBUG_GC

                                    displayMDDTableInfo();
#endif // DEBUG_GC
                                    sylvan_gc_seq();
#ifdef DEBUG_GC

                                    displayMDDTableInfo();
#endif // DEBUG_GC
                                    pthread_mutex_unlock(&m_gc_mutex);
                                }


                            //pthread_spin_unlock(&m_spin_md5);

                            reached_class->m_lddstate=ldd_reachedclass;

                            //MDD Reduced=Canonize(ldd_reachedclass,0);
                           // ldd_refs_push(Reduced);

                                    //MDD Reduced=ldd_reachedclass;
                            string* message_to_send1=new string();
                            convert_wholemdd_stringcpp(ldd_reachedclass,*message_to_send1);
                            get_md5(*message_to_send1,Identif);


                            //lddmc_getsha(ldd_reachedclass, Identif);



                            unsigned int destination=(unsigned int)(Identif[0]%n_tasks);

                            /**************** construction local ******/
                            // cout<<"debut boucle pile process "<<task_id<< " thread "<< id_thread<<endl;

                            if(destination==task_id)
                            {
                                //      cout << " construction local de l'aggregat " <<endl;
                                pthread_mutex_lock(&m_graph_mutex);
                                LDDState* pos=m_graph->find(reached_class);
                                if(!pos)
                                {
                                    m_graph->insert(reached_class);
                                    pthread_mutex_unlock(&m_graph_mutex);
                                    m_graph->addArc();

                                    e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(reached_class,t));
                                    reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(e.first.first,t));

                                    Set fire=firable_obs(ldd_reachedclass);
                                    min_charge=minCharge();

                                    pthread_mutex_lock(&m_spin_stack[min_charge]);
                                    m_st[min_charge].push(Pair(couple(reached_class,ldd_reachedclass),fire));
                                    pthread_mutex_unlock(&m_spin_stack[min_charge]);
                                    //pthread_mutex_lock(&m_spin_charge);
                                    m_charge[min_charge]++;
                                    //pthread_mutex_unlock(&m_spin_charge);
                                    //      cout<<" I'm local destination "<<task_id<< " thread "<< id_thread<< " msg "<< msg<<endl;
                                }
                                else
                                {
                                    pthread_mutex_unlock(&m_graph_mutex);
                                    m_graph->addArc();
                                    e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(pos,t));
                                    pos->Predecessors.insert(pos->Predecessors.begin(),LDDEdge(e.first.first,t));
                                    delete reached_class;

                                }


                            }
                            /**************** construction externe ******/
                            else // send to another process
                            {
                                pthread_mutex_lock(&m_graph_mutex);
                                LDDState* posV=m_graph->findSHA(Identif);
                                if(!posV)
                                {

                                 MDD reached=ldd_reachedclass;

                                    m_graph->insertSHA(reached_class);
                                    strcpySHA(reached_class->m_SHA2,Identif);
                                    pthread_mutex_unlock(&m_graph_mutex);
                                #ifndef REDUCE
                                reached=Canonize(ldd_reachedclass,0);
                                ldd_refs_push(ldd_reachedclass);
                                #endif
                                    //MDD Reduced=ldd_reachedclass;
                                 string* message_to_send2=new string();
                                 convert_wholemdd_stringcpp(reached,*message_to_send2);
                                 get_md5(*message_to_send2,Identif);



                                    m_graph->addArc();
                                    e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(reached_class,t));
                                    reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(e.first.first,t));

                                    pthread_mutex_lock(&m_spin_msg[0]);
                                    m_msg[0].push(MSG(message_to_send2,destination));
                                    pthread_mutex_unlock(&m_spin_msg[0]);


                                }
                                else
                                {
                                    pthread_mutex_unlock(&m_graph_mutex);
                                    m_graph->addArc();
                                    e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(posV,t));
                                    posV->Predecessors.insert(posV->Predecessors.begin(),LDDEdge(e.first.first,t));
                                    delete reached_class;
                                    // read_state_message();

                                }



                            }


                        }



                    }


                    /******************************* Construction des aggregats � partir de pile de messages ************************************/

                    if (!m_msg[id_thread].empty())
                    {

                        MSG popped_msg;
                        pthread_mutex_lock(&m_spin_msg[id_thread]);
                        popped_msg=m_msg[id_thread].top();
                        m_msg[id_thread].pop();
                        pthread_mutex_unlock(&m_spin_msg[id_thread]);

                        m_charge[id_thread]--;
                        MDD receivedLDD=decodage_message(popped_msg.first->c_str());
                        delete popped_msg.first;
                        // ldd_refs_push(receivedLDD);

                        LDDState* Agregate=new LDDState();
                        MDD MState=Accessible_epsilon(receivedLDD);

                        //cout<<"executed3..."<<endl;
                        Agregate->m_lddstate=MState;
                        pthread_mutex_lock(&m_graph_mutex);
                        if (!m_graph->find(Agregate))
                        {
                            m_graph->insert(Agregate);
                            pthread_mutex_unlock(&m_graph_mutex);
                            Set fire=firable_obs(MState);
                            min_charge=minCharge();
                            pthread_mutex_lock(&m_spin_stack[min_charge]);
                            m_st[min_charge].push(Pair(couple(Agregate,MState),fire));
                            pthread_mutex_unlock(&m_spin_stack[min_charge]);
                            ldd_refs_push(MState);
                            m_charge[min_charge]++;
                        }

                        else
                        {
                            pthread_mutex_unlock(&m_graph_mutex);
                            delete Agregate;
                        }
                    }

                    if (id_thread>1)
                    {
                        pthread_mutex_lock(&m_supervise_gc_mutex);
                        m_gc--;
                        if (m_gc==0) pthread_mutex_unlock(&m_gc_mutex);
                        pthread_mutex_unlock(&m_supervise_gc_mutex);
                    }
                pthread_mutex_lock(&m_spin_working);
                m_working--;
                pthread_mutex_unlock(&m_spin_working);

                } // End    while (m_msg[id_thread].size()>0 || m_st[id_thread].size()>0);

           // }


        }    /// End else
        //cout<<"Working process "<<task_id<<" leaved...thread "<<id_thread<<endl;
    }


}


bool MCHybridSOG::isNotTerminated()
{
    bool res=true;
    int i=0;
    while (i<m_nb_thread && res)
    {
        res=m_st[i].empty() && m_msg[i].empty();
        i++;
    }
    return !res;
}



void MCHybridSOG::read_message()
{
    int flag=0;


    MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&flag,&m_status); // exist a msg to receiv?
    while (flag!=0)
    {
        switch (m_status.MPI_TAG)
        {
        case TAG_STATE :
            //cout<<"TAG STATE received by task "<<task_id<<endl;
            read_state_message();
            break;
        case TAG_FINISH : break;
        case TAG_INITIAL : 
             int v;
             MPI_Recv(&v, 1, MPI_INT, m_status.MPI_SOURCE, m_status.MPI_TAG, MPI_COMM_WORLD, &m_status);
             LDDState *i_agregate=m_graph->getInitialState();
             
             stringstream ss;
             ss<<(task_id);ss<<'\0';
             ss<<i_agregate->getSHAValue();
             cout<<"Ldd Value :"<<i_agregate->getLDDValue()<<endl;
             cout<<"string :"<<ss.str()<<endl;   
             
             MPI_Send(ss.str().c_str(),17,MPI_UNSIGNED_CHAR,m_status.MPI_SOURCE, TAG_ACK_INITIAL, MPI_COMM_WORLD);
             break;
        case TAG_ASK_SUCC : 
            int id;
             MPI_Recv(&id, 1, MPI_INT, m_status.MPI_SOURCE, m_status.MPI_TAG, MPI_COMM_WORLD, &m_status);
            // LDDState *aggregate=m_graph->findSHA()
             // Send successors 
            break;
        case TAG_AGREGATE : break;
        default :  //cout<<"unknown received "<<status.MPI_TAG<<" by task "<<task_id<<endl;
            //AbortTerm();
            break;
            /* int v;
             MPI_Recv(&v, 1, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
             if (task_id!=0)
                 MPI_Send( &v, 1, MPI_INT, 0, TAG_ABORT, MPI_COMM_WORLD);*/
        }
        MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,m_comm_world,&flag,&m_status);
    }

}



void MCHybridSOG::read_state_message()
{
    int min_charge;
    /****************************************** debut reception state ********************************************/
    int nbytes;
    MPI_Get_count(&m_status, MPI_CHAR, &nbytes);
    char inmsg[nbytes+1];
    MPI_Recv(inmsg, nbytes, MPI_CHAR,m_status.MPI_SOURCE,TAG_STATE,m_comm_world, &m_status);
    m_nbrecv++;
    string *msg_receiv =new string(inmsg,nbytes);
    min_charge=minCharge();
    m_charge[min_charge]++;
    pthread_mutex_lock(&m_spin_msg[min_charge]);
    m_msg[min_charge].push(MSG(msg_receiv,0));
    pthread_mutex_unlock(&m_spin_msg[min_charge]);
}




void MCHybridSOG::send_state_message()
{
    while (!m_msg[0].empty())
    {
        pthread_mutex_lock(&m_spin_msg[0]);
        MSG s=m_msg[0].top();
        m_msg[0].pop();
        pthread_mutex_unlock(&m_spin_msg[0]);
        unsigned int message_size;
        message_size=s.first->size()+1;
        int destination=s.second;
        read_message();
        MPI_Send(s.first->c_str(), message_size, MPI_CHAR, destination, TAG_STATE, m_comm_world);      
        m_nbsend++;
        m_size_mess+=message_size;
        delete s.first;
    }
}

void MCHybridSOG::computeDSOG(LDDGraph &g)

{
    m_graph=&g;
    double tps;

    // double  t1=(double)clock() / (double)CLOCKS_PER_SEC;
    cout<<"process with id "<<task_id<<" / "<<n_tasks<<endl;
    MPI_Barrier(m_comm_world);
    cout<<"After!!!!process with id "<<task_id<<endl;
   // int nb_th;
    m_nb_thread=nb_th;
    cout<<"nb de thread"<<nb_th<<endl;
    int rc;
    m_id_thread=0;
    m_nbrecv=0;
    m_nbsend=0;
    for (int i=1; i<m_nb_thread; i++)
    {
        m_charge[i]=0;
    }

    //pthread_barrier_init(&m_barrier1, 0, m_nb_thread);
    //pthread_barrier_init(&m_barrier2, 0, m_nb_thread);

    pthread_mutex_init(&m_mutex, NULL);
    pthread_mutex_init(&m_graph_mutex,NULL);
    pthread_mutex_init(&m_gc_mutex,NULL);
    pthread_mutex_init(&m_supervise_gc_mutex,NULL);
    m_gc=0;
    pthread_spin_init(&m_spin_md5,0);
    pthread_mutex_init(&m_spin_working,0);
    m_working=0;


    for (int i=0; i<m_nb_thread; i++)
    {
        pthread_mutex_init(&m_spin_stack[i], 0);
        pthread_mutex_init(&m_spin_msg[i], 0);


    }

    timespec start, finish;
    if (task_id==0)
    clock_gettime(CLOCK_REALTIME, &start);

    for (int i=0; i<m_nb_thread-1; i++)
    {
        if ((rc = pthread_create(&m_list_thread[i], NULL,threadHandler,this)))
        {
            cout<<"error: pthread_create, rc: "<<rc<<endl;
        }
    }

    doCompute();



    for (int i = 0; i < m_nb_thread-1; i++)
    {
        pthread_join(m_list_thread[i], NULL);
    }



    int sum_nbStates=0;
    int sum_nbArcs=0;
    int nbstate=g.m_nbStates;

    int nbarcs=g.m_nbArcs;
    unsigned long  total_size_mess=0;
    MPI_Reduce(&nbstate, &sum_nbStates, 1, MPI_INT, MPI_SUM, 0, m_comm_world);
    MPI_Reduce(&nbarcs, &sum_nbArcs, 1, MPI_INT, MPI_SUM, 0, m_comm_world);
    MPI_Reduce(&m_size_mess, &total_size_mess, 1, MPI_INT, MPI_SUM, 0, m_comm_world);




    if(task_id==0)
    {
        //

        // cout<<"Nb Iteration internes : "<<m_itint<<endl;*/
        //auto t2 = std::chrono::high_resolution_clock::now();
        //tps=(double)clock() / (double)CLOCKS_PER_SEC-t1;
        clock_gettime(CLOCK_REALTIME, &finish);

        tps = (finish.tv_sec - start.tv_sec);
        tps += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
        std::cout << " TIME OF CONSTRUCTION OF THE SOG "
                  << tps
                  << " seconds\n";

        cout<<"NB AGGREGATES****** "<<sum_nbStates<<endl;
        cout<<"NB ARCS****** "<<sum_nbArcs<<endl;
        cout<<" Size of message ***** "<<total_size_mess<<" bytes"<<endl;
    }
    //MPI_Finalize();


}


int MCHybridSOG::minCharge()
{
    int pos=1;
    int min_charge=m_charge[1];
    for (int i=1; i<m_nb_thread; i++)
    {
        if (m_charge[i]<min_charge)
        {
            min_charge=m_charge[i];
            pos=i;
        }
    }
    return pos;
}


void MCHybridSOG::strcpySHA(unsigned char *dest,const unsigned char *source)
{
    for (int i=0; i<LENGTH_ID; i++)
    {
        dest[i]=source[i];
        //if (source[i]=='\0') exit(0);
    }
    dest[LENGTH_ID]='\0';
}



MCHybridSOG::~MCHybridSOG()
{
    //dtor
}





/******************************************convert char ---> MDD ********************************************/

MDD MCHybridSOG::decodage_message(const char *chaine)
{
    MDD M=lddmc_false;
    unsigned int nb_marq=((unsigned char)chaine[1]-1);
    nb_marq=(nb_marq<<7) | ((unsigned char)chaine[0]>>1);

    unsigned int index=2;
    uint32_t list_marq[m_nbPlaces];
    for (unsigned int i=0; i<nb_marq; i++)
    {
        for (unsigned int j=0; j<m_nbPlaces; j++)
        {
            list_marq[j]=(uint32_t)((unsigned char)chaine[index]-1);
            index++;
        }
        MDD N=lddmc_cube(list_marq,m_nbPlaces);
        M=lddmc_union_mono(M,N);
    }
    return M;
}


/*void HybridSOG::DisplayMessage(const char *chaine)
{
    unsigned int longueur=((unsigned char)chaine[1]-1);
    longueur=(longueur<<7) | ((unsigned char)chaine[0]>>1);
    int index=2;
    printf("Chaine longueur %d pour %d places",longueur,m_nbPlaces);
    for (int nb=0; nb<longueur; nb++)
    {
        for (int i=0; i<m_nbPlaces; i++)
        {

            printf("%d",((unsigned char)chaine[index]-1));
            index++;
        }
        printf(" ");
    }

} */






void * MCHybridSOG::threadHandler(void *context)
{

    return ((MCHybridSOG*)context)->doCompute();
}

void MCHybridSOG::convert_wholemdd_stringcpp(MDD cmark,string &res)
{
    typedef pair<string,MDD> Pair_stack;
    vector<Pair_stack> local_stack;

    unsigned int compteur=0;
    MDD explore_mdd=cmark;

    string chaine;

    res="  ";
    local_stack.push_back(Pair_stack(chaine,cmark));
    do
    {
        Pair_stack element=local_stack.back();
        chaine=element.first;
        explore_mdd=element.second;
        local_stack.pop_back();
        compteur++;
        while ( explore_mdd!= lddmc_false  && explore_mdd!=lddmc_true)
        {
            mddnode_t n_val = GETNODE(explore_mdd);
            if (mddnode_getright(n_val)!=lddmc_false)
            {
                local_stack.push_back(Pair_stack(chaine,mddnode_getright(n_val)));
            }
            unsigned int val = mddnode_getvalue(n_val);

            chaine.push_back((unsigned char)(val)+1);
            explore_mdd=mddnode_getdown(n_val);
        }
        /*printchaine(&chaine);printf("\n");*/
        res+=chaine;
    }
    while (local_stack.size()!=0);
    //delete chaine;

    compteur=(compteur<<1) | 1;
    res[1]=(unsigned char)((compteur>>8)+1);
    res[0]=(unsigned char)(compteur & 255);

}


void  MCHybridSOG::get_md5(const string& chaine,unsigned char *md_chaine)
{
    pthread_spin_lock(&m_spin_md5);
    MD5(chaine.c_str(), chaine.size(),md_chaine);
    pthread_spin_unlock(&m_spin_md5);
    md_chaine[16]='\0';
}
