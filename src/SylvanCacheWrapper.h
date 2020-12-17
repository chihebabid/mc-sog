//
// Created by chiheb on 25/09/2020.
//

#ifndef PMC_SOG_SYLVANCACHEWRAPPER_H
#define PMC_SOG_SYLVANCACHEWRAPPER_H

/**
 * Literals for cache
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// MDD operations
static const uint64_t CACHE_MDD_RELPROD             = (10LL<<40);
static const uint64_t CACHE_MDD_MINUS               = (11LL<<40);
static const uint64_t CACHE_MDD_UNION               = (12LL<<40);
static const uint64_t CACHE_MDD_INTERSECT           = (13LL<<40);
static const uint64_t CACHE_MDD_PROJECT             = (14LL<<40);
static const uint64_t CACHE_MDD_JOIN                = (15LL<<40);
static const uint64_t CACHE_MDD_MATCH               = (16LL<<40);
static const uint64_t CACHE_MDD_RELPREV             = (17LL<<40);
static const uint64_t CACHE_MDD_SATCOUNT            = (18LL<<40);
static const uint64_t CACHE_MDD_DIVIDE              = (19LL<<40);
static const uint64_t CACHE_LDD_MINUS               = (20LL<<40);
static const uint64_t CACHE_LDD_FIRE                = (21LL<<40);
static const uint64_t CACHE_LDD_UNION                = (22LL<<40);


#ifdef __cplusplus
}
#endif /* __cplusplus */







/**
 * This cache is designed to store a,b,c->res, with a,b,c,res 64-bit integers.
 *
 * Each cache bucket takes 32 bytes, 2 per cache line.
 * Each cache status bucket takes 4 bytes, 16 per cache line.
 * Therefore, size 2^N = 36*(2^N) bytes.
 */

struct __attribute__((packed)) cache6_entry {
    uint64_t            a;
    uint64_t            b;
    uint64_t            c;
    uint64_t            res;
    uint64_t            d;
    uint64_t            e;
    uint64_t            f;
    uint64_t            res2;
};
typedef struct cache6_entry *cache6_entry_t;

struct __attribute__((packed)) cache_entry {
    uint64_t            a;
    uint64_t            b;
    uint64_t            c;
    uint64_t            res;
};

typedef struct cache_entry *cache_entry_t;
class SylvanCacheWrapper {
public:
    static void cache_create(size_t _cache_size, size_t _max_size);
    static int __attribute__((unused)) cache_get3(uint64_t opid, uint64_t dd, uint64_t d2, uint64_t d3, uint64_t *res);
    static  int __attribute__((unused)) cache_put3(uint64_t opid, uint64_t dd, uint64_t d2, uint64_t d3, uint64_t res);
    static int cache_put(uint64_t a, uint64_t b, uint64_t c, uint64_t res);
    static int cache_get(uint64_t a, uint64_t b, uint64_t c, uint64_t *res);
    static void cache_clear();
private:
    static size_t             m_cache_size;         // power of 2
    static size_t             m_cache_max;
    static cache_entry_t      m_cache_table;
    static uint32_t*          m_cache_status;
    static uint64_t           m_next_opid;
    static uint64_t cache_hash(uint64_t a, uint64_t b, uint64_t c);
    static void cache_free();
};


#endif //PMC_SOG_SYLVANCACHEWRAPPER_H
