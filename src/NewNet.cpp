#include "NewNet.h"
#include <spot/misc/version.hh>
#include <spot/twaalgos/dot.hh>
#include <spot/tl/parse.hh>
#include <spot/tl/print.hh>
#include <spot/twaalgos/translate.hh>
#include <spot/twaalgos/emptiness.hh>


#include <algorithm>
#include <ext/hash_map>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>


#define TAILLEBUFF 500
using namespace std;
/***********************************************************/
/*                      class Node                         */
/***********************************************************/

void Node::addPre(int node, int valuation)
{
    pair<int, int> x(node, valuation);
    pre.push_back(x);
}

void Node::addPost(int node, int valuation)
{
    pair<int, int> x(node, valuation);
    post.push_back(x);
}

void Node::addInhibitor(int node, int valuation)
{
    pair<int, int> x(node, valuation);
    inhibitor.push_back(x);
}

void Node::addPreAuto(int node, int valuation)
{
    pair<int, int> x(node, valuation);
    preAuto.push_back(x);
}

void Node::addPostAuto(int node, int valuation)
{
    pair<int, int> x(node, valuation);
    postAuto.push_back(x);
}

void Node::addReset(int node)
{
    reset.push_back(node);
}

/***********************************************************/
/*                      class RdPE                         */
/***********************************************************/
NewNet::NewNet(const char *f, const char *Formula_trans, const char *Int_trans)
{

    cout << "CREATION D'UN NOUVEAU SOUS-RESEAU \n";
    if (create(f))
    {
        for (vector<class Place>::iterator p = places.begin(); p != places.end();
                p++)
        {
            sort(p->pre.begin(), p->pre.end());
            sort(p->post.begin(), p->post.end());
        }
        for (vector<class Transition>::iterator p = transitions.begin();
                p != transitions.end(); p++)
        {
            sort(p->pre.begin(), p->pre.end());
            sort(p->post.begin(), p->post.end());
        }
    }
    else
    {
        places.clear();
        transitions.clear();
        placeName.clear();
        transitionName.clear();
    }
    if (strlen(Formula_trans) > 0)
    {
        cout<<"transitions de la formule non vide \n";

        Set_Formula_Trans(Formula_trans);
        if (strlen(Int_trans) > 0)
        {
            Set_Interface_Trans(Int_trans);
            // cout<<"transitions de l'interface non vide \n";
        }
        cout << "______________66666666666666666666666______________________\n";
        set_union(InterfaceTrans.begin(), InterfaceTrans.end(),
                  Formula_Trans.begin(), Formula_Trans.end(),
                  inserter(Observable, Observable.begin()));
        Set_Non_Observables();
    }
    else
        for (unsigned int i = 0; i < transitions.size(); i++)
            Observable.insert(i);
    cout << "FIN CREATION \n";
}
/***************************** Constructor for Model checker******************/
NewNet::NewNet(const char *f, const set<string> & f_trans)
{

    cout << "CREATION D'UN NOUVEAU SOUS-RESEAU \n";
    if (create(f))
    {
        for (vector<class Place>::iterator p = places.begin(); p != places.end();
                p++)
        {
            sort(p->pre.begin(), p->pre.end());
            sort(p->post.begin(), p->post.end());
        }
        for (vector<class Transition>::iterator p = transitions.begin();
                p != transitions.end(); p++)
        {
            sort(p->pre.begin(), p->pre.end());
            sort(p->post.begin(), p->post.end());
        }
    }
    else
    {
        places.clear();
        transitions.clear();
        placeName.clear();
        transitionName.clear();
    }
    if (f_trans.size() > 0)
    {
        cout<<"Transition set of formula is not empty\n";
        for (set<string>::iterator p=f_trans.begin(); p!=f_trans.end(); p++)
        {
            cout<<"Transition observable :"<<*p<<endl;
        }
        setListObservable(f_trans);
        //for(Set::iterator p=Formula_Trans.begin();p!=Formula_Trans

        cout << "______________66666666666666666666666______________________\n";
        cout<<"Formula trans size "<<Formula_Trans.size()<<endl;
        cout<<"Interface trans size "<<InterfaceTrans.size()<<endl;
        set_union(InterfaceTrans.begin(), InterfaceTrans.end(),
                  Formula_Trans.begin(), Formula_Trans.end(),
                  inserter(Observable, Observable.begin()));
        Set_Non_Observables();
    }
    else
        for (unsigned int i = 0; i < transitions.size(); i++)
            Observable.insert(i);
    cout << "FIN CREATION \n";
}

