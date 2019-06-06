#ifndef MODELCHECKLACE_H
#define MODELCHECKLACE_H
#include "CommonSOG.h"
class ModelCheckLace : public CommonSOG {
public:
        ModelCheckLace(const NewNet &R, int BOUND,int nbThread);
        LDDState * buildInitialMetaState();
        void buildSucc(LDDState *agregate);
private:
    int m_nb_thread;
    MDD m_initalMarking;
};
#endif
