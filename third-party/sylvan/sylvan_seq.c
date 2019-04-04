/*
 * Copyright 2011-2014 Formal Methods and Tools, University of Twente
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sylvan.h>
#include <sylvan_int.h>

#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <avl.h>

#include <sys/mman.h>

#include <sha2.h>

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




/**
 * MDD node structure
 */
/*typedef struct __attribute__((packed)) mddnode
{
    uint64_t a, b;
} * mddnode_t; // 16 bytes*/

#define GETNODE(mdd) ((mddnode_t)llmsset_index_to_ptr(nodes, mdd))


/*static inline uint64_t __attribute__((unused))
mddnode_getright(mddnode_t n)
{
    return (n->a & 0x0000ffffffffffff) >> 1;
}*/

/*static inline uint64_t __attribute__((unused))
mddnode_getdown(mddnode_t n)
{
    return n->b >> 17;
}

static inline uint8_t __attribute__((unused))
mddnode_getcopy(mddnode_t n)
{
    return n->b & 0x10000 ? 1 : 0;
}*/


/*static inline uint32_t __attribute__((unused))
mddnode_getvalue(mddnode_t n)
{
    return *(uint32_t*)((uint8_t*)n+6);
}

static inline void __attribute__((unused))
mddnode_make(mddnode_t n, uint32_t value, uint64_t right, uint64_t down)
{
    n->a = right << 1;
    n->b = down << 17;
    *(uint32_t*)((uint8_t*)n+6) = value;
}
static inline void __attribute__((unused))
mddnode_makecopy(mddnode_t n, uint64_t right, uint64_t down)
{
    n->a = right << 1;
    n->b = ((down << 1) | 1) << 16;
}
*/
/**************************************************/
/*************************************************/

MDD ldd_makenode(uint32_t value, MDD ifeq, MDD ifneq)
{
    if (ifeq == lddmc_false) return ifneq;

    // check if correct (should be false, or next in value)
    assert(ifneq != lddmc_true);
    if (ifneq != lddmc_false) assert(value < mddnode_getvalue(GETNODE(ifneq)));

    struct mddnode n;
    mddnode_make(&n, value, ifneq, ifeq);

    int created;
    uint64_t index = llmsset_lookup(nodes, n.a, n.b, &created);
    if (index == 0)
    {
        /*LACE_ME;
       sylvan_gc(); // --- MC*/


        index = llmsset_lookup(nodes, n.a, n.b, &created);
        if (index == 0)
        {
            fprintf(stderr, "MDD Unique table full!\n");

            exit(1);
        }
    }

    /*
        if (created) sylvan_stats_count(LDD_NODES_CREATED);
        else sylvan_stats_count(LDD_NODES_REUSED);*/
    return (MDD)index;
}


MDD
ldd_make_copynode(MDD ifeq, MDD ifneq)
{
    struct mddnode n;
    mddnode_makecopy(&n, ifneq, ifeq);

    int created;
    uint64_t index = llmsset_lookup(nodes, n.a, n.b, &created);
    if (index == 0)
    {
        lddmc_refs_push(ifeq);
        lddmc_refs_push(ifneq);
        //  LACE_ME;
        //sylvan_gc();
        lddmc_refs_pop(1);

        index = llmsset_lookup(nodes, n.a, n.b, &created);
        if (index == 0)
        {
            fprintf(stderr, "MDD Unique table full, make_copy_node!\n");
            exit(1);
        }
    }

    /*if (created) sylvan_stats_count(LDD_NODES_CREATED);
        else sylvan_stats_count(LDD_NODES_REUSED);*/

    return (MDD)index;
}

MDD lddmc_union_mono(MDD a, MDD b)
{
    /* Terminal cases */
    if (a == b) return a;
    if (a == lddmc_false) return b;
    if (b == lddmc_false) return a;
    assert(a != lddmc_true && b != lddmc_true); // expecting same length

    /* Test gc */
    // LACE_ME;
    // sylvan_gc_test();

    //sylvan_stats_count(LDD_UNION);

    /* Improve cache behavior */
    if (a < b)
    {
        MDD tmp=b;
        b=a;
        a=tmp;
    }

    /* Access cache */
    MDD result;
    /*if (cache_get3(CACHE_MDD_UNION, a, b, 0, &result)) {
        sylvan_stats_count(LDD_UNION_CACHED);
        return result;
    }*/

    /* Get nodes */
    mddnode_t na = GETNODE(a);
    mddnode_t nb = GETNODE(b);

    const int na_copy = mddnode_getcopy(na) ? 1 : 0;
    const int nb_copy = mddnode_getcopy(nb) ? 1 : 0;
    const uint32_t na_value = mddnode_getvalue(na);
    const uint32_t nb_value = mddnode_getvalue(nb);

    /* Perform recursive calculation */
    if (na_copy && nb_copy)
    {

        MDD right = lddmc_union_mono(mddnode_getright(na), mddnode_getright(nb));
        MDD down = lddmc_union_mono(mddnode_getdown(na), mddnode_getdown(nb));
        result = ldd_make_copynode(down, right);
    }
    else if (na_copy)
    {
        MDD right = lddmc_union_mono(mddnode_getright(na), b);
        result = ldd_make_copynode(mddnode_getdown(na), right);
    }
    else if (nb_copy)
    {
        MDD right = lddmc_union_mono(a, mddnode_getright(nb));
        result = ldd_make_copynode(mddnode_getdown(nb), right);
    }
    else if (na_value < nb_value)
    {
        MDD right = lddmc_union_mono( mddnode_getright(na), b);
        result = ldd_makenode(na_value, mddnode_getdown(na), right);
    }
    else if (na_value == nb_value)
    {

        MDD right = lddmc_union_mono( mddnode_getright(na), mddnode_getright(nb));

        MDD down = lddmc_union_mono(mddnode_getdown(na), mddnode_getdown(nb));
        result = ldd_makenode(na_value, down, right);
    }
    else /* na_value > nb_value */
    {
        MDD right = lddmc_union_mono(a, mddnode_getright(nb));
        result = ldd_makenode(nb_value, mddnode_getdown(nb), right);
    }

    /* Write to cache */
    // if (cache_put3(CACHE_MDD_UNION, a, b, 0, result)) sylvan_stats_count(LDD_UNION_CACHEDPUT);

    return result;
}