/*---------------------------------Init Set of  transitions
 * ------------------------------*/
void NewNet::setListObservable(const set<string> & list_t)
{
    int pos_trans(TRANSITIONS T, string trans);
    Observable.clear();
    for (auto it=placeName.begin();it!=placeName.end();it++) {
        pair<int,string> elt((*it).second,(*it).first);
        m_placePosName.insert(elt);
    }

    for (auto it=transitionName.begin();it!=transitionName.end();it++) {
        pair<int,string> elt((*it).second,(*it).first);
        m_transitionPosName.insert(elt);
    }


    for (set<string>::const_iterator i=list_t.begin(); i!=list_t.end(); i++)
    {
        int pos = pos_trans(transitions, *i);
        if (pos==-1)
        {
            cout<<"Error: "<<*i<<" is not a transition!"<<endl;
            // Check if the proposition corresponds to a place
            map<string, int>::iterator pi = placeName.find(*i);
            if (pi!=placeName.end())
                cout<<"Place was found!"<<endl;
            m_formula_place.insert(pi->second);
            m_lplaceAP.insert(*i);
            // Adding adjacent transitions of a place as observable transitions
            Place p=places.at(pi->second);
            for (auto  iter=p.post.begin(); iter!=p.post.end(); iter++)
            {
                Observable.insert((*iter).first);
                auto it=m_transitionPosName.find((*iter).first);
                m_ltransitionAP.insert(it->second);
            }
            for (auto  iter=p.pre.begin(); iter!=p.pre.end(); iter++)
            {
                Observable.insert((*iter).first);
                auto it=m_transitionPosName.find((*iter).first);
                m_ltransitionAP.insert(it->second);
            }

        }
        else
        {
            Formula_Trans.insert(pos);
            map<string,int>::iterator ti=transitionName.find(*i);
            Observable.insert(pos);
        }
    }

}


