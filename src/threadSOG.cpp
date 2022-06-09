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

    m_place_proposition = R.m_formula_place;
    m_nonObservable = R.NonObservable;

    m_transitionName = &R.transitionName;
    m_placeName = &R.m_placePosName;

    cout << "All transitions:" << endl;

    for (const auto &it2: *m_transitionName) {
        cout << it2.first << " : " << it2.second << endl;
    }

    cout << "mObservable transitions:" << endl;

    for (const auto &it: m_net->mObservable) {
        m_net->transitions[it].mObservable=true;
        cout << it << "  ";
    }
    cout << endl;
    m_nbPlaces = R.places.size();
    cout << "Nombre de places : " << m_nbPlaces << endl;
    cout << "Derniere place : " << R.places[m_nbPlaces - 1].name << endl;

    auto *liste_marques = new uint32_t[R.places.size()];
    auto it_places= R.places.begin();
    for (i = 0, it_places = R.places.begin(); it_places != R.places.end(); ++i, ++it_places) {
        liste_marques[i] = it_places->marking;
    }

    m_initialMarking = SylvanWrapper::lddmc_cube(liste_marques, R.places.size());

    auto *prec = new uint32_t[m_nbPlaces];
    auto *postc = new uint32_t[m_nbPlaces];
    // Transition relation
    for (const auto &t: R.transitions) {
        // Initialisation
        for (i = 0; i < m_nbPlaces; ++i) {
            prec[i] = 0;
            postc[i] = 0;
        }
        // Calculer les places adjacentes a la transition t
        set<int> adjacentPlace;
        for (const auto &it: t.pre) {
            adjacentPlace.insert(it.first);
            prec[it.first] = prec[it.first] + it.second;

        }
        // arcs post
        for (const auto &it: t.post) {
            adjacentPlace.insert(it.first);
            postc[it.first] = postc[it.first] + it.second;
        }
        for (const auto &it: adjacentPlace) {
            MDD Precond = lddmc_true;
            Precond = Precond & (it >= prec[it]);
        }

        MDD _minus = SylvanWrapper::lddmc_cube(prec, m_nbPlaces);
        //#ldd_refs_push(_minus);
        MDD _plus = SylvanWrapper::lddmc_cube(postc, m_nbPlaces);
        //#ldd_refs_push(_plus);
        m_tb_relation.emplace_back(TransSylvan(_minus, _plus));
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
    typedef pair<LDDState *, MDD> couple;
    typedef pair<couple, Set> Pair;
    typedef stack<Pair> pile;
    pile st;

    LDDState *reached_class;
    Set fire;
    //FILE *fp=fopen("test.dot","w");
    // size_t max_meta_state_size;
    auto *c = new LDDState;


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
     //max_meta_state_size=bdd_pathcount(Complete_meta_state);
    st.push(Pair(couple(c, Complete_meta_state), fire));

    g.setInitialState(c);
    g.insert(c);
    //LACE_ME;
    g.m_nbMarking += SylvanWrapper::lddmc_nodecount(c->m_lddstate);
    do {
        auto e = st.top();
        st.pop();
        while (!e.second.empty()) {
            auto t = *e.second.begin();
            e.second.erase(t);
            reached_class = new LDDState;
            {
                //  cout<<"Avant Accessible epsilon \n";
                Complete_meta_state = Accessible_epsilon(get_successor(e.first.second, t));
                //MDD reduced_meta=Canonize(Complete_meta_state,0);

                //#ldd_refs_push(Complete_meta_state);
                //ldd_refs_push(reduced_meta);
                //sylvan_gc_seq();

                reached_class->m_lddstate = Complete_meta_state;

                // reached_class->m_lddstate=reduced_meta;
                auto *pos = g.find(reached_class);
                //nbnode=sylvan_pathcount(reached_class->m_lddstate);
                if (!pos) {
                    reached_class->m_boucle = Set_Div(Complete_meta_state);
                    fire = firable_obs(Complete_meta_state);
                    st.push(Pair(couple(reached_class, Complete_meta_state), fire));
                    //TabMeta[nbmetastate]=reached_class->m_lddstate;
                    //old_size=bdd_anodecount(TabMeta,nbmetastate);
                    e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(reached_class, t));
                    reached_class->Predecessors.insert(reached_class->Predecessors.begin(), LDDEdge(e.first.first, t));
                    g.addArc();
                    g.insert(reached_class);
                } else {
                    //  cout<<" found"<<endl;
                    delete reached_class;
                    e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(pos, t));
                    pos->Predecessors.insert(pos->Predecessors.begin(), LDDEdge(e.first.first, t));
                    g.addArc();
                }
            }

        }
    } while (!st.empty());


    clock_gettime(CLOCK_REALTIME, &finish);
    double tps;
    tps = (finish.tv_sec - start.tv_sec);
    tps += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    std::cout << "TIME OF CONSTRUCTION OF THE SOG " << tps << " seconds\n";


}

