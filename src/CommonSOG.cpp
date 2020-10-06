#include <string>
#include "SylvanWrapper.h"
#include "CommonSOG.h"

#include <stack>


const vector<class Place> *_places = NULL;

CommonSOG::CommonSOG() {
    //ctor
}

CommonSOG::~CommonSOG() {
    //dtor
}

LDDGraph *CommonSOG::getGraph() const {
    return m_graph;
}

MDD CommonSOG::Accessible_epsilon(MDD From) {
    MDD M1;
    MDD M2 = From;
    do {

        M1 = M2;
        for (Set::const_iterator i = m_nonObservable.begin(); !(i == m_nonObservable.end()); i++) {
            //fireTransition
            MDD succ = fireTransition(M2, m_tb_relation[(*i)].getMinus(), m_tb_relation[(*i)].getPlus());

            M2 = SylvanWrapper::lddmc_union_mono(M2, succ);

        }

    } while (M1 != M2);
    return M2;
}

// Return the set of firable observable transitions from an agregate
Set CommonSOG::firable_obs(MDD State) {
    Set res;
    for (Set::const_iterator i = m_observable.begin(); !(i == m_observable.end()); i++) {
        //cout<<"firable..."<<endl;
        MDD succ = fireTransition(State, m_tb_relation[(*i)].getMinus(), m_tb_relation[(*i)].getPlus());
        if (succ != lddmc_false) {
            res.insert(*i);
        }

    }
    return res;
}

MDD CommonSOG::get_successor(MDD From, int t) {
    return fireTransition(From, m_tb_relation[(t)].getMinus(), m_tb_relation[(t)].getPlus());
}


MDD CommonSOG::ImageForward(MDD From) {
    MDD Res = lddmc_false;
    for (Set::const_iterator i = m_nonObservable.begin(); !(i == m_nonObservable.end()); i++) {
        MDD succ = fireTransition(From, m_tb_relation[(*i)].getMinus(), m_tb_relation[(*i)].getPlus());
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


    MDD Repr = lddmc_false;

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

void CommonSOG::printhandler(ostream &o, int var) {
    o << (*_places)[var / 2].name;
    if (var % 2) {
        o << "_p";
    }
}


vector<TransSylvan> *CommonSOG::getTBRelation() {
    return &m_tb_relation;
}

Set *CommonSOG::getNonObservable() {
    return &m_nonObservable;
}


/*********Returns the count of places******************************************/
unsigned int CommonSOG::getPlacesCount() {
    return m_nbPlaces;
}

/**** Detect divergence in an agregate ****/
bool CommonSOG::Set_Div(MDD &M) const {

    if (m_nonObservable.empty()) {
        return false;
    }
    Set::const_iterator i;
    MDD Reached, From;
    //cout<<"Ici detect divergence \n";
    Reached = lddmc_false;
    From = M;
    do {
        Reached = lddmc_false;
        for (i = m_nonObservable.begin(); !(i == m_nonObservable.end()); i++) {
            MDD To = fireTransition(From, m_tb_relation[(*i)].getMinus(), m_tb_relation[(*i)].getPlus());
            Reached = SylvanWrapper::lddmc_union_mono(Reached, To);
        }

        if (Reached == From) {
            MDD Reached_obs=lddmc_false;
            for (i = m_observable.begin(); !(i == m_observable.end()) && (Reached_obs==lddmc_false); i++) {
                Reached_obs= fireTransition(From, m_tb_relation[(*i)].getMinus(), m_tb_relation[(*i)].getPlus());
            }
            if (Reached_obs==lddmc_false) return true;
            return false;
        }
        From = Reached;
    } while (Reached != lddmc_false);
    return false;
}

/**** Detetc deadlocks ****/
bool CommonSOG::Set_Bloc(MDD &M) const {

    MDD cur = lddmc_true;
    for (vector<TransSylvan>::const_iterator i = m_tb_relation.begin(); i != m_tb_relation.end(); i++) {

        cur = cur & (TransSylvan(*i).getMinus());

    }
    return ((M & cur) != lddmc_false);
    //BLOCAGE
}



LDDGraph *CommonSOG::m_graph;
struct elt_t {
    MDD cmark;
    MDD minus;
    MDD plus;
    uint32_t level;
};

MDD CommonSOG::fireTransition(MDD cmark, MDD minus, MDD plus) {
    return SylvanWrapper::lddmc_firing_mono(cmark,minus,plus);
}

//string_view CommonSOG::getTransition(int pos) {
    // cout<<"yes it is : "<<m_transitions.at(pos).name<<endl;
    // cout<<"Ok "<<m_graph->getTransition(pos)<<endl;
//    return string_view {m_transitions.at(pos).name};
//}

string_view CommonSOG::getPlace(int pos) {
    // cout<<"yes it is : "<<m_transitions.at(pos).name<<endl;
    // cout<<"Ok "<<m_graph->getPlace(pos)<<endl;
    return string_view {m_graph->getPlace(pos)};
}