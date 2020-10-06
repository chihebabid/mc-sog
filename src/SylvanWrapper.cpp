//
// Created by chiheb on 10/06/2020.
//


//#include "sylvan.h"
// #include "LDDState.h"

#include <functional>
#include <vector>
#include <cassert>
#include <cstring> // memset
#include <sys/mman.h> // for mmap
#include <iostream>
#include <string>
#include "SylvanCacheWrapper.h"
#include "SylvanWrapper.h"


#define DECLARE_THREAD_LOCAL(name, type) __thread type name
#define INIT_THREAD_LOCAL(name)
#define SET_THREAD_LOCAL(name, value) name = value
#define LOCALIZE_THREAD_LOCAL(name, type)

DECLARE_THREAD_LOCAL(my_region, uint64_t);
DECLARE_THREAD_LOCAL(lddmc_refs_key, lddmc_refs_internal_t);
//lddmc_refs_internal_t lddmc_refs_key;
using namespace std;

MDD lddmc_false = 0;
MDD lddmc_true = 1;


/**************************************
 * Computes number of markings in an MDD
 * @param cmark : an MDD
 */

uint64_t SylvanWrapper::getMarksCount(MDD cmark) {
    //typedef pair<string,MDD> Pair_stack;
    vector<MDD> local_stack;
    uint64_t compteur = 0;
    MDD explore_mdd;
    local_stack.push_back(cmark);
    do {
        explore_mdd = local_stack.back();
        local_stack.pop_back();
        compteur++;
        while (explore_mdd != lddmc_false && explore_mdd != lddmc_true) {
            mddnode_t n_val = GETNODE(explore_mdd);
            if (mddnode_getright(n_val) != lddmc_false) {
                local_stack.push_back(mddnode_getright(n_val));
            }
            //unsigned int val = mddnode_getvalue ( n_val );
            explore_mdd = mddnode_getdown(n_val);
        }

    } while (!local_stack.empty());

    return compteur;
}


