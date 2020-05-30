#include "LDDGraph.h"
#include <sylvan.h>
#include <string.h>
#include <map>

LDDGraph::~LDDGraph() {
    //dtor
}


void LDDGraph::setInitialState(LDDState *c) {
    m_currentstate = m_initialstate = c;
    //

}

LDDState *LDDGraph::getInitialState() const {
    return m_GONodes.at(0);
}

/*----------------------find()----------------*/
LDDState *LDDGraph::find(LDDState *c) {
    std::lock_guard<std::mutex>  lock(m_mutex);
    for (MetaLDDNodes::const_iterator i = m_GONodes.begin(); !(i == m_GONodes.end()); i++)
        if (c->m_lddstate == (*i)->m_lddstate)
            return *i;
    return NULL;
}

LDDState *LDDGraph::insertFind(LDDState *c) {
    LDDState *res = nullptr;
    std::lock_guard<std::mutex> lock(m_mutex);
    for (MetaLDDNodes::const_iterator i = m_GONodes.begin(); !(i == m_GONodes.end()) && res == nullptr; i++) {
        if (c->m_lddstate == (*i)->m_lddstate)
            res = *i;
    }
    if (res == nullptr) m_GONodes.push_back(c);
    return res;
}

/*----------------------find()----------------*/
bool LDDGraph::cmpSHA(const unsigned char *s1, const unsigned char *s2) {
    bool res = true;
    for (int i = 0; i < 16 && res; i++) {
        res = (s1[i] == s2[i]);
    }
    return !res;

}

LDDState *LDDGraph::findSHA(unsigned char *c) {
    std::lock_guard<std::mutex> lock(m_mutex_sha);
    for (MetaLDDNodes::const_iterator i = m_GONodes.begin(); !(i == m_GONodes.end()); i++)
        if ((*i)->isVirtual() == true)
            if ((cmpSHA(c, (unsigned char *) (*i)->m_SHA2) == 0))
                return *i;
    return NULL;
}

/***   Try to find an aggregate by its md5 code, else it is inserted***/
LDDState *LDDGraph::insertFindSha(unsigned char *c, LDDState *agg) {
    LDDState *res = nullptr;
    std::lock_guard<std::mutex> lock(m_mutex_sha);
    for (auto i = m_GONodes.begin(); !(i == m_GONodes.end()) && !res; i++) {
        if ((*i)->isVirtual() == true)
            if ((cmpSHA(c, (unsigned char *) (*i)->m_SHA2) == 0))
                res = *i;
    }
    if (res == nullptr) {
        agg->setVirtual();
        memcpy(agg->m_SHA2, c, 16);
        this->m_GONodes.push_back(agg);
    }
    return res;
}

/*--------------------------------------------*/
size_t LDDGraph::findSHAPos(unsigned char *c, bool &res) {
    size_t i = 0;
    res = false;
    for (i = 0; i < m_GONodes.size(); i++)
        if ((cmpSHA(c, m_GONodes.at(i)->m_SHA2) == 0)) {
            res = true;
            return i;
        }
    // cout<<__func__<<" : Aggregate not found!!!"<<endl;
    return i;
}

/*----------------------insert() ------------*/
void LDDGraph::insert(LDDState *c) {
    std::lock_guard<std::mutex> lock(m_mutex);
    this->m_GONodes.push_back(c);
    m_nbStates++;
}


/*----------------------insert() ------------*/
void LDDGraph::insertSHA(LDDState *c) {
    c->setVirtual();
    this->m_GONodes.push_back(c);

}

/*----------------------NbBddNod()------------------------*/
int LDDGraph::NbBddNode(LDDState *S, size_t &nb) {
    /*if(S->m_visited==false)
    {
        //cout<<"insertion du meta etat numero :"<<nb<<"son id est :"<<S->class_state.id()<<endl;
        //cout<<"sa taille est :"<<bdd_nodecount(S->class_state)<<" noeuds \n";
        Tab[nb-1]=S->class_state;
        S->Visited=true;
        int bddnode=bdd_nodecount(S->class_state);
        int size_succ=0;
        for(Edges::const_iterator i=S->Successors.begin();!(i==S->Successors.end());i++)
        {
            if((*i).first->Visited==false)
            {
                nb++;
                size_succ+=NbBddNode((*i).first,nb);
            }
        }
        return size_succ+bddnode;

    }
    else*/
    cout << "Not implemented yet...." << endl;
    return 0;
}

