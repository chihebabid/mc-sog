#include <string>
#include "SylvanWrapper.h"
#include "CommonSOG.h"

CommonSOG::CommonSOG() =default;
CommonSOG::~CommonSOG() =default;

LDDGraph *CommonSOG::getGraph()  {
    return m_graph;
}

MDD CommonSOG::Accessible_epsilon(MDD From) {
    MDD M1;
    MDD M2 = From;
    do {
        M1 = M2;
        for (auto i = m_nonObservable.begin(); !(i == m_nonObservable.end()); i++) {
            //fireTransition
            MDD succ = SylvanWrapper::lddmc_firing_mono(M2, m_tb_relation[(*i)].getMinus(), m_tb_relation[(*i)].getPlus());
            M2 = SylvanWrapper::lddmc_union_mono(M2, succ);
        }
    } while (M1 != M2);
    return M2;
}

// Return the set of firable observable transitions from an agregate
Set CommonSOG::firable_obs(MDD State) {
    Set res;
    for (auto i : m_observable) {
        MDD succ = SylvanWrapper::lddmc_firing_mono(State, m_tb_relation[i].getMinus(), m_tb_relation[i].getPlus());
        if (succ != lddmc_false) {
            res.insert(i);
        }
    }
    return res;
}

MDD CommonSOG::get_successor(MDD From, int t) {
    return SylvanWrapper::lddmc_firing_mono(From, m_tb_relation[(t)].getMinus(), m_tb_relation[(t)].getPlus());
}


MDD CommonSOG::ImageForward(MDD From) {
    MDD Res = lddmc_false;
    for (auto i : m_nonObservable) {
        MDD succ = SylvanWrapper::lddmc_firing_mono(From, m_tb_relation[i].getMinus(), m_tb_relation[i].getPlus());
        Res = SylvanWrapper::lddmc_union_mono(Res, succ);
    }
    return Res;
}


/*----------------------------------------------CanonizeR()------------------------------------*/
MDD CommonSOG::Canonize(MDD s, unsigned int level) {
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
            // cout<<"premiere boucle interne \n";
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
bool CommonSOG::Set_Div(MDD &M) const {

    if (m_nonObservable.empty()) {
        return false;
    }
    Set::const_iterator i;
    MDD Reached, From;

    From = M;
    do {
        Reached = lddmc_false;
        for (i = m_nonObservable.begin(); !(i == m_nonObservable.end()); i++) {
            MDD To = SylvanWrapper::lddmc_firing_mono(From, m_tb_relation[(*i)].getMinus(), m_tb_relation[(*i)].getPlus());
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
bool CommonSOG::Set_Bloc(MDD &M) const {

    MDD cur = lddmc_true;
    for (auto i : m_tb_relation) {

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

    return string_view {m_graph->getPlace(pos)};
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