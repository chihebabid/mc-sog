#include "ModelCheckThReq.h"
#include <unistd.h>


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
void ModelCheckThReq::ComputeTh_Succ()
{
    int rc;
    m_id_thread=0;
    //cout<<"Enter : "<<__func__ <<endl;
    pthread_mutex_init(&m_mutex, NULL);
    pthread_mutex_init(&m_graph_mutex,NULL);
    pthread_barrier_init(&m_barrier_threads, NULL, m_nb_thread+1);
    pthread_barrier_init(&m_barrier_builder, NULL, m_nb_thread+1);
    for (int i=0; i<m_nb_thread; i++)
    {
        if ((rc = pthread_create(&m_list_thread[i], NULL,threadHandler,this)))
        {
            cout<<"error: pthread_create, rc: "<<rc<<endl;
        }
    }
}

/*
 * Build initial aggregate
 */
LDDState * ModelCheckThReq::getInitialMetaState() {
    //cout<<"Enter : "<<__func__ <<endl;
    if (m_graph->getInitialAggregate()!= nullptr) return m_graph->getInitialAggregate();
    ComputeTh_Succ();
    LDDState *c = new LDDState;
    MDD initial_meta_state = Accessible_epsilon(m_initialMarking);
    SylvanWrapper::lddmc_refs_push(initial_meta_state);
    c->m_lddstate = initial_meta_state;
    m_graph->setInitialState(c);
    m_graph->insert(c);
    Set fire = firable_obs(initial_meta_state);
    int j = 0;
    for (auto it = fire.begin(); it != fire.end(); it++, j++)
        m_st[j % m_nb_thread].push(couple_th(c, *it));
    pthread_barrier_wait(&m_barrier_threads);
    pthread_barrier_wait(&m_barrier_builder);
    c->setVisited();
    return c;
}

/*
 * Build successors of @aggregate using PThreads
 */
void ModelCheckThReq::buildSucc(LDDState *agregate) {
    if (!agregate->isVisited())
    {
        //SylvanWrapper::displayMDDTableInfo();
        agregate->setVisited();
        Set fire=firable_obs(agregate->getLDDValue());
        int j=0;
        for(auto it=fire.begin(); it!=fire.end(); it++,j++)
            m_st[j%m_nb_thread].push(couple_th(agregate,*it));
        pthread_barrier_wait(&m_barrier_threads);
        pthread_barrier_wait(&m_barrier_builder);
    }
}


void * ModelCheckThReq::threadHandler(void *context)
{
    return ((ModelCheckThReq*)context)->Compute_successors();
}



void * ModelCheckThReq::Compute_successors()
{
    int id_thread;
    id_thread=m_id_thread++;
    LDDState* reached_class=nullptr;
   // cout<<"Before do : "<<__func__ <<endl;
    do
    {
         pthread_barrier_wait(&m_barrier_threads);
        while (!m_st[id_thread].empty())
        {
            couple_th elt=m_st[id_thread].top();
            m_st[id_thread].pop();
            LDDState *aggregate=elt.first;
            MDD meta_state=aggregate->getLDDValue();
            int transition=elt.second;
            MDD complete_state=Accessible_epsilon(get_successor(meta_state,transition));
            reached_class=new LDDState;
            reached_class->m_lddstate=complete_state;
            reached_class->setDiv(Set_Div(complete_state));
            reached_class->setDeadLock(Set_Bloc(complete_state));
            pthread_mutex_lock(&m_graph_mutex);
            LDDState* pos=m_graph->find(reached_class);
            if(!pos)
            {
                m_graph->insert(reached_class);
                aggregate->Successors.insert(aggregate->Successors.begin(),LDDEdge(reached_class,transition));
                reached_class->Predecessors.insert(reached_class->Predecessors.begin(),LDDEdge(aggregate,transition));
                pthread_mutex_unlock(&m_graph_mutex);
                m_graph->addArc();
            }
            else
            {   aggregate->Successors.insert(aggregate->Successors.begin(),LDDEdge(pos,transition));
                pos->Predecessors.insert(pos->Predecessors.begin(),LDDEdge(aggregate,transition));
                pthread_mutex_unlock(&m_graph_mutex);
                m_graph->addArc();
                delete reached_class;
            }

        }
        //cout<<"Before barrier : "<<__func__ <<endl;
        pthread_barrier_wait(&m_barrier_builder);
        //m_finish=m_finish;
        //cout<<"After barrier : "<<__func__ <<endl;
    }
    while (!m_finish);
    //cout<<"Quit : "<<__func__ <<" ; "<<id_thread<<endl;
}


ModelCheckThReq::~ModelCheckThReq() {
    //cout<<"1 : "<<__func__ <<endl;
    pthread_barrier_wait(&m_barrier_threads);
    m_finish=true;
    pthread_barrier_wait(&m_barrier_builder);
    /*for (int i = 0; i < m_nb_thread; i++)
    {
        pthread_join(m_list_thread[i], NULL);
    }*/
    //cout<<"3 : "<<__func__ <<endl;
}

