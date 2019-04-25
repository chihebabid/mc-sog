#include "DistributedSOG.h"
// #include <vec.h>

#include <stdio.h>
// #include "test_assert.h"

#include <sylvan_int.h>
#include "sylvan_seq.h"

using namespace sylvan;

//#include <boost/thread.hpp>
#define LENGTH_ID 80
#define TAG_STATE 123
#define TAG_SEND 232
#define TAG_REC 333
#define TAG_TERM 88

#define GETNODE(mdd) ((mddnode_t)llmsset_index_to_ptr(nodes, mdd))





/**************************************************/


DistributedSOG::DistributedSOG(const net &R, int BOUND,bool init)
{


    lace_init(1, 0);
    lace_startup(0, NULL, NULL);

    // Simple Sylvan initialization, also initialize MDD support
    //sylvan_set_sizes(1LL<<27, 1LL<<27, 1LL<<20, 1LL<<22);
    sylvan_set_limits(2LL<<31, 2, 1);
    sylvan_init_package();
    sylvan_init_ldd();
   // sylvan_gc_enable();
    m_net=R;

    m_init=init;

    int i, domain;
    vector<Place>::const_iterator it_places;

    //_______________
    transitions=R.transitions;
    m_observable=R.Observable;
    m_nonObservable=R.NonObservable;
    Formula_Trans=R.Formula_Trans;
    m_transitionName=R.transitionName;
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
    delete []liste_marques;
    // place names
//    __vplaces = &R.places;


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
            //printf("It first %d \n",it->first);
            //printf("In prec %d \n",prec[it->first]);
        }
        // arcs post
        for(vector< pair<int,int> >::const_iterator it=t->post.begin(); it!=t->post.end(); it++)
        {
            adjacentPlace.insert(it->first);
            postc[it->first] = postc[it->first] + it->second;
        }

        MDD _minus=lddmc_cube(prec,m_nbPlaces);
        MDD _plus=lddmc_cube(postc,m_nbPlaces);
        m_tb_relation.push_back(TransSylvan(_minus,_plus));
    }
    delete [] prec;
    delete [] postc;

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////// Version distribuée en utilisant les LDD - MPI/////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void *DistributedSOG::doCompute()
{
    //int nb_it=0;
    //nb_failed=0;
    //nb_success=0;

    int source=0;

    Pair S;
//    pile m_st;
    char msg[LENGTH_ID];

    int v_nbmetastate=0;
    int term=0;

    bool Terminated=false;
    bool Terminating=false;





    /****************************************** initial state ********************************************/
    if (task_id==0)
    {

        LDDState *c=new LDDState;
        MDD Complete_meta_state=Accessible_epsilon(M0);

        c->m_lddstate=Complete_meta_state;

        lddmc_getsha(Complete_meta_state, msg);
         cout <<"nb task "<<n_tasks<<endl;
        int destination=(int)abs((msg[0])%n_tasks);

        if(destination==0)
        {
            m_nbmetastate++;
            //m_old_size=lddmc_nodecount(c->m_lddstate);

            Set fire=firable_obs(Complete_meta_state);
            m_st.push(Pair(couple(c,Complete_meta_state),fire));
            m_graph->setInitialState(c);
            m_graph->insert(c);
            // m_graph->m_nbMarking+=lddmc_nodecount(c->m_lddstate);


        }
        else
        {
            char *message_to_send;
            unsigned int message_size;
            convert_wholemdd_string(M0,&message_to_send,message_size);
            MPI_Send(message_to_send, message_size, MPI_CHAR, destination, TAG_STATE, MPI_COMM_WORLD);
            delete []message_to_send;
            nbsend++;
            delete c;
        }
    }

    while (Terminated==false)
    {
        // cout<<"Nouvelle itération"<<endl;
        /****************************************** debut boucle pile ********************************************/
        if (!m_st.empty())
        {


            m_NbIt++;
            term=0;
            Pair  e=m_st.top();
            m_st.pop();

            m_nbmetastate--;
            LDDState *reached_class=NULL;
           // cout<<"debut boucle pile process "<<task_id<<endl;
            while(!e.second.empty())
            {

                //    FILE *fp=fopen("test.dot","w");
                int t = *(e.second.begin());
                e.second.erase(t);
                double nbnode;
                reached_class=new LDDState;

                //  lddmc_fprintdot(fp,Complete_meta_state);
                MDD marking=get_successor(e.first.second,t);
                MDD lddmarq=Accessible_epsilon(marking);
                reached_class->m_lddstate=lddmarq;
                lddmc_getsha(lddmarq, msg);

                int destination=(int)abs((msg[0])%n_tasks);

                /**************** construction local ******/


                if(destination==task_id)
                {

                    LDDState* pos=m_graph->find(reached_class);

                    if(!pos)
                    {

                        m_graph->addArc();
                        m_graph->insert(reached_class);

                        e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(reached_class,t));
                        reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(e.first.first,t));
                        m_nbmetastate++;

                        Set fire=firable_obs(lddmarq);

                        m_st.push(Pair(couple(reached_class,lddmarq),fire));
                        nb_success++;


                    }
                    else
                    {
                        m_graph->addArc();
                        nb_failed++;
                        e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(pos,t));
                        pos->Predecessors.insert(pos->Predecessors.begin(),LDDEdge(e.first.first,t));
                        delete reached_class;

                    }
                }
                /**************** construction externe ******/
                else // send to another process
                {

                    LDDState* posV=m_graph->findSHA(msg);

                    if(!posV)
                    {


                        m_graph->addArc();

                        m_graph->insertSHA(reached_class);

                        strcpySHA(reached_class->m_SHA2,msg);

                        e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(reached_class,t));
                        reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(e.first.first,t));

                        v_nbmetastate++;

                        char *message_to_send;
                        unsigned int message_size;
                        convert_wholemdd_string(marking,&message_to_send,message_size);
                 //       auto timeC1= std::chrono::high_resolution_clock::now();
                        MDD Reduced=Canonize(marking,0);

                        char *message_to_send2;
                        unsigned int message_size2;
                        convert_wholemdd_string(Reduced,&message_to_send2,message_size2);

                        MPI_Isend(message_to_send2, message_size2, MPI_CHAR, destination, TAG_STATE, MPI_COMM_WORLD,&m_request);

                       // m_sizeMsg=m_sizeMsg+message_size;
                        //m_sizeMsgCanoniz=m_sizeMsgCanoniz+message_size2;


                        read_state_message();
                        nbsend++;

                        MPI_Wait(&m_request,&m_status);
                        delete []message_to_send2;
                        delete []message_to_send;

                    }
                    else
                    {

                        m_graph->addArc();
                        e.first.first->Successors.insert(e.first.first->Successors.begin(),LDDEdge(posV,t));
                        posV->Predecessors.insert(posV->Predecessors.begin(),LDDEdge(e.first.first,t));
                        delete reached_class;
                    }






                }


            }


        }
        /****************************************** fin boucle pile ********************************************/

        read_state_message();


        int flag_s=0, flag_r=0, flag_term=0;

        /****************************************** debut reception de nombre de msg recu ********************************************/
        if(m_st.empty())
        {

            //cout<<"Reception nombre message recu"<<endl;
            MPI_Iprobe(task_id==0 ? n_tasks-1 : task_id-1,TAG_REC,MPI_COMM_WORLD,&flag_r,&m_status); // exist a msg to receiv?

            if(flag_r!=0)
                //   if(status.MPI_TAG==tag_rec)
            {
                int kkk;
                MPI_Irecv(&kkk, 1, MPI_INT, task_id==0 ? n_tasks-1 : task_id-1, TAG_REC, MPI_COMM_WORLD, &m_request);
                if(task_id!=0)
                {
                    //printf("Process %d  nb receive %d\n", task_id, kkk);
                    kkk=kkk+nbrecv;
                    MPI_Send( &kkk, 1, MPI_INT, (task_id+1)%n_tasks, TAG_REC, MPI_COMM_WORLD );
                }
                else
                {

                    total_nb_recv=kkk;
                    //printf("Process 0  total receive %d\n", total_nb_recv);
                    MPI_Send( &nbsend, 1, MPI_INT, 1, TAG_SEND, MPI_COMM_WORLD );
                    //printf("Process 0  nb send au debut %d\n", nbsend);
                }

            }
        }
        /****************************************** debut reception de nombre de msg envoyé ********************************************/
        if(m_st.empty())
        {


            MPI_Iprobe(task_id==0 ? n_tasks-1 : task_id-1,TAG_SEND,MPI_COMM_WORLD,&flag_s,&m_status); // exist a msg to receiv?

            if(flag_s!=0)
                //if(status.MPI_TAG==tag_send)
            {
                int kkk;

                MPI_Irecv( &kkk, 1, MPI_INT,task_id==0 ? n_tasks-1 : task_id-1, TAG_SEND, MPI_COMM_WORLD, &m_request);
                //  cout<<" Terminaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaate \n"<<task_id<<endl;

                if(task_id!=0)
                {
                    //printf("Process %d nb send  %d \n", task_id, nbsend);
                    kkk=kkk+nbsend;
                    MPI_Send( &kkk, 1, MPI_INT, (task_id + 1) % n_tasks, TAG_SEND, MPI_COMM_WORLD );
                }
                else if(kkk==total_nb_recv)
                {
                    MPI_Send( &term, 1, MPI_INT, (task_id + 1) % n_tasks, TAG_TERM, MPI_COMM_WORLD );

                }
                else
                {
                    Terminating=false;
                }





            }
        }

        /****************************************** debut reception de msg de terminaison ********************************************/
        if(m_st.empty())
        {


            MPI_Iprobe(task_id==0 ? n_tasks-1 : task_id-1,TAG_TERM,MPI_COMM_WORLD,&flag_term,&m_status); // exist a msg to receiv?

            if(flag_term!=0 )
                //if(status.MPI_TAG==tag_term)
            {
                MPI_Irecv(&term, 1, MPI_INT, task_id==0 ? n_tasks-1 : task_id-1, TAG_TERM, MPI_COMM_WORLD,&m_request);
               // cout<<" Terminaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaate \n"<<task_id<<endl;
                if(task_id!=0)
                {

                    MPI_Send(&term, 1, MPI_INT, (task_id + 1) % n_tasks, TAG_TERM, MPI_COMM_WORLD);
                }
                Terminated=true;

            }

        }

        /****************************************** Master declanche la debut de terminaison*****************************/

        if(task_id==0 && Terminating==false && m_st.empty())
        {

            //cout<<" Prooooobe Termination \n"<<endl;
            Terminating=true;
            MPI_Send( &nbrecv, 1, MPI_INT, (task_id+1)%n_tasks, TAG_REC, MPI_COMM_WORLD );
            // printf("Process 0  nb receiv au debut %d\n", nbrecv);

        }

    }

}