/*----------------------Visualisation du graphe------------------------*/
void LDDGraph::printCompleteInformation() {
    long count_ldd = 0L;
    for (MetaLDDNodes::const_iterator i = m_GONodes.begin(); !(i == m_GONodes.end()); i++) {
        count_ldd += lddmc_nodecount((*i)->m_lddstate);
    }
    cout << "\n\nGRAPH SIZE : \n";
    cout << "\n\tNB LDD NODES : " << count_ldd;
    cout << "\n\tNB MARKING : " << m_nbMarking;
    cout << "\n\tNB NODES : " << m_nbStates;
    cout << "\n\tNB ARCS : " << m_nbArcs << endl;
    cout << " \n\nCOMPLETE INFORMATION ?(y/n)\n";
    char c;
    cin >> c;
    //InitVisit(initialstate,n);

    size_t n = 1;
    //cout<<"NB BDD NODE : "<<NbBdm_current_state->getContainerProcess()dNode(initialstate,n)<<endl;
    NbBddNode(m_initialstate, n);
    // cout<<"NB BDD NODE : "<<bdd_anodecount(m_Tab,(int)m_nbStates)<<endl;
    //cout<<"Shared Nodes : "<<bdd_anodecount(Tab,nbStates)<<endl;
    InitVisit(m_initialstate, 1);
    //int toto;cin>>toto;
    //bdd Union=UnionMetaState(initialstate,1);
    //cout<<"a titre indicatif taille de l'union : "<<bdd_nodecount(Union)<<endl;
    if (c == 'y' || c == 'Y') {
        size_t n = 1;
        printGraph(m_initialstate, n);
    }
}

/*----------------------InitVisit()------------------------*/
void LDDGraph::InitVisit(LDDState *S, size_t nb) {
    if (nb <= m_nbStates) {

        for (LDDEdges::const_iterator i = S->Successors.begin(); !(i == S->Successors.end()); i++) {

            if ((*i).first->isVisited() == true) {
                nb++;
                InitVisit((*i).first, nb);
            }
        }

    }
}

/*********                  printGraph    *****/

void LDDGraph::printGraph(LDDState *s, size_t &nb) {
    if (nb <= m_nbStates) {
        cout << "\nSTATE NUMBER " << nb << " sha : " << s->getSHAValue() << " LDD v :" << s->getLDDValue() << endl;

        s->setVisited();
        /*printsuccessors(s);
        getchar();
        printpredecessors(s);
        getchar();*/
        LDDEdges::const_iterator i;
        for (i = s->Successors.begin(); !(i == s->Successors.end()); i++) {
            if ((*i).first->isVisited() == false) {
                nb++;
                printGraph((*i).first, nb);
            }
        }

    }

}


/*---------void print_successors_class(Class_Of_State *)------------*/
void LDDGraph::printsuccessors(LDDState *s) {
    cout << "Not implemented yet!" << endl;
}

/*---------void printpredescessors(Class_Of_State *)------------*/
void LDDGraph::printpredecessors(LDDState *s) {
    cout << "Not implemented yet!" << endl;
}

/*** Giving a position in m_GONodes Returns an LDDState ****/
LDDState *LDDGraph::getLDDStateById(unsigned int id) {
    return m_GONodes.at(id);
}

string LDDGraph::getTransition(uint16_t pos) {

    map<string, uint16_t>::iterator it = m_transition->begin();
    while (it != m_transition->end()) {
        if (it->second == pos)
            return it->first;
        it++;
    }
    return it->first;
}

string LDDGraph::getPlace(uint16_t pos) {
    return m_places->find(pos)->second;
}

void LDDGraph::setTransition(map<string, uint16_t> &list_transitions) {
    m_transition = &list_transitions;
}

void LDDGraph::setPlace(map<uint16_t, string> &list_places) {
    m_places = &list_places;
}

//void LDDGraph::setTransition(vector<string> list)

