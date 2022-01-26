//#include "test_assert.h"
//#include "sylvan_common.h"

#define GARBAGE_ENABLED
//#define DEBUG_GC
#include "threadSOG.h"




/*
void write_to_dot(const char *ch,MDD m)
{
    FILE *fp=fopen(ch,"w");
    lddmc_fprintdot(fp,m);
    fclose(fp);
}*/

/*************************************************/




/*************************************************/

threadSOG::threadSOG(const NewNet &R, int nbThread, bool uselace, bool init) {
    m_nb_thread = nbThread;
    uselace = uselace;
    SylvanWrapper::sylvan_set_limits(16LL << 30, 8, 0);

    //sylvan_init_package();
    SylvanWrapper::sylvan_init_package();
    SylvanWrapper::sylvan_init_ldd();
    SylvanWrapper::init_gc_seq();
    SylvanWrapper::displayMDDTableInfo();
    /*sylvan_gc_hook_pregc(TASK(gc_start));
    sylvan_gc_hook_postgc(TASK(gc_end));
    sylvan_gc_enable();*/
    m_net = &R;

    m_init = init;
    int i;
    vector<Place>::const_iterator it_places;





    //_______________
    m_transitions = R.transitions;
    m_observable = R.Observable;
    m_place_proposition = R.m_formula_place;
    m_nonObservable = R.NonObservable;

    m_transitionName = &R.transitionName;
    m_placeName = &R.m_placePosName;

    cout << "All transitions:" << endl;
    map<string, uint16_t>::iterator it2 = m_transitionName->begin();
    for (; it2 != m_transitionName->end(); it2++) {
        cout << (*it2).first << " : " << (*it2).second << endl;
    }


    cout << "Observable transitions:" << endl;

    for (Set::iterator it = m_observable.begin(); it != m_observable.end(); it++) { cout << *it << "  "; }
    cout << endl;
    InterfaceTrans = R.InterfaceTrans;
    m_nbPlaces = R.places.size();
    cout << "Nombre de places : " << m_nbPlaces << endl;
    cout << "Derniere place : " << R.places[m_nbPlaces - 1].name << endl;

    uint32_t *liste_marques = new uint32_t[R.places.size()];
    for (i = 0, it_places = R.places.begin(); it_places != R.places.end(); i++, it_places++) {
        liste_marques[i] = it_places->marking;
    }

    m_initialMarking = SylvanWrapper::lddmc_cube(liste_marques, R.places.size());

    auto *prec = new uint32_t[m_nbPlaces];
    auto *postc = new uint32_t[m_nbPlaces];
    // Transition relation
    for (vector<Transition>::const_iterator t = R.transitions.begin();
         t != R.transitions.end(); t++) {
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
        for (const auto & it : t->post) {
            adjacentPlace.insert(it.first);
            postc[it.first] = postc[it.first] + it.second;
        }
        for (set<int>::const_iterator it = adjacentPlace.begin(); it != adjacentPlace.end(); it++) {
            MDD Precond = lddmc_true;
            Precond = Precond & ((*it) >= prec[*it]);
        }

        MDD _minus = SylvanWrapper::lddmc_cube(prec, m_nbPlaces);
        //#ldd_refs_push(_minus);
        MDD _plus = SylvanWrapper::lddmc_cube(postc, m_nbPlaces);
        //#ldd_refs_push(_plus);
        m_tb_relation.push_back(TransSylvan(_minus, _plus));
    }

    // sylvan_gc_seq();


    delete[] prec;
    delete[] postc;


}


