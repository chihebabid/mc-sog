#include "ModelCheckBaseMT.h"

#include "sylvan.h"
#include "sylvan_seq.h"
#include <sylvan_sog.h>
#include <sylvan_int.h>

using namespace sylvan;
ModelCheckBaseMT::ModelCheckBaseMT(const NewNet &R, int BOUND,int nbThread)
{
    m_nb_thread=nbThread;
    m_net=R;
    m_nbPlaces=m_net.places.size();
}
void ModelCheckBaseMT::loadNet()
{
    preConfigure();    
    m_graph=new LDDGraph(this);
    m_graph->setTransition(m_transitionName);
    m_graph->setPlace(m_placeName);
}

ModelCheckBaseMT::~ModelCheckBaseMT()
{
   
}


  




