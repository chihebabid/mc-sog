#ifndef LDDSTATEEXTEND_H_INCLUDED
#define LDDSTATEEXTEND_H_INCLUDED

#include <set>
#include <vector>
#include <sylvan.h>
#include "LDDState.h"
using namespace std;

typedef set<int> Set;

class LDDStateExtend: public LDDState
{
    public:
       LDDStateExtend(){m_boucle=m_blocage=m_visited=false;}
       virtual ~LDDStateExtend();
       bool m_exist;

        void setLDDExValue(MDD m);
		MDD  getLDDExValue();

    protected:

    private:

};
typedef pair<LDDStateExtend*, int> LDDExEdge;
typedef vector<LDDEdge> LDDExEdgedges;

#endif // LDDSTATEEXTEND_H_INCLUDED