llmsset2_t SylvanWrapper::llmsset_create(size_t initial_size, size_t max_size) {
    llmsset2_t dbs = nullptr;
    if (posix_memalign((void **) &dbs, LINE_SIZE, sizeof(struct llmsset2)) != 0) {
        fprintf(stderr, "llmsset_create: Unable to allocate memory!\n");
        exit(1);
    }

#if LLMSSET_MASK
    /* Check if initial_size and max_size are powers of 2 */
    if (__builtin_popcountll(initial_size) != 1) {
        fprintf(stderr, "llmsset_create: initial_size is not a power of 2!\n");
        exit(1);
    }

    if (__builtin_popcountll(max_size) != 1) {
        fprintf(stderr, "llmsset_create: max_size is not a power of 2!\n");
        exit(1);
    }
#endif

    if (initial_size > max_size) {
        fprintf(stderr, "llmsset_create: initial_size > max_size!\n");
        exit(1);
    }

    // minimum size is now 512 buckets (region size, but of course, n_workers * 512 is suggested as minimum)

    if (initial_size < 512) {
        fprintf(stderr, "llmsset_create: initial_size too small!\n");
        exit(1);
    }

    dbs->max_size = max_size;
    llmsset_set_size(dbs, initial_size);

    /* This implementation of "resizable hash table" allocates the max_size table in virtual memory,
       but only uses the "actual size" part in real memory */

    dbs->table = (uint64_t *) mmap(nullptr, dbs->max_size * 8, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,
                                   0);
    dbs->data = (uint8_t *) mmap(nullptr, dbs->max_size * 16, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,
                                 0);

    /* Also allocate bitmaps. Each region is 64*8 = 512 buckets.
       Overhead of bitmap1: 1 bit per 4096 bucket.
       Overhead of bitmap2: 1 bit per bucket.
       Overhead of bitmapc: 1 bit per bucket. */

    dbs->bitmap1 = (uint64_t *) mmap(0, dbs->max_size / (512 * 8), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
                                     -1, 0);
    dbs->bitmap2 = (uint64_t *) mmap(0, dbs->max_size / 8, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    dbs->bitmapc = (uint64_t *) mmap(0, dbs->max_size / 8, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (dbs->table == (uint64_t *) -1 || dbs->data == (uint8_t *) -1 || dbs->bitmap1 == (uint64_t *) -1 ||
        dbs->bitmap2 == (uint64_t *) -1 || dbs->bitmapc == (uint64_t *) -1) {
        fprintf(stderr, "llmsset_create: Unable to allocate memory: %s!\n", strerror(errno));
        exit(1);
    }

#if defined(madvise) && defined(MADV_RANDOM)
    madvise(dbs->table, dbs->max_size * 8, MADV_RANDOM);
#endif

    // forbid first two positions (index 0 and 1)
    dbs->bitmap2[0] = 0xc000000000000000LL;

    dbs->hash_cb = NULL;
    dbs->equals_cb = NULL;
    dbs->create_cb = NULL;
    dbs->destroy_cb = NULL;

    // yes, ugly. for now, we use a global thread-local value.
    // that is a problem with multiple tables.
    // so, for now, do NOT use multiple tables!!

    /*LACE_ME;*/
    INIT_THREAD_LOCAL(my_region);

    llmsset_reset_region();

    return dbs;
}


void SylvanWrapper::llmsset_reset_region() {
    LOCALIZE_THREAD_LOCAL(my_region, uint64_t);
    my_region = (uint64_t) -1; // no region
    SET_THREAD_LOCAL(my_region, my_region);
}

/**
 * Set the table size of the set.
 * Typically called during garbage collection, after clear and before rehash.
 * Returns 0 if dbs->table_size > dbs->max_size!
 */
void SylvanWrapper::llmsset_set_size(llmsset2_t dbs, size_t size) {
    /* check bounds (don't be rediculous) */
    if (size > 128 && size <= dbs->max_size) {
        dbs->table_size = size;
#if LLMSSET_MASK
        /* Warning: if size is not a power of two, you will get interesting behavior */
        dbs->mask = dbs->table_size - 1;
#endif
        /* Set threshold: number of cache lines to probe before giving up on node insertion */
        dbs->threshold = 192 - 2 * __builtin_clzll(dbs->table_size);
    }
}

void
SylvanWrapper::sylvan_set_limits(size_t memorycap, int table_ratio, int initial_ratio) {
    if (table_ratio > 10 || table_ratio < -10) {
        fprintf(stderr, "sylvan_set_limits: table_ratio unreasonable (between -10 and 10)\n");
        exit(1);
    }

    size_t max_t = 1;
    size_t max_c = 1;
    if (table_ratio > 0) {
        max_t <<= table_ratio;
    } else {
        max_c <<= -table_ratio;
    }

    size_t cur = max_t * 24 + max_c * 36;
    if (cur > memorycap) {
        fprintf(stderr, "sylvan_set_limits: memory cap incompatible with requested table ratio\n");
    }

    while (2 * cur < memorycap && max_t < 0x0000040000000000) {
        max_t *= 2;
        max_c *= 2;
        cur *= 2;
    }

    if (initial_ratio < 0) {
        fprintf(stderr, "sylvan_set_limits: initial_ratio unreasonable (may not be negative)\n");
        exit(1);
    }

    size_t min_t = max_t, min_c = max_c;
    while (initial_ratio > 0 && min_t > 0x1000 && min_c > 0x1000) {
        min_t >>= 1;
        min_c >>= 1;
        initial_ratio--;
    }

    m_table_min = min_t;
    m_table_max = max_t;
    m_cache_min = min_c;
    m_cache_max = max_c;
}

/**
 * Initializes Sylvan.
 */
void SylvanWrapper::sylvan_init_package(void) {
    if (m_table_max == 0) {
        fprintf(stderr, "sylvan_init_package error: table sizes not set (sylvan_set_sizes or sylvan_set_limits)!");
        exit(1);
    }

    /* Create tables */
    m_nodes = llmsset_create(m_table_min, m_table_max);
    SylvanCacheWrapper::cache_create(m_cache_min, m_cache_max);

    /* Initialize garbage collection */
    m_gc = 0;

    /***************** Not yet **************************************/
//#if SYLVAN_AGGRESSIVE_RESIZE
//    main_hook = TASK(sylvan_gc_aggressive_resize);
//#else
//    main_hook = TASK(sylvan_gc_normal_resize);
//#endif


    //sylvan_stats_init();
}

void SylvanWrapper::sylvan_init_ldd() {
    /*sylvan_register_quit(lddmc_quit);
    sylvan_gc_add_mark(TASK(lddmc_gc_mark_external_refs));
    sylvan_gc_add_mark(TASK(lddmc_gc_mark_protected));
    sylvan_gc_add_mark(TASK(lddmc_gc_mark_serialize));*/
    //m_lddmc_protected_created=0; // Should be initialized in the constructor
    refs_create(&m_lddmc_refs, 1024);
    if (!m_lddmc_protected_created) {
        protect_create(&m_lddmc_protected, 4096);
        m_lddmc_protected_created = 1;
    }

    //LACE_ME;
    lddmc_refs_init();
}

void SylvanWrapper::lddmc_refs_init_task() {
    lddmc_refs_internal_t s = (lddmc_refs_internal_t) malloc(sizeof(struct lddmc_refs_internal));
    s->pcur = s->pbegin = (const MDD **) malloc(sizeof(MDD *) * 1024);
    s->pend = s->pbegin + 1024;
    s->rcur = s->rbegin = (MDD *) malloc(sizeof(MDD) * 1024);
    s->rend = s->rbegin + 1024;
    /* s->scur = s->sbegin = (lddmc_refs_task_t)malloc(sizeof(struct lddmc_refs_task) * 1024);
     s->send = s->sbegin + 1024;*/
    SET_THREAD_LOCAL(lddmc_refs_key, s);
}

void SylvanWrapper::lddmc_refs_init() {
    INIT_THREAD_LOCAL(lddmc_refs_key);
    lddmc_refs_init_task();
    // sylvan_gc_add_mark(TASK(lddmc_refs_mark));
}


void SylvanWrapper::refs_create(refs_table2_t *tbl, size_t _refs_size) {
    if (__builtin_popcountll(_refs_size) != 1) {
        fprintf(stderr, "refs: Table size must be a power of 2!\n");
        exit(1);
    }

    tbl->refs_size = _refs_size;
    tbl->refs_table = (uint64_t *) mmap(0, tbl->refs_size * sizeof(uint64_t), PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE | MAP_ANON, -1, 0);
    if (tbl->refs_table == (uint64_t *) -1) {
        fprintf(stderr, "refs: Unable to allocate memory: %s!\n", strerror(errno));
        exit(1);
    }
}

void SylvanWrapper::protect_create(refs_table2_t *tbl, size_t _refs_size) {
    if (__builtin_popcountll(_refs_size) != 1) {
        fprintf(stderr, "refs: Table size must be a power of 2!\n");
        exit(1);
    }

    tbl->refs_size = _refs_size;
    tbl->refs_table = (uint64_t *) mmap(0, tbl->refs_size * sizeof(uint64_t), PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE | MAP_ANON, -1, 0);
    if (tbl->refs_table == (uint64_t *) -1) {
        fprintf(stderr, "refs: Unable to allocate memory: %s!\n", strerror(errno));
        exit(1);
    }
}

size_t SylvanWrapper::llmsset_get_size(const llmsset2_t dbs) {
    return dbs->table_size;
}

/*
 * Initialize GC
 */
void SylvanWrapper::init_gc_seq() {
    m_sequentiel_refs = lddmc_refs_key;
}

/*
 * Return sizes
 */
size_t SylvanWrapper::seq_llmsset_count_marked(llmsset2_t dbs) {
    return llmsset_count_marked_seq(dbs, 0, dbs->table_size);
}

//********************************************
size_t SylvanWrapper::llmsset_count_marked_seq(llmsset2_t dbs, size_t first, size_t count) {
    if (count > 512) {
        size_t split = count / 2;

        size_t right = llmsset_count_marked_seq(dbs, first + split, count - split);
        size_t left = llmsset_count_marked_seq(dbs, first, split);
        return left + right;
    } else {
        size_t result = 0;
        uint64_t *ptr = dbs->bitmap2 + (first / 64);
        if (count == 512) {
            result += __builtin_popcountll(ptr[0]);
            result += __builtin_popcountll(ptr[1]);
            result += __builtin_popcountll(ptr[2]);
            result += __builtin_popcountll(ptr[3]);
            result += __builtin_popcountll(ptr[4]);
            result += __builtin_popcountll(ptr[5]);
            result += __builtin_popcountll(ptr[6]);
            result += __builtin_popcountll(ptr[7]);
        } else {
            uint64_t mask = 0x8000000000000000LL >> (first & 63);
            for (size_t k = 0; k < count; k++) {
                if (*ptr & mask) result += 1;
                mask >>= 1;
                if (mask == 0) {
                    ptr++;
                    mask = 0x8000000000000000LL;
                }
            }
        }
        return result;
    }
}

void SylvanWrapper::displayMDDTableInfo() {
    printf("%zu of %zu buckets filled!\n", seq_llmsset_count_marked(m_nodes), llmsset_get_size(m_nodes));
}

MDD SylvanWrapper::lddmc_cube(uint32_t *values, size_t count) {
    if (count == 0) return lddmc_true;
    return lddmc_makenode(*values, lddmc_cube(values + 1, count - 1), lddmc_false);
}

MDD SylvanWrapper::lddmc_makenode(uint32_t value, MDD ifeq, MDD ifneq) {
    if (ifeq == lddmc_false) return ifneq;


    assert(ifneq != lddmc_true);
    if (ifneq != lddmc_false) assert(value < mddnode_getvalue(LDD_GETNODE(ifneq)));

    struct mddnode n;
    mddnode_make(&n, value, ifneq, ifeq);

    int created;
    uint64_t index = llmsset_lookup(m_nodes, n.a, n.b, &created);
    if (index == 0) {
        /* lddmc_refs_push(ifeq);
         lddmc_refs_push(ifneq);*/

        // sylvan_gc();/*********************************************************************************************/
        /*lddmc_refs_pop(1);*/

        index = llmsset_lookup(m_nodes, n.a, n.b, &created);
        if (index == 0) {
            fprintf(stderr, "MDD Unique table full, %zu of %zu buckets filled!\n", seq_llmsset_count_marked(m_nodes),
                    llmsset_get_size(m_nodes));
            exit(1);
        }
    }

    /*if (created) {
        sylvan_stats_count(LDD_NODES_CREATED);

    }
    else sylvan_stats_count(LDD_NODES_REUSED);*/

    return (MDD) index;
}

mddnode_t SylvanWrapper::LDD_GETNODE(MDD mdd) {
    return ((mddnode_t) llmsset_index_to_ptr(m_nodes, mdd));
}

inline uint64_t
SylvanWrapper::llmsset_lookup2(const llmsset2_t dbs, uint64_t a, uint64_t b, int *created, const int custom) {
    uint64_t hash_rehash = 14695981039346656037LLU;
    if (custom) hash_rehash = dbs->hash_cb(a, b, hash_rehash);
    else hash_rehash = llmsset_hash(a, b, hash_rehash);

    const uint64_t step = (((hash_rehash >> 20) | 1) << 3);
    const uint64_t hash = hash_rehash & MASK_HASH;
    uint64_t idx, last, cidx = 0;
    int i = 0;

#if LLMSSET_MASK
    last = idx = hash_rehash & dbs->mask;
#else
    last = idx = hash_rehash % dbs->table_size;
#endif

    for (;;) {
        volatile uint64_t *bucket = dbs->table + idx;
        uint64_t v = *bucket;

        if (v == 0) {
            if (cidx == 0) {
                // Claim data bucket and write data
                cidx = claim_data_bucket(dbs);
                if (cidx == (uint64_t) -1) return 0; // failed to claim a data bucket
                if (custom) dbs->create_cb(&a, &b);
                uint64_t *d_ptr = ((uint64_t *) dbs->data) + 2 * cidx;
                d_ptr[0] = a;
                d_ptr[1] = b;
            }
            if (cas(bucket, 0, hash | cidx)) {
                if (custom) set_custom_bucket(dbs, cidx, custom);
                *created = 1;
                m_g_created++;
                return cidx;
            } else {
                v = *bucket;
            }
        }

        if (hash == (v & MASK_HASH)) {
            uint64_t d_idx = v & MASK_INDEX;
            uint64_t *d_ptr = ((uint64_t *) dbs->data) + 2 * d_idx;
            if (custom) {
                if (dbs->equals_cb(a, b, d_ptr[0], d_ptr[1])) {
                    if (cidx != 0) {
                        dbs->destroy_cb(a, b);
                        release_data_bucket(dbs, cidx);
                    }
                    *created = 0;
                    return d_idx;
                }
            } else {
                if (d_ptr[0] == a && d_ptr[1] == b) {
                    if (cidx != 0) release_data_bucket(dbs, cidx);
                    *created = 0;
                    return d_idx;
                }
            }
        }

        //sylvan_stats_count(LLMSSET_LOOKUP);

        // find next idx on probe sequence
        idx = (idx & CL_MASK) | ((idx + 1) & CL_MASK_R);
        if (idx == last) {
            if (++i == dbs->threshold) return 0; // failed to find empty spot in probe sequence

            // go to next cache line in probe sequence
            hash_rehash += step;

#if LLMSSET_MASK
            last = idx = hash_rehash & dbs->mask;
#else
            last = idx = hash_rehash % dbs->table_size;
#endif
        }
    }
}

uint64_t SylvanWrapper::llmsset_lookup(const llmsset2_t dbs, const uint64_t a, const uint64_t b, int *created) {
    return llmsset_lookup2(dbs, a, b, created, 0);
}

uint64_t SylvanWrapper::llmsset_hash(const uint64_t a, const uint64_t b, const uint64_t seed) {
    // The FNV-1a hash for 64 bits
    const uint64_t prime = 1099511628211;
    uint64_t hash = seed;
    hash = (hash ^ a) * prime;
    hash = (hash ^ b) * prime;
    return hash ^ (hash >> 32);
}


uint64_t SylvanWrapper::claim_data_bucket(const llmsset2_t dbs) {
    LOCALIZE_THREAD_LOCAL(my_region, uint64_t);
    //printf("%d \n",my_region);
    //printf("Value of my region %d \n",my_region);
    for (;;) {
        if (my_region != (uint64_t) -1) {
            // find empty bucket in region <my_region>
            //printf("My region : %d\n",my_region);
            uint64_t *ptr = dbs->bitmap2 + (my_region * 8);
            int i = 0;
            for (; i < 8;) {
                uint64_t v = *ptr;
                if (v != 0xffffffffffffffffLL) {
                    int j = __builtin_clzll(~v);
                    *ptr |= (0x8000000000000000LL >> j);
                    return (8 * my_region + i) * 64 + j;
                }
                i++;
                ptr++;
            }
        } else {
            //printf("yep!");
            // special case on startup or after garbage collection"
            // my_region += (lace_get_worker()->worker*(dbs->table_size/(64*8)))/lace_workers();
        }
        uint64_t count = dbs->table_size / (64 * 8);
        for (;;) {
            // check if table maybe full
            if (count-- == 0) return (uint64_t) -1;

            my_region += 1;
            if (my_region >= (dbs->table_size / (64 * 8))) my_region = 0;

            // try to claim it
            uint64_t *ptr = dbs->bitmap1 + (my_region / 64);
            uint64_t mask = 0x8000000000000000LL >> (my_region & 63);
            uint64_t v;
            restart:
            v = *ptr;
            if (v & mask) continue; // taken
            if (cas(ptr, v, v | mask)) break;
            else goto restart;
        }
        SET_THREAD_LOCAL(my_region, my_region);
    }
}

void SylvanWrapper::release_data_bucket(const llmsset2_t dbs, uint64_t index) {
    uint64_t *ptr = dbs->bitmap2 + (index / 64);
    uint64_t mask = 0x8000000000000000LL >> (index & 63);
    *ptr &= ~mask;
}


void SylvanWrapper::set_custom_bucket(const llmsset2_t dbs, uint64_t index, int on) {
    uint64_t *ptr = dbs->bitmapc + (index / 64);
    uint64_t mask = 0x8000000000000000LL >> (index & 63);
    if (on) *ptr |= mask;
    else *ptr &= ~mask;
}

MDD __attribute__((unused)) SylvanWrapper::lddmc_refs_push(MDD lddmc) {
    LOCALIZE_THREAD_LOCAL(lddmc_refs_key, lddmc_refs_internal_t);
    *(lddmc_refs_key->rcur++) = lddmc;
    if (lddmc_refs_key->rcur == lddmc_refs_key->rend) return lddmc_refs_refs_up(lddmc_refs_key, lddmc);
    else return lddmc;
}

void __attribute__((unused)) SylvanWrapper::lddmc_refs_pop(long amount) {
    LOCALIZE_THREAD_LOCAL(lddmc_refs_key, lddmc_refs_internal_t);
    lddmc_refs_key->rcur -= amount;
}


MDD __attribute__((noinline)) SylvanWrapper::lddmc_refs_refs_up(lddmc_refs_internal_t lddmc_refs_key, MDD res) {

    long size = lddmc_refs_key->rend - lddmc_refs_key->rbegin;
    lddmc_refs_key->rbegin = (MDD *) realloc(lddmc_refs_key->rbegin, sizeof(MDD) * size * 2);
    lddmc_refs_key->rcur = lddmc_refs_key->rbegin + size;
    lddmc_refs_key->rend = lddmc_refs_key->rbegin + (size * 2);
    return res;
}

size_t SylvanWrapper::lddmc_nodecount(MDD mdd) {
    size_t result = lddmc_nodecount_mark(mdd);
    lddmc_nodecount_unmark(mdd);
    return result;
}

void SylvanWrapper::lddmc_nodecount_unmark(MDD mdd) {
    if (mdd <= lddmc_true) return;
    mddnode_t n = LDD_GETNODE(mdd);
    if (mddnode_getmark(n)) {
        mddnode_setmark(n, 0);
        lddmc_nodecount_unmark(mddnode_getright(n));
        lddmc_nodecount_unmark(mddnode_getdown(n));
    }
}

/**
 * Count number of nodes in MDD
 */

size_t SylvanWrapper::lddmc_nodecount_mark(MDD mdd) {
    if (mdd <= lddmc_true) return 0;
    mddnode_t n = LDD_GETNODE(mdd);
    if (mddnode_getmark(n)) return 0;
    mddnode_setmark(n, 1);
    return 1 + lddmc_nodecount_mark(mddnode_getdown(n)) + lddmc_nodecount_mark(mddnode_getright(n));
}

MDD SylvanWrapper::lddmc_union_mono(MDD a, MDD b) {
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
    if (a < b) {
        MDD tmp = b;
        b = a;
        a = tmp;
    }

    /* Access cache */
    MDD result;
    if (SylvanCacheWrapper::cache_get3(CACHE_LDD_UNION, a, b, 0,&result)) {
       // std::cout<<"Cache succeeded!"<<std::endl;
        return result;
    }


    /* Get nodes */
    mddnode_t na = GETNODE(a);
    mddnode_t nb = GETNODE(b);

    const int na_copy = mddnode_getcopy(na) ? 1 : 0;
    const int nb_copy = mddnode_getcopy(nb) ? 1 : 0;
    const uint32_t na_value = mddnode_getvalue(na);
    const uint32_t nb_value = mddnode_getvalue(nb);

    /* Perform recursive calculation */
    if (na_copy && nb_copy) {

        MDD right = lddmc_union_mono(mddnode_getright(na), mddnode_getright(nb));
        MDD down = lddmc_union_mono(mddnode_getdown(na), mddnode_getdown(nb));
        result = lddmc_make_copynode(down, right);
    } else if (na_copy) {
        MDD right = lddmc_union_mono(mddnode_getright(na), b);
        result = lddmc_make_copynode(mddnode_getdown(na), right);
    } else if (nb_copy) {
        MDD right = lddmc_union_mono(a, mddnode_getright(nb));
        result = lddmc_make_copynode(mddnode_getdown(nb), right);
    } else if (na_value < nb_value) {
        MDD right = lddmc_union_mono(mddnode_getright(na), b);
        result = lddmc_makenode(na_value, mddnode_getdown(na), right);
    } else if (na_value == nb_value) {

        MDD right = lddmc_union_mono(mddnode_getright(na), mddnode_getright(nb));

        MDD down = lddmc_union_mono(mddnode_getdown(na), mddnode_getdown(nb));
        result = lddmc_makenode(na_value, down, right);
    } else /* na_value > nb_value */
    {
        MDD right = lddmc_union_mono(a, mddnode_getright(nb));
        result = lddmc_makenode(nb_value, mddnode_getdown(nb), right);
    }


    SylvanCacheWrapper::cache_put3(CACHE_LDD_UNION, a, b, 0, result);

    return result;
}

MDD SylvanWrapper::lddmc_make_copynode(MDD ifeq, MDD ifneq) {
    struct mddnode n;
    mddnode_makecopy(&n, ifneq, ifeq);

    int created;
    uint64_t index = llmsset_lookup(m_nodes, n.a, n.b, &created);
    if (index == 0) {
        //lddmc_refs_push(ifeq);
        //lddmc_refs_push(ifneq);
        /*LACE_ME;
        sylvan_gc();*/
        //lddmc_refs_pop(1);

        index = llmsset_lookup(m_nodes, n.a, n.b, &created);
        if (index == 0) {
            fprintf(stderr, "MDD Unique table full, %zu of %zu buckets filled!\n", seq_llmsset_count_marked(m_nodes),
                    llmsset_get_size(m_nodes));
            exit(1);
        }
    }

    /*if (created) sylvan_stats_count(LDD_NODES_CREATED);
    else sylvan_stats_count(LDD_NODES_REUSED);*/

    return (MDD) index;
}


bool SylvanWrapper::isSingleMDD(MDD mdd) {
    mddnode_t node;

    while (mdd > lddmc_true) {
        node = GETNODE(mdd);
        if (mddnode_getright(node) != lddmc_false) return false;
        mdd = mddnode_getdown(node);
    }
    return true;
}

// Renvoie le nbre de noeuds à un niveau donné
int SylvanWrapper::get_mddnbr(MDD mdd, int level) {
    mddnode_t node;
    for (int j = 0; j < level; j++) {
        node = GETNODE(mdd);
        mdd = mddnode_getdown(node);
    }
    int i = 0;

    while (mdd != lddmc_false) {
        mddnode_t r = GETNODE(mdd);
        mdd = mddnode_getright(r);
        i++;
    }
    return i;
}

MDD SylvanWrapper::ldd_divide_internal(MDD a, int current_level, int level) {
    /* Terminal cases */

    if (a == lddmc_false) return lddmc_false;
    if (a == lddmc_true) return lddmc_true;

    MDD result;
    if (SylvanCacheWrapper::cache_get3(CACHE_MDD_DIVIDE, a, current_level, level, &result))
    {
        return result;
    }

    mddnode_t node_a = GETNODE(a);
    const uint32_t na_value = mddnode_getvalue(node_a);


    /* Perform recursive calculation */
    if (current_level <= level) {

        MDD down = ldd_divide_internal(mddnode_getdown(node_a), current_level + 1, level);
        MDD right = lddmc_false;
        result = lddmc_makenode(na_value, down, right);
    } else {
        MDD right = ldd_divide_internal(mddnode_getright(node_a), current_level, level);
        MDD down = ldd_divide_internal(mddnode_getdown(node_a), current_level + 1, level);
        result = lddmc_makenode(na_value, down, right);
    }

    SylvanCacheWrapper::cache_put3(CACHE_MDD_DIVIDE, a, current_level, level, result);
    return result;
}


MDD SylvanWrapper::ldd_divide_rec(MDD a, int level) {
    return ldd_divide_internal(a, 0, level);
}

MDD SylvanWrapper::ldd_minus(MDD a, MDD b) {
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
    if (SylvanCacheWrapper::cache_get(CACHE_LDD_MINUS, a, b, &result)) {
        return result;
    }
    /* Get nodes */
    mddnode_t na = GETNODE(a);
    mddnode_t nb = GETNODE(b);
    uint32_t na_value = mddnode_getvalue(na);
    uint32_t nb_value = mddnode_getvalue(nb);

    /* Perform recursive calculation */
    if (na_value < nb_value) {
        MDD right = ldd_minus(mddnode_getright(na), b);
        result = lddmc_makenode(na_value, mddnode_getdown(na), right);
    } else if (na_value == nb_value) {
//        lddmc_refs_spawn(SPAWN(lddmc_minus, mddnode_getright(na), mddnode_getright(nb)));
        MDD down = ldd_minus(mddnode_getdown(na), mddnode_getdown(nb));
        // lddmc_refs_push(down);
        MDD right = ldd_minus(mddnode_getright(na), mddnode_getright(nb));
        //lddmc_refs_pop(1);
        result = lddmc_makenode(na_value, down, right);
    } else /* na_value > nb_value */
    {
        result = ldd_minus(a, mddnode_getright(nb));
    }
    SylvanCacheWrapper::cache_put(CACHE_LDD_MINUS, a, b, result);
    return result;
}


MDD SylvanWrapper::lddmc_firing_mono(MDD cmark, MDD minus, MDD plus) {
    // for an empty set of source states, or an empty transition relation, return the empty set
    if (cmark == lddmc_true) return lddmc_true;
    if (minus == lddmc_false || plus == lddmc_false) return lddmc_false;

    MDD result;
    MDD _cmark = cmark, _minus = minus, _plus = plus;

    if (SylvanCacheWrapper::cache_get3(CACHE_LDD_FIRE, cmark, minus, plus, &result)) {
          return result;
    }
    MDD cache_cmark=cmark;
    mddnode_t n_cmark = GETNODE(_cmark);
    mddnode_t n_plus = GETNODE(_plus);
    mddnode_t n_minus = GETNODE(_minus);


    result = lddmc_false;

    for (;;) {
        uint32_t value;
        value = mddnode_getvalue(n_cmark);
        uint32_t value_minus = mddnode_getvalue(n_minus);
        uint32_t value_plus = mddnode_getvalue(n_plus);
        if (value >= value_minus) {
            MDD result2 =
                    lddmc_makenode(value - value_minus + value_plus,
                                   lddmc_firing_mono(mddnode_getdown(n_cmark), mddnode_getdown(n_minus),
                                                     mddnode_getdown(n_plus)), lddmc_false);


            if (result2 != lddmc_false) result = lddmc_union_mono(result, result2);
        }
        cmark = mddnode_getright(n_cmark);
        if (cmark == lddmc_false) break;
        n_cmark = GETNODE(cmark);
    }
    SylvanCacheWrapper::cache_put3(CACHE_LDD_FIRE, cache_cmark, minus, plus, result);
    return result;
}

// so: proj: -2 (end; quantify rest), -1 (end; keep rest), 0 (quantify), 1 (keep)
MDD SylvanWrapper::lddmc_project( const MDD mdd, const MDD proj)
{
    if (mdd == lddmc_false) return lddmc_false; // projection of empty is empty
    if (mdd == lddmc_true) return lddmc_true; // projection of universe is universe...
    mddnode_t p_node = GETNODE(proj);
    uint32_t p_val = mddnode_getvalue(p_node);
    if (p_val == (uint32_t)-1) return mdd;
    if (p_val == (uint32_t)-2) return lddmc_true; // because we always end with true.

    MDD result;
    if (SylvanCacheWrapper::cache_get3(CACHE_MDD_PROJECT, mdd, proj, 0, &result))
    {
        return result;
    }

    mddnode_t n = LDD_GETNODE(mdd);

    if (p_val == 1)   // keep
    {
        MDD down = lddmc_project( mddnode_getdown(n), mddnode_getdown(p_node));
        //lddmc_refs_push(down);
       // MDD right = lddmc_refs_sync(SYNC(lddmc_project));
        //lddmc_refs_pop(1);
        result = lddmc_makenode(mddnode_getvalue(n), down, right);
    } else     // quantify
    {
        if (mddnode_getdown(n) == lddmc_true)   // assume lowest level
        {
            result = lddmc_true;
        }
        else
        {
            int count = 0;
            result = lddmc_false;
            MDD p_down = mddnode_getdown(p_node), _mdd=mdd;
            while (1)
            {
                //lddmc_refs_spawn(SPAWN(lddmc_project, mddnode_getdown(n), p_down));
                MDD down= lddmc_project (mddnode_getdown(n), p_down);
                result = lddmc_union_mono(result, down);
                count++;
                _mdd = mddnode_getright(n);
                assert(_mdd != lddmc_true);
                if (_mdd == lddmc_false) break;
                n = LDD_GETNODE(_mdd);
            }


        }
    }

    SylvanCacheWrapper::cache_put3(CACHE_MDD_PROJECT, mdd, proj, 0, result);
    return result;
}

int SylvanWrapper::isGCRequired()
{
    return (m_g_created>llmsset_get_size(m_nodes)/2);
}

void SylvanWrapper::convert_wholemdd_stringcpp(MDD cmark,string &res)
{
    typedef pair<string,MDD> Pair_stack;
    vector<Pair_stack> local_stack;

    unsigned int compteur=0;
    MDD explore_mdd=cmark;

    string chaine;

    res="  ";
    local_stack.push_back(Pair_stack(chaine,cmark));
    do
    {
        Pair_stack element=local_stack.back();
        chaine=element.first;
        explore_mdd=element.second;
        local_stack.pop_back();
        compteur++;
        while ( explore_mdd!= lddmc_false  && explore_mdd!=lddmc_true)
        {
            mddnode_t n_val = GETNODE(explore_mdd);
            if (mddnode_getright(n_val)!=lddmc_false)
            {
                local_stack.push_back(Pair_stack(chaine,mddnode_getright(n_val)));
            }
            unsigned int val = mddnode_getvalue(n_val);

            chaine.push_back((unsigned char)(val)+1);
            explore_mdd=mddnode_getdown(n_val);
        }
        /*printchaine(&chaine);printf("\n");*/
        res+=chaine;
    }
    while (local_stack.size()!=0);
    //delete chaine;

    compteur=(compteur<<1) | 1;
    res[1]=(unsigned char)((compteur>>8)+1);
    res[0]=(unsigned char)(compteur & 255);

}






size_t SylvanWrapper::m_table_min = 0;
size_t SylvanWrapper::m_table_max = 0;
size_t SylvanWrapper::m_cache_min = 0;
size_t SylvanWrapper::m_cache_max = 0;
/**
 * This variable is used for a cas flag so only one gc runs at one time
 */
volatile int SylvanWrapper::m_gc;
llmsset2_t SylvanWrapper::m_nodes;
/**
 * External refrences
 */
refs_table2_t SylvanWrapper::m_lddmc_refs;
refs_table2_t SylvanWrapper::m_lddmc_protected;
int SylvanWrapper::m_lddmc_protected_created = 0;

/*
 * Internal references
 */
lddmc_refs_internal_t SylvanWrapper::m_sequentiel_refs;
//lddmc_refs_internal_t SylvanWrapper::m_lddmc_refs_key;

/** Sylvan table**/
uint32_t SylvanWrapper::m_g_created;