void DistributedSOG::read_state_message()
{
    MPI_Status   status;
    int flag;
    flag=0;
    /****************************************** debut reception state ********************************************/
    MPI_Iprobe(MPI_ANY_SOURCE,TAG_STATE,MPI_COMM_WORLD,&flag,&status); // exist a msg to receiv?
    while(flag!=0)
    {

        auto timeR1= std::chrono::high_resolution_clock::now();

        int nbytes;
        MPI_Get_count(&status, MPI_CHAR, &nbytes);
        char *inmsg=new char[nbytes];

        //cout<<"Message to be received Size : "<<nbytes<<" by process  :"<<task_id<<endl;
        MPI_Recv(inmsg, nbytes, MPI_CHAR, status.MPI_SOURCE, TAG_STATE,MPI_COMM_WORLD, &status);
        //cout<<"Message  received Size : "<<nbytes<<" by process  :"<<task_id<<endl;
        nbrecv++;
        MDD M=decodage_message(inmsg);
        //auto timeR2= std::chrono::high_resolution_clock::now();
        //auto timeR= std::chrono::duration_cast<std::chrono::milliseconds>(timeR2-timeR1).count();
        //tempR=tempR+timeR; //lddmc_cube(l_marques,nbPlaces);
        delete []inmsg;
        // delete []l_marques;
        LDDState* Agregate=new LDDState;
        MDD MState=Accessible_epsilon(M);
        Agregate->m_lddstate=MState;

        if (!m_graph->find(Agregate))
        {
            m_graph->insert(Agregate);
            Set fire=firable_obs(MState);
            m_nbmetastate++;
            m_old_size=lddmc_nodecount(Agregate->m_lddstate);
            m_st.push(Pair(couple(Agregate,MState),fire));


        }
        else delete Agregate;
        MPI_Iprobe(MPI_ANY_SOURCE,TAG_STATE,MPI_COMM_WORLD,&flag,&status); // exist a msg to receiv?
        //cout<<"Fin Boucle réception state "<<task_id<<endl;

    }



}