MDD ldd_minus(MDD a, MDD b)
{
    /* Terminal cases */
    if (a == b) return lddmc_false;
    if (a == lddmc_false) return lddmc_false;
    if (b == lddmc_false) return a;
    assert(b != lddmc_true);
    assert(a != lddmc_true); // Universe is unknown!! // Possibly depth issue?

    /* Test gc */
    //sylvan_gc_test();

    // sylvan_stats_count(LDD_MINUS);

    MDD result;

    /* Get nodes */
    mddnode_t na = GETNODE(a);
    mddnode_t nb = GETNODE(b);
    uint32_t na_value = mddnode_getvalue(na);
    uint32_t nb_value = mddnode_getvalue(nb);

    /* Perform recursive calculation */
    if (na_value < nb_value)
    {
        MDD right = ldd_minus(mddnode_getright(na), b);
        result = ldd_makenode(na_value, mddnode_getdown(na), right);
    }
    else if (na_value == nb_value)
    {
//        lddmc_refs_spawn(SPAWN(lddmc_minus, mddnode_getright(na), mddnode_getright(nb)));
        MDD down = ldd_minus( mddnode_getdown(na), mddnode_getdown(nb));
        // lddmc_refs_push(down);
        MDD right = ldd_minus( mddnode_getright(na), mddnode_getright(nb));
        //lddmc_refs_pop(1);
        result = ldd_makenode(na_value, down, right);
    }
    else /* na_value > nb_value */
    {
        result = ldd_minus( a, mddnode_getright(nb));
    }




    return result;
}

MDD lddmc_firing_mono(MDD cmark, MDD minus, MDD plus)
{
    // for an empty set of source states, or an empty transition relation, return the empty set
    if (cmark == lddmc_true) return lddmc_true;
    if (minus == lddmc_false) return lddmc_false;
    if (plus  == lddmc_false) return lddmc_false; // we assume that if meta is finished, then the rest is not in rel


    /* Access cache */
    MDD result;
    MDD _cmark=cmark, _minus=minus, _plus=plus;
    /*if (cache_get3(CACHE_MDD_RELPROD, cmark, minus, plus, &result)) {
        sylvan_stats_count(LDD_RELPROD_CACHED);
        return result;
    }*/

    mddnode_t n_cmark = GETNODE(_cmark);
    mddnode_t n_plus = GETNODE(_plus);
    mddnode_t n_minus = GETNODE(_minus);


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
                ldd_makenode(value-value_minus+value_plus,lddmc_firing_mono( mddnode_getdown(n_cmark), mddnode_getdown(n_minus), mddnode_getdown(n_plus)), lddmc_false);


            if (result2!=lddmc_false) result = lddmc_union_mono( result, result2);
        }
        cmark = mddnode_getright(n_cmark);
        if (cmark == lddmc_false) break;
        n_cmark = GETNODE(cmark);
    }
    return result;
}


MDD lddmc_firing_help_mono(uint32_t val, MDD mark, MDD minus, MDD plus)
{
    return ldd_makenode(val,lddmc_firing_mono(mark, minus, plus), lddmc_false);
}






void convert_to_string(MDD mdd,char *chaine_mdd)
{
    unsigned int indice=0;
    MDD explore_mdd=mdd;

    mddnode_t n_val;

    while ( explore_mdd!= lddmc_false && explore_mdd!= lddmc_true )
    {
        n_val= GETNODE(explore_mdd);

        uint32_t val = mddnode_getvalue(n_val);
        chaine_mdd[indice] = (char) val;
        explore_mdd=mddnode_getdown(n_val);
        // explore_mdd=mddnode_getright(n_val);
        indice++;
    }
    chaine_mdd[indice]='\0';
}

