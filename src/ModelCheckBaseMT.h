#ifndef MODELCHECKBASEMT_H
#define MODELCHECKBASEMT_H
#include "CommonSOG.h"
#include <mutex>
#include <condition_variable>
class ModelCheckBaseMT : public CommonSOG {
public:
        ModelCheckBaseMT(const NewNet &R,int nbThread);
        virtual LDDState * getInitialMetaState();
        virtual void buildSucc(LDDState *agregate);
        void loadNet();
        virtual ~ModelCheckBaseMT();
protected:
    uint8_t m_nb_thread;
    std::condition_variable m_condBuild;
    bool m_finish_initial=false;
private:
    virtual void preConfigure()=0;    
    std::mutex m_mutexBuild;

};
#endif
