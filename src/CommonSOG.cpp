#include <string>
#include "SylvanWrapper.h"
#include "CommonSOG.h"

CommonSOG::CommonSOG() = default;

CommonSOG::~CommonSOG() = default;

LDDGraph *CommonSOG::getGraph() {
    return m_graph;
}

MDD CommonSOG::Accessible_epsilon(const MDD& From) {
    MDD M1;
    MDD M2 = From;
    do {
        M1 = M2;
        for (auto i = m_nonObservable.begin(); !(i == m_nonObservable.end()); i++) {
            //fireTransition
            MDD succ = SylvanWrapper::lddmc_firing_mono(M2, m_tb_relation[(*i)].getMinus(),
                                                        m_tb_relation[(*i)].getPlus());
            M2 = SylvanWrapper::lddmc_union_mono(M2, succ);
        }
    } while (M1 != M2);
    return M2;
}

// Return the set of firable observable transitions from an agregate
Set CommonSOG::firable_obs(const MDD& State) {
    Set res;
    for (auto i: m_observable) {
        MDD succ = SylvanWrapper::lddmc_firing_mono(State, m_tb_relation[i].getMinus(), m_tb_relation[i].getPlus());
        if (succ != lddmc_false) {
            res.insert(i);
        }
    }
    return res;
}

MDD CommonSOG::get_successor(const MDD &From, const int &t) {
    return SylvanWrapper::lddmc_firing_mono(From, m_tb_relation[(t)].getMinus(), m_tb_relation[t].getPlus());
}

MDD CommonSOG::ImageForward(const MDD& From) {
    MDD Res = lddmc_false;
    for (auto i: m_nonObservable) {
        MDD succ = SylvanWrapper::lddmc_firing_mono(From, m_tb_relation[i].getMinus(), m_tb_relation[i].getPlus());
        Res = SylvanWrapper::lddmc_union_mono(Res, succ);
    }
    return Res;
}

/*----------------------------------------------CanonizeR()------------------------------------*/
MDD CommonSOG::Canonize(const MDD& s, unsigned int level) {
    if (level > m_nbPlaces || s == lddmc_false) {
        return lddmc_false;
    }
    if (SylvanWrapper::isSingleMDD(s)) {
        return s;
    }
    MDD s0 = lddmc_false, s1 = lddmc_false;
    bool res = false;
    do {
        if (SylvanWrapper::get_mddnbr(s, level) > 1) {
            s0 = SylvanWrapper::ldd_divide_rec(s, level);
            s1 = SylvanWrapper::ldd_minus(s, s0);
            res = true;
        } else {
            level++;
        }
    } while (level < m_nbPlaces && !res);
    if (s0 == lddmc_false && s1 == lddmc_false) {
        return lddmc_false;
    }
    // if (level==nbPlaces) return lddmc_false;
    MDD Front, Reach;
    if (s0 != lddmc_false && s1 != lddmc_false) {
        Front = s1;
        Reach = s1;
        do {
            Front = SylvanWrapper::ldd_minus(ImageForward(Front), Reach);
            Reach = SylvanWrapper::lddmc_union_mono(Reach, Front);
            s0 = SylvanWrapper::ldd_minus(s0, Front);
        } while ((Front != lddmc_false) && (s0 != lddmc_false));
    }
    if ((s0 != lddmc_false) && (s1 != lddmc_false)) {
        Front = s0;
        Reach = s0;
        do {
            //  cout<<"deuxieme boucle interne \n";
            Front = SylvanWrapper::ldd_minus(ImageForward(Front), Reach);
            Reach = SylvanWrapper::lddmc_union_mono(Reach, Front);
            s1 = SylvanWrapper::ldd_minus(s1, Front);
        } while (Front != lddmc_false && s1 != lddmc_false);
    }

    MDD Repr;
    if (SylvanWrapper::isSingleMDD(s0)) {
        Repr = s0;
    } else {
        Repr = Canonize(s0, level);
    }

    if (SylvanWrapper::isSingleMDD(s1)) {
        Repr = SylvanWrapper::lddmc_union_mono(Repr, s1);
    } else {
        Repr = SylvanWrapper::lddmc_union_mono(Repr, Canonize(s1, level));
    }


    return Repr;


}

/**** Detect divergence in an agregate ****/
bool CommonSOG::Set_Div(const MDD &M) const {
    if (m_nonObservable.empty()) {
        return false;
    }
    MDD Reached, From{M};
    do {
        Reached = lddmc_false;
        for (auto i: m_nonObservable) {
            MDD To = SylvanWrapper::lddmc_firing_mono(From, m_tb_relation[(i)].getMinus(),
                                                      m_tb_relation[(i)].getPlus());
            Reached = SylvanWrapper::lddmc_union_mono(Reached, To);
        }

        if (Reached == From) {
            return true;
        }
        From = Reached;
    } while (Reached != lddmc_false);
    return false;
}

/**** Detetc deadlocks ****/
bool CommonSOG::Set_Bloc(const MDD &M) const {

    MDD cur {lddmc_true};
    for (const auto & i: m_tb_relation) {
        cur = cur & (i.getMinus());
    }
    return ((M & cur) != lddmc_false);
    //BLOCAGE
}


LDDGraph *CommonSOG::m_graph;


/*MDD CommonSOG::fireTransition(MDD cmark, MDD minus, MDD plus) {
    return SylvanWrapper::lddmc_firing_mono(cmark,minus,plus);
}*/

//string_view CommonSOG::getTransition(int pos) {
// cout<<"yes it is : "<<m_transitions.at(pos).name<<endl;
// cout<<"Ok "<<m_graph->getTransition(pos)<<endl;
//    return string_view {m_transitions.at(pos).name};
//}

