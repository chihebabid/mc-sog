#ifndef NEWNET_H_INCLUDED
#define NEWNET_H_INCLUDED
/* -*- C++ -*- */
#ifndef NET_H
#define NET_H

#include <cstring>
#include <ext/hash_map>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "RdPMonteur.hpp"

typedef set<int> Set;

class Node {
public:
    Node() {};

    ~Node() {};
    vector<pair<int, int>> pre, post, inhibitor, preAuto, postAuto;
    vector<int> reset;

    void addPre(int, int);

    void addPost(int, int);

    void addInhibitor(int, int);

    void addPreAuto(int, int);

    void addPostAuto(int, int);

    void addReset(int);
};

class Place : public Node {
public:
    string name;
    int marking, capacity;

    Place(const string &p, int m = 0, int c = 0)
            : name(p), marking(m), capacity(c) {};

    ~Place() {};

    bool isLossQueue() const { return marking == -2; }

    bool isQueue() const { return marking <= -1; }

    bool hasCapacity() const { return capacity != 0; }
};

class Transition : public Node {
public:
    string name;
    bool mObservable{false};

    Transition(const string &t) : name(t) {};

    ~Transition() {};
};

/*-----------------------------------------------------------------*/
struct ltstr {
    bool operator()(const char *s1, const char *s2) const {
        return std::strcmp(s1, s2) < 0;
    }
};

typedef set<const char *, ltstr> Set_mot;
typedef vector<Place> PLACES;
typedef vector<Transition> TRANSITIONS;

class NewNet : public RdPMonteur {
public:
    void setListObservable(const set<string> &list_t);

private:
    /*Initialisation des attributs*/
    bool Set_Interface_Trans(const char *file);

    bool Set_Formula_Trans(const char *file);

    void Set_Non_Observables();

public:
    /* Attributs */
    vector<class Place> places;
    vector<class Transition> transitions;
    map<string, uint16_t> m_placeName;
    map<string, uint16_t> transitionName;
    Set mObservable;
    Set NonObservable;
    Set InterfaceTrans;
    Set Formula_Trans;
    set<uint16_t> m_formula_place;

    /* Constructors */
    NewNet() = delete;

    ~NewNet() = default;

    NewNet(const char *file, const char *Obs = "", const char *Int = "");

    NewNet(const char *f, const set<string> &f_trans);

    /* Monteur */
    bool addPlace(const string &place, int marking = 0, int capacity = 0);

    bool addQueue(const string &place, int capacity = 0);

    bool addLossQueue(const string &place, int capacity = 0);

    bool addTrans(const string &transition);

    bool addPre(const string &place, const string &transition, int valuation = 1);

    bool addPost(const string &place, const string &transition,
                 int valuation = 1);

    bool addPreQueue(const string &place, const string &transition,
                     int valuation = 1);

    bool addPostQueue(const string &place, const string &transition,
                      int valuation = 1);

    bool addInhibitor(const string &place, const string &transition,
                      int valuation = 1);

    bool addPreAuto(const string &place, const string &transition,
                    const string &valuation);

    bool addPostAuto(const string &place, const string &transition,
                     const string &valuation);

    bool addReset(const string &place, const string &transition);

    /* Visualisation */
    int nbPlace() const { return places.size(); };
    int nbTransition() const { return transitions.size(); };
    set<string> &getListTransitionAP() { return m_ltransitionAP; }
    set<string> &getListPlaceAP() { return m_lplaceAP; }
    string_view getPlaceName(size_t pos) { return m_placePosName.find(pos)->second; }
    string_view getTransitionName(size_t pos) { return m_transitionPosName.find(pos)->second; }
    map<uint16_t, string> m_placePosName;
private:
    map<uint16_t, string> m_transitionPosName;
    set<string> m_ltransitionAP;
    set<string> m_lplaceAP;
};

ostream &operator<<(ostream &, const NewNet &);

#endif


#endif // NEWNET_H_INCLUDED