/*---------------------------------Set_formula_trans()------------------*/
bool NewNet::Set_Formula_Trans(const char *f)
{
    FILE *in;
    int i, nb;

    //	cout<<"ici set formula transitions \n";
    int pos_trans(TRANSITIONS, string);
    char Buff[TAILLEBUFF], *z;


    in = fopen(f, "r");
    if (in == NULL)
    {
        cout << "file " << f << " doesn't exist" << endl;
        exit(1);
    }
    int nb_formula_trans;
    fscanf(in, "%d\n", &nb_formula_trans);
    nb = fread(Buff, 1, TAILLEBUFF - 1, in);
    Buff[nb] = '\0';
    z = strtok(Buff, " \t\n");
    cout << "taille " << TAILLEBUFF << " buff " << Buff << " z: " << z << endl;
    for (i = 0; i < nb_formula_trans; i++)
    {
        cout << " z: " << z << endl;
        if (z == NULL)
        {
            cout << "error in formula trans format " << endl;
            return false;
        }
        string tmp(z);
        int pos = pos_trans(transitions, tmp);
        if (pos == -1)
        {
            cout << z << "    Error??? : observale transition " << tmp
                 << " doesn't exist \n";
            // return false;
        }
        else
            Formula_Trans.insert(pos);
        /*cout<<"insertion de :"<<transitions[pos].name<<endl;*/
        z = strtok(NULL, " \t\n");
        if (z == NULL)
        {
            nb = fread(Buff, 1, TAILLEBUFF - 1, in);
            Buff[nb] = '\0';
            z = strtok(Buff, " \t\n");
        }
    }
    fclose(in);
    return true;
}
/*---------------------------------Set_Interface_trans()------------------*/
bool NewNet::Set_Interface_Trans(const char *f)
{
    FILE *in;
    int i, nb;
    int pos_trans(TRANSITIONS, string);
    char Buff[TAILLEBUFF], *z;
    in = fopen(f, "r");
    if (in == NULL)
    {
        cout << "file " << f << " doesn't exist" << endl;
        exit(1);
    }
    int int_trans;
    fscanf(in, "%d\n", &int_trans);
    nb = fread(Buff, 1, TAILLEBUFF - 1, in);
    Buff[nb] = '\0';
    z = strtok(Buff, " \t\n");
    cout << "taille " << TAILLEBUFF << " buff " << Buff << " z: " << z << endl;
    for (i = 0; i < int_trans; i++)
    {
        cout << " z: " << z << endl;
        if (z == NULL)
        {
            cout << "error in interface format " << endl;
            return false;
        }
        string tmp(z);
        int pos = pos_trans(transitions, tmp);
        // if(Formula_Trans.find(pos)==Formula_Trans.end())
        if (pos == -1)
        {
            cout << z << "         Error??? : interface transition doesn't exist \n";
            //	return false;
        }
        else
            InterfaceTrans.insert(pos);
        z = strtok(NULL, " \t\n");
        if (z == NULL)
        {
            nb = fread(Buff, 1, TAILLEBUFF - 1, in);
            Buff[nb] = '\0';
            z = strtok(Buff, " \t\n");
        }
    }
    fclose(in);
    return true;
}
/*---------------------------------Set_Non_Observables()------------------*/
void NewNet::Set_Non_Observables()
{
    NonObservable.clear();
    for (unsigned int i = 0; i < transitions.size(); i++)
        if (Observable.find(i) == Observable.end())
        {
            NonObservable.insert(i);
        }
}
/*-----------------------pos_trans()--------------------*/
int pos_trans(TRANSITIONS T, string trans)
{
    int pos = 0;
    //	cout<<"on cherche "<<trans<<" dans :\n";
    for (TRANSITIONS::const_iterator i = T.begin(); !(i == T.end()); i++, pos++)
    {
        //  cout<<i->name<<"   ";
        if (i->name == trans)
            return pos;
    }
    // cout<<"Non trouve :\n";
    return -1;
}
/* ------------------------------ operator<< -------------------------------- */
ostream &operator<<(ostream &os, const Set &s)
{
    bool b = false;

    if (s.size())
    {
        for (Set::const_iterator i = s.begin(); !(i == s.end()); i++)
        {
            if (b)
                os << ", ";
            else
                os << "{";
            os << *i << " ";
            b = true;
        }
        os << "}";
    }
    else
        os << "empty set";
    return os;
}
/*----------------------------------------------------------------------*/
bool NewNet::addPlace(const string &place, int marking, int capacity)
{
    map<string, int>::const_iterator pi = placeName.find(place);
    if (pi == placeName.end())
    {
        placeName[place] = places.size();
        Place p(place, marking, capacity);
        places.push_back(p);
        return true;
    }
    else
        return false;
}

bool NewNet::addQueue(const string &place, int capacity)
{
    map<string, int>::const_iterator pi = placeName.find(place);
    if (pi == placeName.end())
    {
        placeName[place] = places.size();
        Place p(place, -1, capacity);
        places.push_back(p);
        return true;
    }
    else
        return false;
}

bool NewNet::addLossQueue(const string &place, int capacity)
{
    map<string, int>::const_iterator pi = placeName.find(place);
    if (pi == placeName.end())
    {
        placeName[place] = places.size();
        Place p(place, -2, capacity);
        places.push_back(p);
        return true;
    }
    else
        return false;
}

bool NewNet::addTrans(const string &trans)
{
    map<string, int>::const_iterator ti = transitionName.find(trans);
    if (ti == transitionName.end())
    {
        transitionName[trans] = transitions.size();
        Transition t(trans);
        transitions.push_back(t);
        return true;
    }
    else
        return false;
}