//**************************************************** Version s�quentielle en utilisant les LDD********************************************************************
void threadSOG::computeSeqSOG(LDDGraph &g) {
    // m_graph=&g;
    clock_gettime(CLOCK_REALTIME, &start);

    //-------------------------------------------------------------------
    //-------------------------------------------------------------------
    int nb_failed = 0;
    // cout<<"COMPUTE CANONICAL DETERMINISTIC GRAPH_________________________ \n";
    //cout<<"nb MDD var : "<<bdd_varnum()<<endl;
    // double d,tps;
    //d=(double)clock() / (double)CLOCKS_PER_SEC;

    double old_size;

    m_nbmetastate = 0;
    m_MaxIntBdd = 0;
    typedef pair<LDDState *, MDD> couple;
    typedef pair<couple, Set> Pair;
    typedef stack<Pair> pile;
    pile st;
    m_NbIt = 0;
    m_itext = m_itint = 0;
    LDDState *reached_class;
    Set fire;
    //FILE *fp=fopen("test.dot","w");
    // size_t max_meta_state_size;
    LDDState *c = new LDDState;


    // cout<<"Marquage initial :\n";
    //cout<<bddtable<<M0<<endl;
    MDD Complete_meta_state = Accessible_epsilon(m_initialMarking);
    // write_to_dot("detectDiv.dot",Complete_meta_state);
    //cout<<" div "<<Set_Div(Complete_meta_state)<<endl;
    //lddmc_fprintdot(fp,Complete_meta_state);
    //fclose(fp);

    //cout<<"Apres accessible epsilon \n";
    fire = firable_obs(Complete_meta_state);

    c->m_lddstate = Complete_meta_state;
    //TabMeta[m_nbmetastate]=c->m_lddstate;
    m_nbmetastate++;

    //max_meta_state_size=bdd_pathcount(Complete_meta_state);
    st.push(Pair(couple(c, Complete_meta_state), fire));

    g.setInitialState(c);
    g.insert(c);
    //LACE_ME;
    g.m_nbMarking += SylvanWrapper::lddmc_nodecount(c->m_lddstate);
    do {
        m_NbIt++;
        Pair e = st.top();
        st.pop();
        m_nbmetastate--;
        while (!e.second.empty()) {
            int t = *e.second.begin();
            e.second.erase(t);
            reached_class = new LDDState;
            {
                //  cout<<"Avant Accessible epsilon \n";
                MDD Complete_meta_state = Accessible_epsilon(get_successor(e.first.second, t));
                //MDD reduced_meta=Canonize(Complete_meta_state,0);

                //#ldd_refs_push(Complete_meta_state);
                //ldd_refs_push(reduced_meta);
                //sylvan_gc_seq();

                reached_class->m_lddstate = Complete_meta_state;

                // reached_class->m_lddstate=reduced_meta;
                LDDState *pos = g.find(reached_class);
                //nbnode=sylvan_pathcount(reached_class->m_lddstate);
                if (!pos) {
                    //  cout<<"not found"<<endl;
                    reached_class->m_boucle = Set_Div(Complete_meta_state);

                    fire = firable_obs(Complete_meta_state);
                    st.push(Pair(couple(reached_class, Complete_meta_state), fire));
                    //TabMeta[nbmetastate]=reached_class->m_lddstate;
                    m_nbmetastate++;
                    //old_size=bdd_anodecount(TabMeta,nbmetastate);
                    e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(reached_class, t));
                    reached_class->Predecessors.insert(reached_class->Predecessors.begin(), LDDEdge(e.first.first, t));
                    g.addArc();
                    g.insert(reached_class);
                } else {
                    //  cout<<" found"<<endl;
                    nb_failed++;
                    delete reached_class;
                    e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(pos, t));
                    pos->Predecessors.insert(pos->Predecessors.begin(), LDDEdge(e.first.first, t));
                    g.addArc();
                }
            }

        }
    } while (!st.empty());


    clock_gettime(CLOCK_REALTIME, &finish);
    //tps=(double)clock() / (double)CLOCKS_PER_SEC-d;
    // cout<<"TIME OF CONSTRUCTION : "<<tps<<" second"<<endl;
    // cout<<" MAXIMAL INTERMEDIARY MDD SIZE \n"<<m_MaxIntBdd<<endl;
    //cout<<"OLD SIZE : "<<old_size<<endl;
    //cout<<"NB SHARED NODES : "<<bdd_anodecount(TabMeta,nbmetastate)<<endl;
    cout << "NB META STATE DANS CONSTRUCTION : " << m_nbmetastate << endl;
    // cout<<"NB ITERATIONS CONSTRUCTION : "<<m_NbIt<<endl;
    //  cout<<"Nb Iteration externes : "<<m_itext<<endl;
    // cout<<"Nb Iteration internes : "<<m_itint<<endl;
    // cout<<"Nb failed :"<<nb_failed<<endl;
    double tps;
    tps = (finish.tv_sec - start.tv_sec);
    tps += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    std::cout << "TIME OF CONSTRUCTION OF THE SOG " << tps << " seconds\n";


}