void DistributedSOG::computeDSOG(LDDGraph &g)

{



    //MPI_Comm_size(MPI_COMM_WORLD,&n_tasks);
    //MPI_Comm_rank(MPI_COMM_WORLD,&task_id);

    int rc;
    m_graph=&g;

    double d,tps;
    auto t1 = std::chrono::high_resolution_clock::now();

    MPI_Barrier(MPI_COMM_WORLD);


    doCompute();


    int sum_nbStates=0;
    int sum_nbArcs=0;
    long sum_tCanon=0;
    long sum_tS=0;
    long sum_tR=0;
    long sum_sizeMsg=0;
    long sum_sizeMsgCanoniz=0;
    //int sum_ldd=0;
    int nbstate=g.m_nbStates;
    int nbarcs=g.m_nbArcs;
//    int nbldd=g.getLDDnodes();
    MPI_Reduce(&nbstate, &sum_nbStates, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&nbarcs, &sum_nbArcs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);


 //   MPI_Reduce(&tempCanoniz, &sum_tCanon, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
//    MPI_Reduce(&tempS, &sum_tS , 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
//    MPI_Reduce(&tempR, &sum_tR , 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&m_sizeMsg, &sum_sizeMsg , 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    //MPI_Reduce(&m_sizeMsgCanoniz, &sum_sizeMsgCanoniz , 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);





    if(task_id==0)
    {

        //cout<<"NB META STATE DANS CONSTRUCTION : "<<m_nbmetastate<<endl;
        //cout<<"NB ITERATIONS CONSTRUCTION : "<<m_NbIt<<endl;
        //cout<<"Nb Iteration externes : "<<m_itext<<endl;
        // cout<<"Nb Iteration internes : "<<m_itint<<endl;*/
        auto t2 = std::chrono::high_resolution_clock::now();
        std::cout << "TIME OF CONSTRUCTION OF THE SOG "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count()
                  << " milliseconds\n";

        cout<<"NB AGGREGATES****** "<<sum_nbStates<<endl;
        cout<<"NB ARCS****** "<<sum_nbArcs<<endl;

        //cout<<"Taille de messages*** "<<sum_sizeMsg<<endl;
        //cout<<"Taille de messages Canoniz*** "<<sum_sizeMsgCanoniz<<endl;


   //     cout<<"Temps Canonization****** "<<sum_tCanon<<endl;
   //     cout<<"Temps envoi et reception ****** "<<sum_tS+sum_tR <<endl;
     //   cout<<"Temps reception****** "<<sum_tR<<endl;
    }

}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