/************ Non Canonised construction with pthread ***********************************/
void *threadSOG::doCompute() {
    int id_thread;
    id_thread = m_id_thread++;
    Set fire;
    int min_charge=0;
    if (id_thread == 0) {
        clock_gettime(CLOCK_REALTIME, &start);
        //  printf("*******************PARALLEL*******************\n");

        auto *c = new LDDState;

        //cout<<"Marquage initial is being built..."<<endl;
        // cout<<bddtable<<M0<<endl;

        MDD Complete_meta_state(Accessible_epsilon(m_initialMarking));
        //#ldd_refs_push(Complete_meta_state);
        // MDD canonised_initial=Canonize(Complete_meta_state,0);
        //ldd_refs_push(canonised_initial);
        fire = firable_obs(Complete_meta_state);

        c->m_lddstate = Complete_meta_state;

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
            m_terminaison[id_thread] = false;
            pthread_spin_lock(&m_spin_stack[id_thread]);
            Pair e = m_st[id_thread].top();
            //pthread_spin_lock(&m_accessible);
            m_st[id_thread].pop();

            pthread_spin_unlock(&m_spin_stack[id_thread]);

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
                    //pthread_mutex_lock(&m_mutex);
                    fire = firable_obs(Complete_meta_state);
                    //if (max_succ<fire.size()) max_succ=fire.size();
                    //pthread_mutex_unlock(&m_mutex);
                    min_charge = minCharge();
                    //m_min_charge=(m_min_charge+1) % m_nb_thread;
                    pthread_spin_lock(&m_spin_stack[min_charge]);
                    m_st[min_charge].push(Pair(couple(reached_class, Complete_meta_state), fire));
                    pthread_spin_unlock(&m_spin_stack[min_charge]);
                    m_charge[min_charge]++;
                } else {
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
    id_thread = m_id_thread++;
    Set fire;
    uint16_t min_charge;
    if (id_thread == 0) {
        clock_gettime(CLOCK_REALTIME, &start);
        //  printf("*******************PARALLEL*******************\n");

        auto *c = new LDDState;
        MDD Complete_meta_state(Accessible_epsilon(m_initialMarking));
        //#ldd_refs_push(Complete_meta_state);
        MDD canonised_initial = Canonize(Complete_meta_state, 0);
        //#ldd_refs_push(canonised_initial);
        fire = firable_obs(Complete_meta_state);
        c->m_lddstate = canonised_initial;
        //m_TabMeta[m_nbmetastate]=c->m_lddstate;
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
            m_terminaison[id_thread] = false;
            pthread_spin_lock(&m_spin_stack[id_thread]);
            Pair e = m_st[id_thread].top();
            //pthread_spin_lock(&m_accessible);
            m_st[id_thread].pop();

            pthread_spin_unlock(&m_spin_stack[id_thread]);

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

                    //pthread_mutex_lock(&m_mutex);
                    fire = firable_obs(Complete_meta_state);
                    //if (max_succ<fire.size()) max_succ=fire.size();
                    //pthread_mutex_unlock(&m_mutex);
                    min_charge = minCharge();
                    //m_min_charge=(m_min_charge+1) % m_nb_thread;
                    pthread_spin_lock(&m_spin_stack[min_charge]);
                    m_st[min_charge].push(Pair(couple(reached_class, Complete_meta_state), fire));
                    pthread_spin_unlock(&m_spin_stack[min_charge]);
                    m_charge[min_charge]++;
                } else {
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
        ++i;
    }
    return !res;
}

void threadSOG::computeDSOG(LDDGraph &g,uint8_t&& method){

    cout << "number of threads " << int(m_nb_thread) << endl;
    m_graph = &g;
    m_graph->setTransition(*m_transitionName);
    m_graph->setPlace(*m_placeName);
    m_id_thread = 0;

    pthread_mutex_init(&m_mutex, nullptr);
    m_gc = 0;
    for (int i = 0; i < m_nb_thread; ++i) {
        pthread_spin_init(&m_spin_stack[i], 0);
        m_charge[i] = 0;
        m_terminaison[i] = false;
    }

    for (int i = 0; i < m_nb_thread - 1; ++i) {
        m_list_thread[i] = new std::thread(threadHandler, this,method);
        if (m_list_thread[i] == nullptr) {
            cout << "error: creating threads #" << i << endl;
        }
    }

    switch (method) {
        case 0 : doCompute();
            break;
        case 1:
            doComputeCanonized();
            break;
        case 2:
            doComputePOR();
            break;
    }

    for (int i = 0; i < m_nb_thread - 1; ++i) {
        m_list_thread[i]->join();
        delete m_list_thread[i];
    }
    clock_gettime(CLOCK_REALTIME, &finish);

    double tps;

    cout << "Nb Threads : " << m_nb_thread << endl;
    tps = (finish.tv_sec - start.tv_sec);
    tps += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    std::cout << "TIME OF CONSTRUCTION OF THE SOG " << tps << " seconds\n";

}

void *threadSOG::threadHandler(void *context,const uint8_t& method) {

    switch (method) {
        case 0:
            return ((threadSOG *) context)->doCompute();
        case 1:
            return ((threadSOG *) context)->doComputeCanonized();
        case 2:
            return ((threadSOG *) context)->doComputePOR();
    }
    return ((threadSOG *) context)->doCompute();
}

int threadSOG::minCharge() {
    int pos = 0;
    int min_charge = m_charge[0];
    for (int i = 1; i < m_nb_thread; ++i) {
        if (m_charge[i] < min_charge) {
            min_charge = m_charge[i];
            pos = i;
        }
    }
    return pos;

}

void *threadSOG::doComputePOR() {
    int id_thread;
    id_thread = m_id_thread++;
    Set fireObs;
    bool _div, _dead;
    MDD Complete_meta_state;
    int min_charge = 0;
    if (id_thread == 0) {
        clock_gettime(CLOCK_REALTIME, &start);
        auto *c = new LDDState;
        Complete_meta_state=saturatePOR(m_initialMarking, fireObs, _div, _dead);
        cout<<Complete_meta_state<<endl;
        //cout<<"Size of Obs : "<<fireObs.size()<<endl;
        c->m_lddstate = Complete_meta_state;

        m_st[0].push(Pair(couple(c, Complete_meta_state), fireObs));
        m_graph->setInitialState(c);
        m_graph->insert(c);
        m_charge[0] = 1;
    }

    LDDState *reached_class;
    do {
        while (!m_st[id_thread].empty()) {
            m_terminaison[id_thread] = false;
            pthread_spin_lock(&m_spin_stack[id_thread]);
            Pair e = m_st[id_thread].top();
            m_st[id_thread].pop();
            pthread_spin_unlock(&m_spin_stack[id_thread]);

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
                MDD succ=get_successor(e.first.second, t);
                Complete_meta_state=saturatePOR(succ, fireObs, _div, _dead);

                //cout<<"Marks : "<<SylvanWrapper::getMarksCount(Complete_meta_state)<<endl;
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
                m_graph_mutex.lock();
                LDDState *pos = m_graph->find(reached_class);
                if (!pos) {
                    m_graph->addArc();
                    m_graph->insert(reached_class);
                    e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(reached_class, t));
                    reached_class->Predecessors.insert(reached_class->Predecessors.begin(), LDDEdge(e.first.first, t));
                    m_graph_mutex.unlock();
                    min_charge = minCharge();
                    pthread_spin_lock(&m_spin_stack[min_charge]);
                    m_st[min_charge].push(Pair(couple(reached_class, Complete_meta_state), fireObs));
                    pthread_spin_unlock(&m_spin_stack[min_charge]);
                    m_charge[min_charge]++;
                } else {
                    m_graph->addArc();
                    e.first.first->Successors.insert(e.first.first->Successors.begin(), LDDEdge(pos, t));
                    pos->Predecessors.insert(pos->Predecessors.begin(), LDDEdge(e.first.first, t));
                    m_graph_mutex.unlock();
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