/************ Non Canonised construction with pthread ***********************************/
void *threadSOG::doCompute() {
    int id_thread;
    int nb_it = 0, nb_failed = 0;
    id_thread = m_id_thread++;
    Set fire;
    if (id_thread == 0) {
        clock_gettime(CLOCK_REALTIME, &start);
        //  printf("*******************PARALLEL*******************\n");
        m_min_charge = 0;
        m_nbmetastate = 0;
        m_MaxIntBdd = 0;
        m_NbIt = 0;
        m_itext = m_itint = 0;


        LDDState *c = new LDDState;

        //cout<<"Marquage initial is being built..."<<endl;
        // cout<<bddtable<<M0<<endl;

        MDD Complete_meta_state(Accessible_epsilon(m_initialMarking));
        //#ldd_refs_push(Complete_meta_state);
        // MDD canonised_initial=Canonize(Complete_meta_state,0);
        //ldd_refs_push(canonised_initial);
        fire = firable_obs(Complete_meta_state);

        c->m_lddstate = Complete_meta_state;
        //m_TabMeta[m_nbmetastate]=c->m_lddstate;
        m_nbmetastate++;
        m_old_size = SylvanWrapper::lddmc_nodecount(c->m_lddstate);
        //max_meta_state_size=bdd_pathcount(Complete_meta_state);

        m_st[0].push(Pair(couple(c, Complete_meta_state), fire));
        m_graph->setInitialState(c);
        m_graph->insert(c);
        //m_graph->nbMarking+=bdd_pathcount(c->m_lddstate);
        m_charge[0] = 1;

    }

    LDDState *reached_class;

    // size_t max_meta_state_size;
    // int min_charge;
    do {

        while (!m_st[id_thread].empty()) {

            nb_it++;
            m_terminaison[id_thread] = false;
            pthread_spin_lock(&m_spin_stack[id_thread]);
            Pair e = m_st[id_thread].top();
            //pthread_spin_lock(&m_accessible);
            m_st[id_thread].pop();

            pthread_spin_unlock(&m_spin_stack[id_thread]);
            m_nbmetastate--;

            m_charge[id_thread]--;
            while (!e.second.empty()) {
                int t = *e.second.begin();
                e.second.erase(t);
                reached_class = new LDDState();
                if (id_thread) {
                    if (++m_gc == 1) {
                        m_gc_mutex.lock();
                    }
                }


                MDD Complete_meta_state = Accessible_epsilon(get_successor(e.first.second, t));

                //#ldd_refs_push(Complete_meta_state);

                //MDD reduced_meta=Canonize(Complete_meta_state,0);
                //ldd_refs_push(reduced_meta);

                if (id_thread == 0) {
                    m_gc_mutex.lock();
                    //     #ifdef DEBUG_GC
                    //   displayMDDTableInfo();
                    //   #endif // DEBUG_GC
                    //# sylvan_gc_seq();
                    //   #ifdef DEBUG_GC
                    //   displayMDDTableInfo();
                    //   #endif // DEBUG_GC
                    m_gc_mutex.unlock();
                }
                reached_class->m_lddstate = Complete_meta_state;
                //reached_class->m_lddstate=reduced_meta;
                //nbnode=bdd_pathcount(reached_class->m_lddstate);

                //pthread_spin_lock(&m_accessible);
                m_graph_mutex.lock();

                LDDState *pos = m_graph->find(reached_class);

                if (!pos) {
                    //  cout<<"not found"<<endl;
                    //reached_class->blocage=Set_Bloc(Complete_meta_state);
                    //reached_class->boucle=Set_Div(Complete_meta_state);

                    m_graph->addArc();
                    m_graph->insert(reached_class);
                    m_graph_mutex.unlock();

                    e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(reached_class, t));
                    reached_class->Predecessors.insert(reached_class->Predecessors.begin(), LDDEdge(e.first.first, t));
                    m_nbmetastate++;
                    //pthread_mutex_lock(&m_mutex);
                    fire = firable_obs(Complete_meta_state);
                    //if (max_succ<fire.size()) max_succ=fire.size();
                    //pthread_mutex_unlock(&m_mutex);
                    m_min_charge = minCharge();
                    //m_min_charge=(m_min_charge+1) % m_nb_thread;
                    pthread_spin_lock(&m_spin_stack[m_min_charge]);
                    m_st[m_min_charge].push(Pair(couple(reached_class, Complete_meta_state), fire));
                    pthread_spin_unlock(&m_spin_stack[m_min_charge]);
                    m_charge[m_min_charge]++;
                } else {
                    nb_failed++;
                    m_graph->addArc();
                    m_graph_mutex.unlock();

                    e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(pos, t));
                    pos->Predecessors.insert(pos->Predecessors.begin(), LDDEdge(e.first.first, t));
                    delete reached_class;

                }
                if (id_thread) {


                    if (--m_gc == 0) m_gc_mutex.unlock();

                }
            }
        }
        m_terminaison[id_thread] = true;
    } while (isNotTerminated());
    //cout<<"Thread :"<<id_thread<<"  has performed "<<nb_it<<" it�rations avec "<<nb_failed<<" �checs"<<endl;
    //cout<<"Max succ :"<<max_succ<<endl;
}