string to_hex(unsigned char s)
{
    stringstream ss;
    ss << hex << (int) s;
    return ss.str();
}




char * DistributedSOG::concat_string(const char *s1,int longueur1,const char *s2,int longueur2,char *dest)
{
    for (int i=0; i<longueur1; i++)
    {
        dest[i]=(char)(s1[i]);
    }
    for (int i=longueur1; i<longueur1+longueur2; i++)
        dest[i]=s2[i-longueur1];
    dest[longueur1+longueur2]='\0';
    return dest;
}


void DistributedSOG::strcpySHA(char *dest,const char *source)
{
    for (int i=0; i<LENGTH_ID; i++)
    {
        dest[i]=source[i];
    }
    dest[LENGTH_ID]='\0';
}



DistributedSOG::~DistributedSOG()
{
    //dtor
}



void DistributedSOG::convert_wholemdd_string(MDD cmark,char **result,unsigned int &length)
{

    typedef pair<string,MDD> Pair_stack;
    vector<Pair_stack> local_stack;
    unsigned int indice=0;
    unsigned int count=0;
    MDD explore_mdd=cmark;
    string res;
    string chaine="";
    local_stack.push_back(Pair_stack(chaine,cmark));
    do
    {
        Pair_stack element=local_stack.back();
        chaine=element.first;
        explore_mdd=element.second;
        local_stack.pop_back();
        count++;
        assert (count<32000);
        while ( explore_mdd!= lddmc_false && explore_mdd!= lddmc_true )
        {
            mddnode_t n_val = GETNODE(explore_mdd);
            if (mddnode_getright(n_val)!=lddmc_false)
            {

                local_stack.push_back(Pair_stack(chaine,mddnode_getright(n_val)));
            }
            unsigned int val = mddnode_getvalue(n_val);
            chaine.push_back(char(val));
            explore_mdd=mddnode_getdown(n_val);
        }
        res+=chaine;
    }
    while (local_stack.size()!=0);
    *result= new char[res.size()+2];
    **result=(unsigned char)count;
    *(*result+1)=(unsigned char )(count>>8);
    for (unsigned int i=2; i<res.size()+2; i++)
        *(*result+i)=res.at(i-2);
    //*(*result+res.size()+2)='\0';
    length=res.size()+2;
    //cout<<" Count : "<<count<<"by process "<<task_id<<endl;
}

MDD DistributedSOG::decodage_message(char *chaine)
{
    MDD M=lddmc_false;
    unsigned int nb_marq=(unsigned char)chaine[1];
    nb_marq=(nb_marq<<8) | (unsigned char)chaine[0];

    unsigned int index=2;
    uint32_t list_marq[m_nbPlaces];
    for (unsigned int i=0; i<nb_marq; i++)
    {
        for (unsigned int j=0; j<m_nbPlaces; j++)
        {
            list_marq[j]=chaine[index];
            index++;
        }
        MDD N=lddmc_cube(list_marq,m_nbPlaces);
        M=lddmc_union_mono(M,N);
    }
    return M;
}


void DistributedSOG::MarquageString(char *dest,const char *source)
{
    int length=source[0];
    printf("Longueur %d\n",source[0]);
    int index=1;
    for (int nb=0; nb<length; nb++)
    {
        for (int i=0; i<m_nbPlaces; i++)
        {
            dest[index]=(unsigned char)source[index]+(unsigned char)'0';
            printf("%d",source[index]);
            index++;
        }
    }
    printf("\n");
    dest[index]='\0';
}

/******************************************************************************/



