#include "LDDGraph.h"
#include <cstring>
#include <map>
#include "SylvanWrapper.h"

LDDGraph::~LDDGraph()=default;


void LDDGraph::setInitialState(LDDState *c) {
   m_initialstate = c;
}


/*----------------------find()----------------*/
LDDState *LDDGraph::find(LDDState *c) {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    for (const auto & i : m_GONodes)
        if (c->m_lddstate == i->m_lddstate)
            return i;
    return nullptr;
}



LDDState *LDDGraph::insertFindByMDD(MDD md, bool &found) {
    std::lock_guard lock(m_mutex);
    {

        for (auto& i : m_GONodes) {
            if (md == i->m_lddstate) {
                found = true;
                return i;
            }
        }
    }
    LDDState *n = new LDDState;
    n->m_lddstate = md;
    found = false;
    m_GONodes.push_back(n);
    return n;
}



LDDState *LDDGraph::findSHA(unsigned char *c) {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    for (MetaLDDNodes::const_iterator i = m_GONodes.begin(); !(i == m_GONodes.end()); i++)
        if ((*i)->isVirtual() == true)
            if (memcmp((char*)c, (char*) (*i)->m_SHA2,16) == 0)
                return *i;
    return nullptr;
}

/***   Try to find an aggregate by its md5 code, else it is inserted***/
LDDState *LDDGraph::insertFindSha(unsigned char *c, LDDState *agg) {
    LDDState *res = nullptr;
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    for (auto i = m_GONodes.begin(); !(i == m_GONodes.end()) && !res; i++) {
        if ((*i)->isVirtual() == true)
            if (memcmp((char*)c, (char*) (*i)->m_SHA2,16) == 0)
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
    size_t i;
    res = false;
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    for (i = 0; i < m_GONodes.size(); i++)
        if (memcmp(c, m_GONodes.at(i)->m_SHA2,16) == 0) {
            res = true;
            return i;
        }
    return i;
}

/*----------------------insert() ------------*/
void LDDGraph::insert(LDDState *c) {
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    this->m_GONodes.push_back(c);
}


/*----------------------insert() ------------*/
void LDDGraph::insertSHA(LDDState *c) {
    c->setVirtual();
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    this->m_GONodes.push_back(c);

}


/*----------------------Visualisation du graphe------------------------*/
void LDDGraph::printCompleteInformation() {
    long count_ldd = 0L;
    for (const auto & i : m_GONodes) {
        count_ldd += SylvanWrapper::lddmc_nodecount(i->m_lddstate);
        m_nbMarking += SylvanWrapper::getMarksCount(i->m_lddstate);
    }
    cout << "\n\nGRAPH SIZE : \n";
    cout << "\n\tNB LDD NODES : " << count_ldd;
    cout << "\n\tNB MARKING : " << m_nbMarking;
    cout << "\n\tNB NODES : " << m_GONodes.size();
    cout << "\n\tNB ARCS : " << m_nbArcs << endl;
    /*cout << " \n\nCOMPLETE INFORMATION ?(y/n)\n";
    char c;
    cin >> c;*/
    //InitVisit(initialstate,n);

    /* size_t n = 1;
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
     }*/
}

/*----------------------InitVisit()------------------------*/
void LDDGraph::InitVisit(LDDState *S, size_t nb) {
    if (nb <= m_nbStates) {

        for (const auto & i : S->Successors) {

            if (i.first->isVisited() == true) {
                nb++;
                InitVisit(i.first, nb);
            }
        }

    }
}

/*********                  printGraph    *****/

void LDDGraph::printGraph(LDDState *s, size_t &nb) {
    if (nb <= m_nbStates) {
        cout << "\nSTATE NUMBER " << nb << " sha : " << s->getSHAValue() << " LDD v :" << s->getLDDValue() << endl;

        s->setVisited();
        LDDEdges::const_iterator i;
        for (const auto & i:  s->Successors) {
            if (i.first->isVisited() == false) {
                nb++;
                printGraph(i.first, nb);
            }
        }

    }

}


/*** Giving a position in m_GONodes Returns an LDDState ****/
LDDState *LDDGraph::getLDDStateById(const unsigned int& id) {
    return m_GONodes.at(id);
}

string_view LDDGraph::getTransition(uint16_t pos) {

    /*for (const auto & it : m_transition) {
        if (it.second == pos) {
            return it.first;
        }
    }
    return it.first;*/

    map<string, uint16_t>::iterator it = m_transition->begin();

    while (it != m_transition->end()) {
        if (it->second == pos) {
           return it->first;
        }
        it++;
    }
    return it->first;
}

string_view LDDGraph::getPlace(uint16_t pos) {
    return m_places->find(pos)->second;
}

void LDDGraph::setTransition(map<string, uint16_t> &list_transitions) {
    m_transition = &list_transitions;
}

void LDDGraph::setPlace(map<uint16_t, string> &list_places) {
    m_places = &list_places;
}

//void LDDGraph::setTransition(vector<string> list)

