#ifndef MODELCHECKBASEMT_H
#define MODELCHECKBASEMT_H
#include "CommonSOG.h"
class ModelCheckBaseMT : public CommonSOG {
public:
        ModelCheckBaseMT(const NewNet &R, int BOUND,int nbThread);
        virtual LDDState * getInitialMetaState()=0;
        virtual void buildSucc(LDDState *agregate)=0;
        void loadNet();
        virtual ~ModelCheckBaseMT();
protected:
    int m_nb_thread;
private:
    virtual void preConfigure()=0;

};
#endif
