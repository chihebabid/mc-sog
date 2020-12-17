//
// Created by chiheb on 10/06/2020.
//

#ifndef PMC_SOG_SYLVANWRAPPER_H
#define PMC_SOG_SYLVANWRAPPER_H
#include <cstdint>
#include <cstddef>
#include <string>

/**
 * Lockless hash table (set) to store 16-byte keys.
 * Each unique key is associated with a 42-bit number.
 *
 * The set has support for stop-the-world garbage collection.
 * Methods llmsset_clear, llmsset_mark and llmsset_rehash implement garbage collection.
 * During their execution, llmsset_lookup is not allowed.
 *
 * WARNING: Originally, this table is designed to allow multiple tables.
 * However, this is not compatible with thread local storage for now.
 * Do not use multiple tables.
 */

/**
 * hash(a, b, seed)
 * equals(lhs_a, lhs_b, rhs_a, rhs_b)
 * create(a, b) -- with a,b pointers, allows changing pointers on create of node,
 *                 but must keep hash/equals same!
 * destroy(a, b)
 */


typedef uint64_t (*llmsset_hash_cb)(uint64_t, uint64_t, uint64_t);

typedef int (*llmsset_equals_cb)(uint64_t, uint64_t, uint64_t, uint64_t);

typedef void (*llmsset_create_cb)(uint64_t *, uint64_t *);

typedef void (*llmsset_destroy_cb)(uint64_t, uint64_t);


typedef uint64_t MDD;       // Note: low 40 bits only
extern MDD lddmc_false; //= 0;
extern MDD lddmc_true;// = 1;

typedef struct llmsset2 {
    uint64_t *table;       // table with hashes
    uint8_t *data;        // table with values
    uint64_t *bitmap1;     // ownership bitmap (per 512 buckets)
    uint64_t *bitmap2;     // bitmap for "contains data"
    uint64_t *bitmapc;     // bitmap for "use custom functions"
    size_t max_size;     // maximum size of the hash table (for resizing)
    size_t table_size;   // size of the hash table (number of slots) --> power of 2!
#if LLMSSET_MASK
    size_t            mask;         // size-1
#endif
    size_t f_size;
    llmsset_hash_cb hash_cb;      // custom hash function
    llmsset_equals_cb equals_cb;    // custom equals function
    llmsset_create_cb create_cb;    // custom create function
    llmsset_destroy_cb destroy_cb;  // custom destroy function
    int16_t threshold;    // number of iterations for insertion until returning error
} *llmsset2_t;

/**
 * LDD node structure
 *
 * RmRR RRRR RRRR VVVV | VVVV DcDD DDDD DDDD (little endian - in memory)
 * VVVV RRRR RRRR RRRm | DDDD DDDD DDDc VVVV (big endian)
 */
typedef struct __attribute__((packed)) mddnode {
    uint64_t a, b;
} *mddnode_t; // 16 bytes

/**
 * Implementation of external references
 * Based on a hash table for 40-bit non-null values, linear probing
 * Use tombstones for deleting, higher bits for reference count
 */
typedef struct {
    uint64_t *refs_table;           // table itself
    size_t refs_size;               // number of buckets

    /* helpers during resize operation */
    volatile uint32_t refs_control; // control field
    uint64_t *refs_resize_table;    // previous table
    size_t refs_resize_size;        // size of previous table
    size_t refs_resize_part;        // which part is next
    size_t refs_resize_done;        // how many parts are done
} refs_table2_t;
/************************************** From lace.h******/



typedef struct lddmc_refs_internal {
    const MDD **pbegin, **pend, **pcur;
    MDD *rbegin, *rend, *rcur;
} *lddmc_refs_internal_t;


#ifndef cas
#define cas(ptr, old, new) (__sync_bool_compare_and_swap((ptr),(old),(new)))
#endif
/* Typical cacheline size of system architectures */
#ifndef LINE_SIZE
#define LINE_SIZE 64
#endif
// The LINE_SIZE is defined in lace.h
static const uint64_t CL_MASK = ~(((LINE_SIZE) / 8) - 1);
static const uint64_t CL_MASK_R = ((LINE_SIZE) / 8) - 1;
/* 40 bits for the index, 24 bits for the hash */
#define MASK_INDEX ((uint64_t)0x000000ffffffffff)
#define MASK_HASH  ((uint64_t)0xffffff0000000000)

class SylvanWrapper {
public:
    static uint64_t getMarksCount(MDD cmark);

    static llmsset2_t llmsset_create(size_t initial_size, size_t max_size);

    static void llmsset_set_size(llmsset2_t dbs, size_t size);

    static void sylvan_set_limits(size_t memorycap, int table_ratio, int initial_ratio);

    static void sylvan_init_package();

    inline static mddnode_t GETNODE(MDD mdd) { return ((mddnode_t) llmsset_index_to_ptr(m_nodes, mdd)); }

    static void *llmsset_index_to_ptr(const llmsset2_t dbs, size_t index) {
        return dbs->data + index * 16;
    }

    // Explore basic operations : to isolated to another file
    static inline uint64_t __attribute__((unused)) mddnode_getright(mddnode_t n) {
        return (n->a & 0x0000ffffffffffff) >> 1;
    }

    static inline uint32_t __attribute__((unused)) mddnode_getvalue(mddnode_t n) {
        return *(uint32_t *) ((uint8_t *) n + 6);
    }

