#include "CommonSOG.h"
#include "sylvan.h"
#include "sylvan_seq.h"
#include <sylvan_sog.h>
#include <sylvan_int.h>
#include <stack>

#define GETNODE(mdd) ((mddnode_t)llmsset_index_to_ptr(nodes, mdd))
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

            M2 = lddmc_union_mono(M2, succ);

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
        Res = lddmc_union_mono(Res, succ);
    }
    return Res;
}


/*----------------------------------------------CanonizeR()------------------------------------*/
MDD CommonSOG::Canonize(MDD s, unsigned int level) {
    if (level > m_nbPlaces || s == lddmc_false) {
        return lddmc_false;
    }
    if (isSingleMDD(s)) {
        return s;
    }
    MDD s0 = lddmc_false, s1 = lddmc_false;

    bool res = false;
    do {
        if (get_mddnbr(s, level) > 1) {
            s0 = ldd_divide_rec(s, level);
            s1 = ldd_minus(s, s0);
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
            Front = ldd_minus(ImageForward(Front), Reach);
            Reach = lddmc_union_mono(Reach, Front);
            s0 = ldd_minus(s0, Front);
        } while ((Front != lddmc_false) && (s0 != lddmc_false));
    }
    if ((s0 != lddmc_false) && (s1 != lddmc_false)) {
        Front = s0;
        Reach = s0;
        do {
            //  cout<<"deuxieme boucle interne \n";
            Front = ldd_minus(ImageForward(Front), Reach);
            Reach = lddmc_union_mono(Reach, Front);
            s1 = ldd_minus(s1, Front);
        } while (Front != lddmc_false && s1 != lddmc_false);
    }


    MDD Repr = lddmc_false;

    if (isSingleMDD(s0)) {
        Repr = s0;
    } else {

        Repr = Canonize(s0, level);

    }

    if (isSingleMDD(s1)) {
        Repr = lddmc_union_mono(Repr, s1);
    } else {
        Repr = lddmc_union_mono(Repr, Canonize(s1, level));
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

        for (i = m_nonObservable.begin(); !(i == m_nonObservable.end()); i++) {

            MDD To = fireTransition(From, m_tb_relation[(*i)].getMinus(), m_tb_relation[(*i)].getPlus());
            Reached = lddmc_union_mono(Reached, To);
            //Reached=To;
        }

        if (Reached == From) {
            return true;
        }
        From = Reached;
        Reached = lddmc_false;

    } while (Reached != lddmc_false && Reached != lddmc_true);

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

string CommonSOG::getTransition(int pos) {
    return m_graph->getTransition(pos);
}

string CommonSOG::getPlace(int pos) {
    return m_graph->getPlace(pos);
}

LDDGraph *CommonSOG::m_graph;
struct elt_t {
    MDD cmark;
    MDD minus;
    MDD plus;
    uint32_t level;
};

MDD CommonSOG::fireTransition(MDD cmark, MDD minus, MDD plus) {
    return lddmc_firing_mono(cmark,minus,plus);
    // for an empty set of source states, or an empty transition relation, return the empty set

    MDD result;
    mddnode_t n_cmark, n_plus, n_minus;
    uint32_t value, value_minus, value_plus;
    std::stack<elt_t> lstack;
    elt_t elt;
    elt.cmark = cmark;
    elt.plus = plus;
    elt.minus = minus;
    elt.level = 0;
    lstack.push(elt);
    result = lddmc_false;
    uint32_t *currentMarking = new uint32_t[m_nbPlaces];
    elt_t relt;
    bool goDown = false;
    //MDD temp;
    while (!lstack.empty() || goDown) {
        if (!goDown) {
            elt = lstack.top();
            lstack.pop();
            n_cmark = GETNODE(elt.cmark);
            n_minus = GETNODE(elt.minus);
            n_plus = GETNODE(elt.plus);
            value = mddnode_getvalue(n_cmark);
            value_minus = mddnode_getvalue(n_minus);
            value_plus = mddnode_getvalue(n_plus);
        } else  goDown = false;

        //cout << "Level : " << elt.level << endl;
        //cout << "Current : " << value << " Minus : " << value_minus << " Plus : " << value_plus << endl;
        if (value >= value_minus) {
            *(currentMarking + elt.level) = value - value_minus + value_plus;
            if (elt.level == m_nbPlaces - 1) {
                MDD b = ldd_cube(currentMarking, m_nbPlaces);
                result = lddmc_union_mono(result, b);
            } else { // Goto next level and push right
                if (mddnode_getright(n_cmark) != lddmc_false) { // Look for another node in the same level
                    relt = elt;
                    relt.cmark = mddnode_getright(n_cmark);
                    lstack.push(relt);
                }
                //}  while (temp>lddmc_true);
                if (elt.level < m_nbPlaces - 1) { // Go down
                    elt.level++;
                    elt.cmark = mddnode_getdown(n_cmark);
                    n_cmark = GETNODE(elt.cmark);
                    elt.minus = mddnode_getdown(n_minus);
                    n_minus = GETNODE(elt.minus);
                    elt.plus = mddnode_getdown(n_plus);
                    n_plus = GETNODE(elt.plus);
                    goDown = true;
                    value = mddnode_getvalue(n_cmark);
                    value_minus = mddnode_getvalue(n_minus);
                    value_plus = mddnode_getvalue(n_plus);
                }
            }
        } else {
             // Look for another node in the same level
            elt.cmark = mddnode_getright(n_cmark);
             while (elt.cmark>lddmc_true && !goDown) {
                 n_cmark = GETNODE(elt.cmark);
                 value = mddnode_getvalue(n_cmark);
                 if (value >= value_minus) goDown = true; else elt.cmark = mddnode_getright(n_cmark);
             }

            }
        }
    delete[]currentMarking;

    return result;
}