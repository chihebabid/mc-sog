#include "ModelCheckBaseMT.h"

#include "sylvan.h"
#include "sylvan_seq.h"
#include <sylvan_sog.h>
#include <sylvan_int.h>

using namespace sylvan;
ModelCheckBaseMT::ModelCheckBaseMT(const NewNet &R,int nbThread)
{
    m_nb_thread=nbThread;
    m_net=&R;
    m_nbPlaces=m_net->places.size();
}
void ModelCheckBaseMT::loadNet()
{
	m_graph=new LDDGraph(this);
	preConfigure();
    m_graph->setTransition(m_transitionName);
    m_graph->setPlace(m_placeName);
}

ModelCheckBaseMT::~ModelCheckBaseMT()
{
   
}

void ModelCheckBaseMT::buildSucc ( LDDState *agregate )
{
    if ( !agregate->isVisited() ) {
        agregate->setVisited();        
        std::unique_lock<std::mutex> lk ( m_mutexBuild );
        m_condBuild.wait(lk,[&agregate]{return agregate->isCompletedSucc();});
        lk.unlock();        
    }
}

LDDState* ModelCheckBaseMT::getInitialMetaState()
{
    while ( !m_finish_initial);
    LDDState *initial_metastate = m_graph->getInitialState();
    buildSucc(initial_metastate);    
    return initial_metastate;
}