string_view CommonSOG::getPlace(int pos) {

    return string_view{m_graph->getPlace(pos)};
}

void CommonSOG::initializeLDD() {
    SylvanWrapper::sylvan_set_limits(16LL << 30, 10, 0);
    SylvanWrapper::sylvan_init_package();
    SylvanWrapper::sylvan_init_ldd();
    SylvanWrapper::init_gc_seq();
    SylvanWrapper::lddmc_refs_init();
    SylvanWrapper::displayMDDTableInfo();
}

void CommonSOG::loadNetFromFile() {
    int i;
    vector<Place>::const_iterator it_places;
    m_nbPlaces = m_net->places.size();
    m_transitions = m_net->transitions;
    m_observable = m_net->Observable;
    m_place_proposition = m_net->m_formula_place;
    m_nonObservable = m_net->NonObservable;

    m_transitionName = &m_net->transitionName;
    m_placeName = &m_net->m_placePosName;


    cout << "Nombre de places : " << m_nbPlaces << endl;
    cout << "Derniere place : " << m_net->places[m_nbPlaces - 1].name << endl;

    auto *liste_marques = new uint32_t[m_net->places.size()];
    for (i = 0, it_places = m_net->places.begin(); it_places != m_net->places.end(); ++i, ++it_places) {
        liste_marques[i] = it_places->marking;
    }

    m_initialMarking = SylvanWrapper::lddmc_cube(liste_marques, m_net->places.size());
    SylvanWrapper::lddmc_refs_push(m_initialMarking);
    delete[]liste_marques;

    auto *prec = new uint32_t[m_nbPlaces];
    auto *postc = new uint32_t[m_nbPlaces];
    // Transition relation
    for (const auto & t :  m_net->transitions)  {
        // Initialisation
        for (i = 0; i < m_nbPlaces; i++) {
            prec[i] = 0;
            postc[i] = 0;
        }
        // Calculer les places adjacentes a la transition t
        set<int> adjacentPlace;
        for (const auto & it : t.pre) {
            adjacentPlace.insert(it.first);
            prec[it.first] = prec[it.first] + it.second;
        }
        // arcs post
        for (const auto & it : t.post) {
            adjacentPlace.insert(it.first);
            postc[it.first] = postc[it.first] + it.second;

        }
        for (const auto &it : adjacentPlace) {
            MDD Precond = lddmc_true;
            Precond = Precond & ((it) >= prec[it]);
        }

        MDD _minus = SylvanWrapper::lddmc_cube(prec, m_nbPlaces);
        SylvanWrapper::lddmc_refs_push(_minus);
        MDD _plus = SylvanWrapper::lddmc_cube(postc, m_nbPlaces);
        SylvanWrapper::lddmc_refs_push(_plus);
        m_tb_relation.emplace_back(TransSylvan(_minus, _plus));
    }
    delete[] prec;
    delete[] postc;
}

void CommonSOG::AddConflict(const MDD &S, const int &transition, Set &ample) {
    auto haveCommonPre = [&](vector<pair<int, int>> &preT1, vector<pair<int, int>> &preT2) -> bool {
        for (auto &elt1: preT1) {
            for (auto &elt2: preT2) {
                if (std::get<0>(elt1) == std::get<0>(elt2)) return true;
            }
        }
        return false;
    };
    if (SylvanWrapper::isFirable(S,m_tb_relation[transition].getMinus())) {
        for (auto i = 0; i < m_transitions.size(); i++) {
            if (i != transition) {
                auto &preT1 = m_transitions[i].pre;
                auto &preT2 = m_transitions[transition].pre;
                if (!haveCommonPre(preT1, preT2)) ample.insert(i);
            }
        }
    } else {
        for (auto i = 0; i < m_transitions.size(); i++) {
            if (i != transition) {
                auto &postT1 = m_transitions[i].post;
                auto &preT2 = m_transitions[transition].pre;
                if (!haveCommonPre(postT1, preT2)) ample.insert(i);
            }
        }
    }
}

Set CommonSOG::computeAmple(const MDD &s) {
    Set ample;
    Set enabledT;
    // Compute enabled transitions in s
    for (auto index=0;index<m_tb_relation.size();index++) {
        if (SylvanWrapper::isFirable(s,m_tb_relation[index].getMinus()))
            enabledT.insert(index);
    }
    if (!enabledT.empty()) {
        auto it=enabledT.begin();
        ample.insert(*it);
        enabledT.erase(it);
    }

    for (auto &t : enabledT) {
        AddConflict(s,t,ample);
    }
    return ample;
}

MDD CommonSOG::saturatePOR(const MDD &s, Set& tObs,bool &div,bool &dead) {
    MDD From {s}, Reach2 {s};
    MDD Reach1;
    Set ample;
    set<MDD> myStack;
    myStack.insert(s);
    tObs.erase(tObs.begin(),tObs.end());
    MDD To=lddmc_true;
    do {
        Reach1=Reach2;
        ample=computeAmple(From);
        if (ample.empty()) dead=true;
        else {
            for (const auto & t : ample) {
                MDD succ= get_successor(s,t);
                if (m_transitions[t].mObservable) {
                    tObs.insert(t);
                }
                else {
                    //Compute div
                    if (myStack.find(succ)!=myStack.end()) {
                        div=true;
                    }
                    else {
                        To=SylvanWrapper::lddmc_union_mono(To,succ);
                        myStack.insert(succ);
                    }

                }
            }
        }
        From=To;
        Reach2=SylvanWrapper::lddmc_union_mono(Reach2,To);
    } while (Reach1!=Reach2);
    return Reach1;
}