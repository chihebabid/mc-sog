
#include <sylvan.h>
#include <sylvan_int.h>
#include <sylvan_sog.h>

/*******************************************MDD node structur*********************************/
/*typedef struct __attribute__((packed)) mddnode
{
    uint64_t a, b;
} * mddnode_t; // 16 bytes*/

// RmRR RRRR RRRR VVVV | VVVV DcDD DDDD DDDD (little endian - in memory)
// VVVV RRRR RRRR RRRm | DDDD DDDD DDDc VVVV (big endian)

// Ensure our mddnode is 16 bytes
typedef char __lddmc_check_mddnode_t_is_16_bytes[ ( sizeof ( struct mddnode ) ==16 ) ? 1 : -1];

static const uint64_t CACHE_FIRING_LACE  = ( 57LL<<40 );

#define GETNODE(mdd) ((mddnode_t)llmsset_index_to_ptr(nodes, mdd))


TASK_IMPL_3 ( MDD, lddmc_firing_lace, MDD, cmark, MDD, minus, MDD,plus ) {

    if ( cmark == lddmc_true ) return lddmc_true;
    if ( minus == lddmc_false ) return lddmc_false;
    if ( plus  == lddmc_false ) return lddmc_false;

    MDD result;
    if ( cache_get3 ( CACHE_FIRING_LACE, cmark, minus, plus, &result ) ) {
        printf ( "cached\n" );
        sylvan_stats_count ( CACHE_FIRING_LACE );
        return result;
        }

    mddnode_t n_cmark = GETNODE ( cmark );
    mddnode_t n_plus = GETNODE ( plus );
    mddnode_t n_minus = GETNODE ( minus );


    //lddmc_refs_pushptr(&result);
    uint32_t count=0;
    uint32_t l_values[128];
    for ( ;; ) {
        uint32_t value;
        value = mddnode_getvalue ( n_cmark );
        uint32_t value_minus = mddnode_getvalue ( n_minus );
        uint32_t value_plus = mddnode_getvalue ( n_plus );
        if ( value>=value_minus ) {

            lddmc_refs_spawn ( SPAWN ( lddmc_firing_lace, mddnode_getdown ( n_cmark ), mddnode_getdown ( n_minus ), mddnode_getdown ( n_plus ) ) );
            l_values[count]=value-value_minus+value_plus;
            count++;

            }
        cmark = mddnode_getright ( n_cmark );
        if ( cmark == lddmc_false ) break;
        n_cmark = GETNODE ( cmark );
        }

    result= lddmc_false;
    while ( count-- ) {
        lddmc_refs_push ( result );
        MDD left=lddmc_refs_sync ( SYNC ( lddmc_firing_lace ) );
        lddmc_refs_push ( left );
        MDD result2 =
            lddmc_makenode ( l_values[count],left, lddmc_false );
        lddmc_refs_push ( result2 );
        result = lddmc_union ( result, result2 );
        lddmc_refs_pop ( 3 );
        }
    //lddmc_refs_popptr(1);
    if ( cache_put3 ( CACHE_FIRING_LACE, cmark, minus, plus,result ) ) sylvan_stats_count ( CACHE_FIRING_LACE );
    return result;
    }


MDD ldd_firing_fast ( MDD cmark, MDD minus, MDD plus ) {
    // for an empty set of source states, or an empty transition relation, return the empty set
    if ( cmark == lddmc_true ) return lddmc_true;
    if ( minus == lddmc_false || plus  == lddmc_false ) return lddmc_false;

    MDD result;
    MDD _cmark=cmark, _minus=minus, _plus=plus;


    mddnode_t n_cmark = GETNODE ( _cmark );
    mddnode_t n_plus = GETNODE ( _plus );
    mddnode_t n_minus = GETNODE ( _minus );


    result = lddmc_false;
    uint16_t depth=0;
    uint16_t new_ldd[256];

    for ( ; _cmark!=lddmc_false; ) {
        uint32_t value;
        value = mddnode_getvalue ( n_cmark );
        uint32_t value_minus = mddnode_getvalue ( n_minus );
        uint32_t value_plus = mddnode_getvalue ( n_plus );
        uint8_t res=1;
        while ( _cmark!=lddmc_false && _cmark!=lddmc_true && res ) {
            if ( value>=value_minus ) {
                //new_ldd=value-value_minus+value_plus;
                //push ( mddnode_getdown ( n_cmark ), mddnode_getdown ( n_minus ), mddnode_getdown ( n_plus ) );
                MDD result2 =
                    ldd_makenode ( value-value_minus+value_plus,lddmc_firing_mono ( mddnode_getdown ( n_cmark ), mddnode_getdown ( n_minus ), mddnode_getdown ( n_plus ) ), lddmc_false );


                if ( result2!=lddmc_false ) result = lddmc_union_mono ( result, result2 );
                }
            else res=0;
            }
        if (res) {/* Build LDD and Union*/}
        cmark = mddnode_getright ( n_cmark );
        if ( cmark == lddmc_false ) break;
        n_cmark = GETNODE ( cmark );
        }
    //cache_put3(CACHE_LDD_FIRE, cmark, minus, plus, result);
    return result;
    }