/************ Canonised construction based on Pthread library ***********************************/
void *threadSOG::doComputeCanonized() {
    int id_thread;
    int nb_it = 0, nb_failed = 0;
    id_thread = m_id_thread++;
    Set fire;
    if (id_thread == 0) {
        clock_gettime(CLOCK_REALTIME, &start);
        //  printf("*******************PARALLEL*******************\n");
        m_min_charge = 0;
        m_nbmetastate = 0;
        m_MaxIntBdd = 0;
        m_NbIt = 0;
        m_itext = m_itint = 0;


        LDDState *c = new LDDState;

        //cout<<"Marquage initial is being built..."<<endl;
        // cout<<bddtable<<M0<<endl;

        MDD Complete_meta_state(Accessible_epsilon(m_initialMarking));
        //#ldd_refs_push(Complete_meta_state);
        MDD canonised_initial = Canonize(Complete_meta_state, 0);
        //#ldd_refs_push(canonised_initial);
        fire = firable_obs(Complete_meta_state);


        c->m_lddstate = canonised_initial;
        //m_TabMeta[m_nbmetastate]=c->m_lddstate;
        m_nbmetastate++;
        m_old_size = SylvanWrapper::lddmc_nodecount(c->m_lddstate);
        //max_meta_state_size=bdd_pathcount(Complete_meta_state);

        m_st[0].push(Pair(couple(c, Complete_meta_state), fire));
        m_graph->setInitialState(c);
        m_graph->insert(c);
        //m_graph->nbMarking+=bdd_pathcount(c->m_lddstate);
        m_charge[0] = 1;

    }

    LDDState *reached_class;


    do {

        while (!m_st[id_thread].empty()) {

            nb_it++;
            m_terminaison[id_thread] = false;
            pthread_spin_lock(&m_spin_stack[id_thread]);
            Pair e = m_st[id_thread].top();
            //pthread_spin_lock(&m_accessible);
            m_st[id_thread].pop();

            pthread_spin_unlock(&m_spin_stack[id_thread]);
            m_nbmetastate--;

            m_charge[id_thread]--;
            while (!e.second.empty()) {
                int t = *e.second.begin();
                e.second.erase(t);
                reached_class = new LDDState();
                if (id_thread) {


                    if (++m_gc == 1) {
                        m_gc_mutex.lock();
                    }

                }


                MDD Complete_meta_state = Accessible_epsilon(get_successor(e.first.second, t));

                //#ldd_refs_push(Complete_meta_state);

                MDD reduced_meta = Canonize(Complete_meta_state, 0);
                //#ldd_refs_push(reduced_meta);

                if (id_thread == 0) {
                    m_gc_mutex.lock();
                    //     #ifdef DEBUG_GC
                    //   displayMDDTableInfo();
                    //   #endif // DEBUG_GC
                    //#  sylvan_gc_seq();
                    //   #ifdef DEBUG_GC
                    //   displayMDDTableInfo();
                    //   #endif // DEBUG_GC
                    m_gc_mutex.unlock();
                }
                reached_class->m_lddstate = reduced_meta;
                //reached_class->m_lddstate=reduced_meta;
                //nbnode=bdd_pathcount(reached_class->m_lddstate);

                //pthread_spin_lock(&m_accessible);
                m_graph_mutex.lock();

                LDDState *pos = m_graph->find(reached_class);

                if (!pos) {
                    //  cout<<"not found"<<endl;
                    //reached_class->blocage=Set_Bloc(Complete_meta_state);
                    //reached_class->boucle=Set_Div(Complete_meta_state);

                    m_graph->addArc();
                    m_graph->insert(reached_class);
                    m_graph_mutex.unlock();

                    e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(reached_class, t));
                    reached_class->Predecessors.insert(reached_class->Predecessors.begin(), LDDEdge(e.first.first, t));
                    m_nbmetastate++;
                    //pthread_mutex_lock(&m_mutex);
                    fire = firable_obs(Complete_meta_state);
                    //if (max_succ<fire.size()) max_succ=fire.size();
                    //pthread_mutex_unlock(&m_mutex);
                    m_min_charge = minCharge();
                    //m_min_charge=(m_min_charge+1) % m_nb_thread;
                    pthread_spin_lock(&m_spin_stack[m_min_charge]);
                    m_st[m_min_charge].push(Pair(couple(reached_class, Complete_meta_state), fire));
                    pthread_spin_unlock(&m_spin_stack[m_min_charge]);
                    m_charge[m_min_charge]++;
                } else {
                    nb_failed++;
                    m_graph->addArc();
                    m_graph_mutex.unlock();

                    e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(pos, t));
                    pos->Predecessors.insert(pos->Predecessors.begin(), LDDEdge(e.first.first, t));
                    delete reached_class;

                }
                if (id_thread) {

                    if (--m_gc == 0) m_gc_mutex.unlock();

                }
            }
        }
        m_terminaison[id_thread] = true;


    } while (isNotTerminated());
    //cout<<"Thread :"<<id_thread<<"  has performed "<<nb_it<<" it�rations avec "<<nb_failed<<" �checs"<<endl;
    //cout<<"Max succ :"<<max_succ<<endl;
}


