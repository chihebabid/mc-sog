#ifndef MODELCHECKLACE_H
#define MODELCHECKLACE_H
#include "ModelCheckBaseMT.h"
class ModelCheckLace : public ModelCheckBaseMT {
public:
        ModelCheckLace(const NewNet &R, int BOUND,int nbThread);
        LDDState * getInitialMetaState() override;
        void buildSucc(LDDState *agregate) override;
private:
        void preConfigure();

};
#endif

