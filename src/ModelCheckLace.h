#ifndef MODELCHECKLACE_H
#define MODELCHECKLACE_H
#include "ModelCheckBaseMT.h"
class ModelCheckLace : public ModelCheckBaseMT {
public:
        ModelCheckLace(const NewNet &R, int BOUND,int nbThread);
        LDDState * buildInitialMetaState();
        void buildSucc(LDDState *agregate);
private:
        void preConfigure();

};
#endif
