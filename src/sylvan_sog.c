
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
typedef char __lddmc_check_mddnode_t_is_16_bytes[(sizeof(struct mddnode)==16) ? 1 : -1];

/*static inline uint32_t __attribute__((unused))
mddnode_getvalue(mddnode_t n)
{
    return *(uint32_t*)((uint8_t*)n+6);
}*/

/*static inline uint8_t __attribute__((unused))
mddnode_getmark(mddnode_t n)
{
    return n->a & 1;
}*/

/*static inline uint8_t __attribute__((unused))
mddnode_getcopy(mddnode_t n)
{
    return n->b & 0x10000 ? 1 : 0;
}*/

/*static inline uint64_t __attribute__((unused))
mddnode_getright(mddnode_t n)
{
    return (n->a & 0x0000ffffffffffff) >> 1;
}

static inline uint64_t __attribute__((unused))
mddnode_getdown(mddnode_t n)
{
    return n->b >> 17;
}*/

#define GETNODE(mdd) ((mddnode_t)llmsset_index_to_ptr(nodes, mdd))

TASK_IMPL_3(MDD, lddmc_firing_lace, MDD, cmark, MDD, minus, MDD ,plus) {

  if (cmark == lddmc_true) return lddmc_true;
    if (minus == lddmc_false) return lddmc_false;
    if (plus  == lddmc_false) return lddmc_false; // we assume that if meta is finished, then the rest is not in rel


    /* Access cache */
   MDD result;
    MDD _cmark=cmark, _minus=minus, _plus=plus;
    /*if (cache_get3(CACHE_LDD_FIRING, cmark, minus, plus, &result)) {
        sylvan_stats_count(LDD_FIRING_CACHED);
        return result;
    }*/

    mddnode_t n_cmark = GETNODE(_cmark);
    mddnode_t n_plus = GETNODE(_plus);
    mddnode_t n_minus = GETNODE(_minus);
    // meta: -1 (end; rest not in rel), 0 (not in rel), 1 (read), 2 (write), 3 (only-read), 4 (only-write)

    /* Recursive operations */

    // write, only-write
//        if (m_val == 4) {
//            // only-write, so we need to include 'for all variables'
//            // the reason is that we did not have a read phase, so we need to 'insert' a read phase here
//
//        }

    // if we're here and we are only-write, then we read the current value

    // spawn for every value to write (rel)

    result = lddmc_false;

    for (;;)
    {
        uint32_t value;
        value = mddnode_getvalue(n_cmark);
        uint32_t value_minus = mddnode_getvalue(n_minus);
        uint32_t value_plus = mddnode_getvalue(n_plus);
        if (value>=value_minus)
        {
            MDD result2 =
                lddmc_makenode(value-value_minus+value_plus,CALL(lddmc_firing_lace, mddnode_getdown(n_cmark), mddnode_getdown(n_minus), mddnode_getdown(n_plus)), lddmc_false);


            if (result2!=lddmc_false) result = lddmc_union( result, result2);
        }
        cmark = mddnode_getright(n_cmark);
        if (cmark == lddmc_false) break;
        n_cmark = GETNODE(cmark);
    }
    return result;
}