/*void convert_to_string(MDD mdd,char *chaine_mdd)
{
    unsigned int indice=0;
    MDD explore_mdd=mdd;
     mddnode_t n;
     MDD right;
     MDD down;
    n= GETNODE(explore_mdd);
    uint32_t val;
    do
    {
       val = mddnode_getvalue(n);
       chaine_mdd[indice] = (char) val;
        indice++;
        down=mddnode_getdown(n);
        while (down!= lddmc_false&& down!=lddmc_true)
        {
            mddnode_t n_down= GETNODE(down);
            val = mddnode_getvalue(n_down);
            chaine_mdd[indice] = (char) val;
            indice++;
            // explore_mdd=mddnode_getright(n_val);
            down=mddnode_getdown(n_down);
        }
        right=mddnode_getright(n);
        n= GETNODE(right);
    }
    while(right!=lddmc_false&&right!=lddmc_true);

    chaine_mdd[indice]= " X ";
}*/






MDD
lddmc_project_node(MDD mdd)
{

    if (mdd == lddmc_false) return lddmc_false;
    if (mdd == lddmc_true) return lddmc_true;

    const mddnode_t n = GETNODE(mdd);

    if (mddnode_getdown(n))
    {

        mdd=mddnode_getdown(n);

    }
    mdd=mddnode_getright(n);

    return mdd;
}

// Renvoie le nbre de noeuds à un niveau donné
int get_mddnbr(MDD mdd,int level)
{
    mddnode_t node;
    for (int j=0; j<level; j++)
    {
        node=GETNODE(mdd);
        mdd=mddnode_getdown(node);
    }
    int i=0;
    node=GETNODE(mdd);


    while(mdd!=lddmc_false)
    {
        mddnode_t r=GETNODE(mdd);
        mdd=mddnode_getright(r);
        i++;
    }
    return i;
}

/*static inline void __attribute__((unused))
mddnode_setdown(mddnode_t n, uint64_t down)
{
    n->b = (n->b & 0x000000000001ffff) | (down << 17);
}*/

void ldd_divide(MDD mdd,const int level,MDD *mdd1,MDD *mdd2)
{
    uint32_t * list_values=malloc(sizeof(uint32_t)*(level+1));

    mddnode_t node=GETNODE(mdd);
    MDD pred=mdd;

    for (int i=0; i<=level; i++)
    {
        pred=mdd;
        uint32_t val = mddnode_getvalue(node);
        mdd=mddnode_getdown(node);
        node=GETNODE(mdd);
        list_values[i]=val;
    }


    *mdd1=lddmc_cube(list_values,level+1);



    mddnode_t node_pred=GETNODE(pred);


    MDD copy_mdd1=mddnode_getdown(node_pred);//,
 mddnode_t node_mdd1,node_mdd2;
    MDD _mdd1=*mdd1;
    node_mdd1=GETNODE(_mdd1);
    for (int j=0; j<level; j++)
    {
        _mdd1=mddnode_getdown(node_mdd1);
        node_mdd1=GETNODE(_mdd1);


    }


    //mddnode_setdown
    //ldd_make_copynode

    mddnode_setdown(node_mdd1,ldd_make_copynode(copy_mdd1,lddmc_false));

/*     FILE *fp0=fopen("copy.dot","w");
 *     lddmc_fprintdot(fp0,copy_mdd1);
 *     fclose(fp0);
 */

    if (level==0)
        *mdd2=mddnode_getright(node_pred);
    else
    {
        *mdd2=lddmc_cube(list_values,level);
        MDD _mdd2=*mdd2;
        node_mdd2=GETNODE(_mdd2);
        for (int j=0; j<level-1; j++)
        {
            _mdd2=mddnode_getdown(node_mdd2);
            node_mdd2=GETNODE(_mdd2);
        }

      mddnode_setdown(node_mdd2,mddnode_getright(node_pred));

    }
    free(list_values);
}

int isSingleMDD(MDD mdd)
{
    mddnode_t node;

    while (mdd>lddmc_true)
    {
        node=GETNODE(mdd);
        if (mddnode_getright(node)!=lddmc_false) return 0;
        mdd=mddnode_getdown(node);
    }
    return 1;
}

MDD ldd_divide_internal(MDD a,int current_level,int level) {
    /* Terminal cases */

    if (a == lddmc_false) return lddmc_false;
    if (a == lddmc_true) return lddmc_true;


    MDD result;
    mddnode_t node_a = GETNODE(a);
    const uint32_t na_value = mddnode_getvalue(node_a);


    /* Perform recursive calculation */
    if (current_level<=level)
    {

        MDD down = ldd_divide_internal( mddnode_getdown(node_a), current_level+1,level);
        MDD right=lddmc_false;
        result = ldd_makenode(na_value, down, right);
    }
    else
    {
        MDD right = ldd_divide_internal(mddnode_getright(node_a), current_level,level);
        MDD down = ldd_divide_internal( mddnode_getdown(node_a), current_level+1,level);
        result = ldd_makenode(na_value, down, right);
    }


    return result;
}



MDD ldd_divide_rec(MDD a, int level)
{
    return ldd_divide_internal(a,0,level);
}