bool NewNet::addPre(const string &place, const string &trans, int valuation)
{
    int p, t;
    map<string, int>::const_iterator pi = placeName.find(place);
    if (pi == placeName.end() || places[pi->second].isQueue())
        return false;
    else
        p = pi->second;
    map<string, int>::const_iterator ti = transitionName.find(trans);
    if (ti == transitionName.end())
        return false;
    else
        t = ti->second;
    transitions[t].addPre(p, valuation);
    places[p].addPost(t, valuation);
    return true;
}

bool NewNet::addPost(const string &place, const string &trans, int valuation)
{
    int p, t;
    map<string, int>::const_iterator pi = placeName.find(place);
    if (pi == placeName.end() || places[pi->second].isQueue())
        return false;
    else
        p = pi->second;
    map<string, int>::const_iterator ti = transitionName.find(trans);
    if (ti == transitionName.end())
        return false;
    else
        t = ti->second;
    transitions[t].addPost(p, valuation);
    places[p].addPre(t, valuation);
    return true;
}

bool NewNet::addPreQueue(const string &place, const string &trans, int valuation)
{
    int p, t;
    map<string, int>::const_iterator pi = placeName.find(place);
    if (pi == placeName.end() || !places[pi->second].isQueue())
        return false;
    else
        p = pi->second;
    map<string, int>::const_iterator ti = transitionName.find(trans);
    if (ti == transitionName.end())
        return false;
    else
        t = ti->second;
    transitions[t].addPre(p, valuation);
    places[p].addPost(t, valuation);
    return true;
}

bool NewNet::addPostQueue(const string &place, const string &trans,
                          int valuation)
{
    int p, t;
    map<string, int>::const_iterator pi = placeName.find(place);
    if (pi == placeName.end() || !places[pi->second].isQueue())
        return false;
    else
        p = pi->second;
    map<string, int>::const_iterator ti = transitionName.find(trans);
    if (ti == transitionName.end())
        return false;
    else
        t = ti->second;
    transitions[t].addPost(p, valuation);
    places[p].addPre(t, valuation);
    return true;
}

bool NewNet::addInhibitor(const string &place, const string &trans,
                          int valuation)
{
    int p, t;
    map<string, int>::const_iterator pi = placeName.find(place);
    if (pi == placeName.end())
        return false;
    else
        p = pi->second;
    map<string, int>::const_iterator ti = transitionName.find(trans);
    if (ti == transitionName.end())
        return false;
    else
        t = ti->second;
    transitions[t].addInhibitor(p, valuation);
    places[p].addInhibitor(t, valuation);
    return true;
}

bool NewNet::addPreAuto(const string &place, const string &trans,
                        const string &valuation)
{
    int p, t, v;
    map<string, int>::const_iterator pi = placeName.find(place);
    if (pi == placeName.end() || places[pi->second].isQueue())
        return false;
    else
        p = pi->second;
    map<string, int>::const_iterator ti = transitionName.find(trans);
    if (ti == transitionName.end())
        return false;
    else
        t = ti->second;
    map<string, int>::const_iterator pv = placeName.find(valuation);
    if (pv == placeName.end() || places[pv->second].isQueue())
        return false;
    else
        v = pv->second;
    transitions[t].addPreAuto(p, v);
    places[p].addPostAuto(t, v);
    return true;
}

bool NewNet::addPostAuto(const string &place, const string &trans,
                         const string &valuation)
{
    int p, t, v;
    map<string, int>::const_iterator pi = placeName.find(place);
    if (pi == placeName.end() || places[pi->second].isQueue())
        return false;
    else
        p = pi->second;
    map<string, int>::const_iterator ti = transitionName.find(trans);
    if (ti == transitionName.end())
        return false;
    else
        t = ti->second;
    map<string, int>::const_iterator pv = placeName.find(valuation);
    if (pv == placeName.end() || places[pi->second].isQueue())
        return false;
    else
        v = pv->second;
    transitions[t].addPostAuto(p, v);
    places[p].addPreAuto(t, v);
    return true;
}

bool NewNet::addReset(const string &place, const string &trans)
{
    int p, t;
    map<string, int>::const_iterator pi = placeName.find(place);
    if (pi == placeName.end())
        return false;
    else
        p = pi->second;
    map<string, int>::const_iterator ti = transitionName.find(trans);
    if (ti == transitionName.end())
        return false;
    else
        t = ti->second;
    transitions[t].addReset(p);
    places[p].addReset(t);
    return true;
}