    static inline uint64_t __attribute__((unused)) mddnode_getdown(mddnode_t n) {
        return n->b >> 17;
    }

    static void sylvan_init_ldd();

    static void refs_create(refs_table2_t *tbl, size_t _refs_size);

    static void protect_create(refs_table2_t *tbl, size_t _refs_size);

    static void init_gc_seq();

    static size_t llmsset_get_size(const llmsset2_t dbs);

    static size_t seq_llmsset_count_marked(llmsset2_t dbs);

    static size_t llmsset_count_marked_seq(llmsset2_t dbs, size_t first, size_t count);

    static void displayMDDTableInfo();

    static MDD lddmc_cube(uint32_t *values, size_t count);

    static MDD lddmc_makenode(uint32_t value, MDD ifeq, MDD ifneq);

    static mddnode_t LDD_GETNODE(MDD mdd);

    static MDD __attribute__((unused)) lddmc_refs_push(MDD lddmc);

    static void __attribute__((unused)) lddmc_refs_pop(long amount);

    static MDD __attribute__((noinline)) lddmc_refs_refs_up(lddmc_refs_internal_t lddmc_refs_key, MDD res);

    static size_t lddmc_nodecount(MDD mdd);

    static MDD lddmc_union_mono(MDD a, MDD b);

    static bool isSingleMDD(MDD mdd);

    static int get_mddnbr(MDD mdd, unsigned int level);

    static MDD ldd_divide_rec(MDD a, int level);

    static MDD ldd_divide_internal(MDD a, int current_level, int level);

    static MDD ldd_minus(MDD a, MDD b);

    static MDD lddmc_firing_mono(MDD cmark, const MDD minus, const MDD plus);

    static size_t lddmc_nodecount_mark(MDD mdd);

    static void lddmc_nodecount_unmark(MDD mdd);

    static MDD lddmc_make_copynode(MDD ifeq, MDD ifneq);

    static void lddmc_refs_init();

    static void lddmc_refs_init_task();

    // sylvan_int.h
    static inline void __attribute__((unused))
    mddnode_makecopy(mddnode_t n, uint64_t right, uint64_t down) {
        n->a = right << 1;
        n->b = ((down << 1) | 1) << 16;
    }

    static inline uint8_t __attribute__((unused)) mddnode_getcopy(mddnode_t n) {
        return n->b & 0x10000 ? 1 : 0;
    }

    static inline uint8_t __attribute__((unused)) mddnode_getmark(mddnode_t n) {
        return n->a & 1;
    }

    static inline void __attribute__((unused)) mddnode_setmark(mddnode_t n, uint8_t mark) {
        n->a = (n->a & 0xfffffffffffffffe) | (mark ? 1 : 0);
    }

    //sylvan_table.c
    static inline void __attribute__((unused))
    mddnode_make(mddnode_t n, uint32_t value, uint64_t right, uint64_t down) {
        n->a = right << 1;
        n->b = down << 17;
        *(uint32_t *) ((uint8_t *) n + 6) = value;
    }

    static inline uint64_t
    llmsset_lookup2(const llmsset2_t dbs, uint64_t a, uint64_t b, int *created, const int custom);

    static uint64_t llmsset_lookup(const llmsset2_t dbs, const uint64_t a, const uint64_t b, int *created);

    static uint64_t llmsset_hash(const uint64_t a, const uint64_t b, const uint64_t seed);

    static uint64_t claim_data_bucket(const llmsset2_t dbs);

    static void release_data_bucket(const llmsset2_t dbs, uint64_t index);

    static void set_custom_bucket(const llmsset2_t dbs, uint64_t index, int on);

    static void llmsset_reset_region();
    static MDD lddmc_project( const MDD mdd, const MDD proj);
    static int isGCRequired();
    static void convert_wholemdd_stringcpp(MDD cmark,std::string &res);
    static void sylvan_gc_seq();
private:
    static int is_custom_bucket(const llmsset2_t dbs, uint64_t index);
    static int llmsset_rehash_bucket(const llmsset2_t dbs, uint64_t d_idx);
    static int llmsset_rehash_seq (llmsset2_t dbs);
    static int llmsset_rehash_par_seq(llmsset2_t  dbs, size_t  first, size_t  count);
    static void llmsset_clear_hashes_seq( llmsset2_t dbs);
    static void sylvan_rehash_all();
    static void llmsset_destroy(llmsset2_t dbs, size_t first, size_t count);
    static void llmsset_destroy_unmarked( llmsset2_t dbs);
    static void llmsset_clear_data(llmsset2_t dbs);
    static void ldd_gc_mark_protected();
    static void ldd_refs_mark_p_par( const MDD** begin, size_t count);
    static void ldd_gc_mark_rec(MDD mdd);
    static int  llmsset_mark(const llmsset2_t dbs, uint64_t index);
    static void ldd_refs_mark_r_par( MDD* begin, size_t count);


    static llmsset2_t m_nodes;
    static size_t m_table_min, m_table_max, m_cache_min, m_cache_max;
    static volatile int m_gc;

    static refs_table2_t m_lddmc_refs; // External references
    static refs_table2_t m_lddmc_protected;// External references
    static int m_lddmc_protected_created;// External references

    static lddmc_refs_internal_t m_sequentiel_refs;
    //static lddmc_refs_internal_t m_lddmc_refs_key; // This is local to a thread

    static uint32_t m_g_created;




};


#endif //PMC_SOG_SYLVANWRAPPER_H
