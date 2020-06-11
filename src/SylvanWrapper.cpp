//
// Created by chiheb on 10/06/2020.
//


#include "sylvan.h"
#include "LDDState.h"
#include <sylvan_int.h>
#include "sylvan_seq.h"
//#include <sylvan_sog.h>


#include <functional>
#include <iostream>
#include <fstream>

#include "SylvanWrapper.h"
#define GETNODE(mdd) ((mddnode_t)llmsset_index_to_ptr(nodes, mdd))
using namespace sylvan;
using namespace std;


uint64_t SylvanWrapper::m_Size=0;
uint64_t SylvanWrapper::m_agg=0;
/**************************************
 * Computes number of markings in an MDD
 * @param cmark : an MDD
 */

uint64_t SylvanWrapper::getMarksCount ( MDD cmark )
{
    //typedef pair<string,MDD> Pair_stack;
    vector<MDD> local_stack;
    uint64_t compteur=0;
    MDD explore_mdd=cmark;
    local_stack.push_back ( cmark  );
    do {
        explore_mdd=local_stack.back();
        local_stack.pop_back();
        compteur++;
        while ( explore_mdd!= lddmc_false  && explore_mdd!=lddmc_true ) {
            mddnode_t n_val = GETNODE ( explore_mdd );
            if ( mddnode_getright ( n_val ) !=lddmc_false ) {
                local_stack.push_back ( mddnode_getright ( n_val )  );
            }
            unsigned int val = mddnode_getvalue ( n_val );
            explore_mdd=mddnode_getdown ( n_val );
        }

    } while ( local_stack.size() !=0 );

    return compteur;
}