/* Visualisation */
ostream &operator<<(ostream &os, const NewNet &R)
{
    /* affichage nombre de places et de transitions */
    os << "***************************" << endl;
    os << "Nombre de places     :" << R.nbPlace() << endl;
    os << "Nombre de transitions:" << R.nbTransition() << endl;

    /* affichage de la liste des places */
    os << "********** places **********" << endl;
    int i = 0;
    for (vector<class Place>::const_iterator p = R.places.begin();
            p != R.places.end(); p++, i++)
    {
        if (p->isQueue())
        {
            os << "queue " << setw(4) << i << ":" << p->name << ", cp(" << p->capacity
               << ")";
            if (p->isLossQueue())
                cout << " loss";
            cout << endl;
        }
        else
            os << "place " << setw(4) << i << ":" << p->name << ":" << p->marking
               << " <..>, cp(" << p->capacity << ")" << endl;
    }
    os << "********** transitions  de la formule  **********" << endl;
    for (Set::const_iterator h = R.Formula_Trans.begin();
            !(h == R.Formula_Trans.end()); h++)
        cout << R.transitions[*h].name << endl;
    os << "********** transitions  de l'interface  **********" << endl;
    for (Set::const_iterator h = R.InterfaceTrans.begin();
            !(h == R.InterfaceTrans.end()); h++)
        cout << R.transitions[*h].name << endl;
    os << "Nombre de transitions observable:" << R.Observable.size() << endl;
    os << "********** transitions observables **********" << endl;
    for (Set::const_iterator h = R.Observable.begin(); !(h == R.Observable.end());
            h++)
        cout << R.transitions[*h].name << endl;
    os << "Nombre de transitions non observees:" << R.NonObservable.size()
       << endl;
    os << "********** transitions  non observees **********" << endl;
    for (Set::const_iterator h = R.NonObservable.begin();
            !(h == R.NonObservable.end()); h++)
        cout << R.transitions[*h].name << endl;
    i = 0;
    os << "Nombre global de transitions :" << R.nbTransition() << endl;
    os << "********** transitions  **********" << endl;
    for (TRANSITIONS::const_iterator t = R.transitions.begin();
            t != R.transitions.end(); t++, i++)
    {
        os << setw(4) << i << ":" << t->name << endl;

        // os<<"        IN  { ";
        // for (vector< pair<int,int> >::const_iterator
        // a=p->pre.begin();a!=p->pre.end();a++)
        //  if (R.places[a->first].isQueue())
        //  os<<R.places[a->first].name<<":<"<<a->second<<">;";
        //  else
        //  os<<R.places[a->first].name<<":"<<a->second<<";";
        //    for (vector< pair<int,int> >::const_iterator
        //    a=p->inhibitor.begin();a!=p->inhibitor.end();a++)
        // os<<R.places[a->first].name<<"<"<<a->second<<";";
        // for (vector< pair<int,int> >::const_iterator
        // a=p->preAuto.begin();a!=p->preAuto.end();a++)
        // os<<R.places[a->first].name<<":"<<R.places[a->second].name<<";";
        // for (vector<int>::const_iterator
        // a=p->reset.begin();a!=p->reset.end();a++) os<<R.places[*a].name<<":
        // reset;"; os<<" }"<<endl;

        // os<<"        OUT { ";
        // for (vector< pair<int,int> >::const_iterator
        // a=p->post.begin();a!=p->post.end();a++)
        //  if (R.places[a->first].isQueue())
        //  os<<R.places[a->first].name<<":<"<<a->second<<">;";
        //  else
        //  os<<R.places[a->first].name<<":"<<a->second<<";";
        // for (vector< pair<int,int> >::const_iterator
        // a=p->postAuto.begin();a!=p->postAuto.end();a++)
        // os<<R.places[a->first].name<<":"<<R.places[a->second].name<<";";
        // os<<" }"<<endl;
    }
    return (os);
}