bool threadSOG::isNotTerminated() {
    bool res = true;
    int i = 0;
    while (i < m_nb_thread && res) {
        res = m_terminaison[i];
        i++;
    }
    return !res;
}

void threadSOG::computeDSOG(LDDGraph &g, bool canonised) {

    cout << "number of threads " << int(m_nb_thread) << endl;
    int rc;
    m_graph = &g;
    m_graph->setTransition(*m_transitionName);
    m_graph->setPlace(*m_placeName);
    m_id_thread = 0;

    pthread_mutex_init(&m_mutex, NULL);
    m_gc = 0;
    for (int i = 0; i < m_nb_thread; i++) {
        pthread_spin_init(&m_spin_stack[i], NULL);
        m_charge[i] = 0;
        m_terminaison[i] = false;
    }

    for (int i = 0; i < m_nb_thread - 1; i++) {
        if ((rc = pthread_create(&m_list_thread[i], NULL, canonised ? threadHandlerCanonized : threadHandler, this))) {
            cout << "error: pthread_create, rc: " << rc << endl;
        }
    }

    if (canonised) doComputeCanonized();
    else doCompute();
    for (int i = 0; i < m_nb_thread - 1; i++) {
        pthread_join(m_list_thread[i], NULL);
    }
    clock_gettime(CLOCK_REALTIME, &finish);

    double tps;

    cout << "Nb Threads : " << m_nb_thread << endl;
    tps = (finish.tv_sec - start.tv_sec);
    tps += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    std::cout << "TIME OF CONSTRUCTION OF THE SOG " << tps << " seconds\n";

}

void *threadSOG::threadHandler(void *context) {
   return ((threadSOG *) context)->doCompute();
   // return ((threadSOG *) context)->doComputePOR();
}

void *threadSOG::threadHandlerCanonized(void *context) {
    return ((threadSOG *) context)->doComputeCanonized();
    //return ((threadSOG *) context)->doComputePOR();
}

int threadSOG::minCharge() {
    int pos = 0;
    int min_charge = m_charge[0];
    for (int i = 1; i < m_nb_thread; i++) {
        if (m_charge[i] < min_charge) {
            min_charge = m_charge[i];
            pos = i;
        }
    }
    return pos;

}





