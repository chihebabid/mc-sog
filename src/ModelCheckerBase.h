#ifndef MODELCHECKERBASE_H
#define MODELCHECKERBASE_H
#include "CommonSOG.h"
typedef pair<LDDState *, int> couple_th;
typedef stack<pair<LDDState *,int>> pile_t;

class ModelCheckerBase : public CommonSOG
{
public:
    virtual ModelCheckerTh(const NewNet &R, int BOUND,int nbThread);
    virtual ~ModelCheckerTh();
    LDDState * buildInitialMetaState();

    void buildSucc(LDDState *agregate);

    void *Compute_successors();
    void ComputeTh_Succ();
private:

    MDD m_initalMarking;


};
#endif