//}


threadSOG::~threadSOG() {
    //dtor
}


void *threadSOG::doComputePOR() {
    int id_thread;
    int nb_it = 0, nb_failed = 0;
    id_thread = m_id_thread++;


    Set fireObs;
    bool _div, _dead;
    if (id_thread == 0) {
        clock_gettime(CLOCK_REALTIME, &start);
        //  printf("*******************PARALLEL*******************\n");
        m_min_charge = 0;
        m_nbmetastate = 0;
        m_MaxIntBdd = 0;
        m_NbIt = 0;
        m_itext = m_itint = 0;


        LDDState *c = new LDDState;
        //cout<<"Marquage initial is being built..."<<endl;
        // cout<<bddtable<<M0<<endl;
        MDD Complete_meta_state{saturatePOR(m_initialMarking, fireObs, _div, _dead)};
        c->m_lddstate = Complete_meta_state;
        //m_TabMeta[m_nbmetastate]=c->m_lddstate;
        m_nbmetastate++;
        m_old_size = SylvanWrapper::lddmc_nodecount(c->m_lddstate);
        //max_meta_state_size=bdd_pathcount(Complete_meta_state);

        m_st[0].push(Pair(couple(c, Complete_meta_state), fireObs));
        m_graph->setInitialState(c);
        m_graph->insert(c);
        //m_graph->nbMarking+=bdd_pathcount(c->m_lddstate);
        m_charge[0] = 1;

    }

    LDDState *reached_class;
    do {
        while (!m_st[id_thread].empty()) {
            nb_it++;
            m_terminaison[id_thread] = false;
            pthread_spin_lock(&m_spin_stack[id_thread]);
            Pair e = m_st[id_thread].top();
            m_st[id_thread].pop();
            pthread_spin_unlock(&m_spin_stack[id_thread]);
            m_nbmetastate--;

            m_charge[id_thread]--;
            while (!e.second.empty()) {
                int t = *e.second.begin();
                e.second.erase(t);
                reached_class = new LDDState();
                if (id_thread) {
                    if (++m_gc == 1) {
                        m_gc_mutex.lock();
                    }
                }


                MDD Complete_meta_state {saturatePOR(get_successor(e.first.second, t), fireObs, _div, _dead)};
                /* if (id_thread==0)
                 {
                     m_gc_mutex.lock();
                     //     #ifdef DEBUG_GC
                     //   displayMDDTableInfo();
                     //   #endif // DEBUG_GC
                     //# sylvan_gc_seq();
                     //   #ifdef DEBUG_GC
                     //   displayMDDTableInfo();
                     //   #endif // DEBUG_GC
                     m_gc_mutex.unlock();
                 }*/
                reached_class->m_lddstate = Complete_meta_state;
                //reached_class->m_lddstate=reduced_meta;
                //nbnode=bdd_pathcount(reached_class->m_lddstate);

                //pthread_spin_lock(&m_accessible);
                m_graph_mutex.lock();
                LDDState *pos = m_graph->find(reached_class);
                if (!pos) {
                    m_graph->addArc();
                    m_graph->insert(reached_class);
                    m_graph_mutex.unlock();

                    e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(reached_class, t));
                    reached_class->Predecessors.insert(reached_class->Predecessors.begin(), LDDEdge(e.first.first, t));
                    m_nbmetastate++;
                    m_min_charge = minCharge();
                    //m_min_charge=(m_min_charge+1) % m_nb_thread;
                    pthread_spin_lock(&m_spin_stack[m_min_charge]);
                    m_st[m_min_charge].push(Pair(couple(reached_class, Complete_meta_state), fireObs));
                    pthread_spin_unlock(&m_spin_stack[m_min_charge]);
                    m_charge[m_min_charge]++;
                } else {
                    nb_failed++;
                    m_graph->addArc();
                    m_graph_mutex.unlock();

                    e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(pos, t));
                    pos->Predecessors.insert(pos->Predecessors.begin(), LDDEdge(e.first.first, t));
                    delete reached_class;

                }
                if (id_thread) {
                    if (--m_gc == 0) m_gc_mutex.unlock();
                }
            }
        }
        m_terminaison[id_thread] = true;
    } while (isNotTerminated());
}
