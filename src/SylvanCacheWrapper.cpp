//
// Created by chiheb on 25/09/2020.
//
#include <string.h> // memset
#include <sys/mman.h> // for mmap
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cerrno>
#include "SylvanCacheWrapper.h"

#ifndef compiler_barrier
#define compiler_barrier() { asm volatile("" ::: "memory"); }
#endif

#ifndef cas
#define cas(ptr, old, new) (__sync_bool_compare_and_swap((ptr),(old),(new)))
#endif

void SylvanCacheWrapper::cache_create(size_t _cache_size, size_t _max_size)
{
#if CACHE_MASK
    // Cache size must be a power of 2
    if (__builtin_popcountll(_cache_size) != 1 || __builtin_popcountll(_max_size) != 1) {
        fprintf(stderr, "cache_create: Table size must be a power of 2!\n");
        exit(1);
    }
#endif

    m_cache_size = _cache_size;
    m_cache_max  = _max_size;
#if CACHE_MASK
    cache_mask = cache_size - 1;
#endif

    if (m_cache_size > m_cache_max) {
        fprintf(stderr, "cache_create: Table size must be <= max size!\n");
        exit(1);
    }

    m_cache_table = (cache_entry_t)mmap(0, m_cache_max * sizeof(struct cache_entry), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    m_cache_status = (uint32_t*)mmap(0, m_cache_max * sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (m_cache_table == (cache_entry_t)-1 || m_cache_status == (uint32_t*)-1) {
        fprintf(stderr, "cache_create: Unable to allocate memory: %s!\n", strerror(errno));
        exit(1);
    }

    m_next_opid = 512LL << 40;
}

/**
 * dd must be MTBDD, d2/d3 can be anything
 */
int __attribute__((unused))
SylvanCacheWrapper::cache_get3(uint64_t opid, uint64_t dd, uint64_t d2, uint64_t d3, uint64_t *res)
{
    return cache_get(dd | opid, d2, d3, res);
}

int SylvanCacheWrapper::cache_get(uint64_t a, uint64_t b, uint64_t c, uint64_t *res)
{
    const uint64_t hash = cache_hash(a, b, c);
#if CACHE_MASK
    volatile uint32_t *s_bucket = m_cache_status + (hash & m_cache_mask);
    cache_entry_t bucket = m_cache_table + (hash & m_cache_mask);
#else
    volatile uint32_t *s_bucket = m_cache_status + (hash % m_cache_size);
    cache_entry_t bucket = m_cache_table + (hash % m_cache_size);
#endif
    const uint32_t s = *s_bucket;
    compiler_barrier();
    // abort if locked or if part of a 2-part cache entry
    if (s & 0xc0000000) return 0;
    // abort if different hash
    if ((s ^ (hash>>32)) & 0x3fff0000) return 0;
    // abort if key different
    if (bucket->a != a || bucket->b != b || bucket->c != c) return 0;
    *res = bucket->res;
    compiler_barrier();
    // abort if status field changed after compiler_barrier()
    return *s_bucket == s ? 1 : 0;
}

/* Rotating 64-bit FNV-1a hash */
uint64_t SylvanCacheWrapper::cache_hash(uint64_t a, uint64_t b, uint64_t c)
{
    const uint64_t prime = 1099511628211;
    uint64_t hash = 14695981039346656037LLU;
    hash = (hash ^ (a>>32));
    hash = (hash ^ a) * prime;
    hash = (hash ^ b) * prime;
    hash = (hash ^ c) * prime;
    return hash;
}

/**
 * dd must be MTBDD, d2/d3 can be anything
 */
int __attribute__((unused))
SylvanCacheWrapper::cache_put3(uint64_t opid, uint64_t dd, uint64_t d2, uint64_t d3, uint64_t res)
{
    return cache_put(dd | opid, d2, d3, res);
}

int SylvanCacheWrapper::cache_put(uint64_t a, uint64_t b, uint64_t c, uint64_t res)
{
    const uint64_t hash = cache_hash(a, b, c);
#if CACHE_MASK
    volatile uint32_t *s_bucket = cache_status + (hash & cache_mask);
    cache_entry_t bucket = cache_table + (hash & cache_mask);
#else
    volatile uint32_t *s_bucket = m_cache_status + (hash % m_cache_size);
    cache_entry_t bucket = m_cache_table + (hash % m_cache_size);
#endif
    const uint32_t s = *s_bucket;
    // abort if locked
    if (s & 0x80000000) return 0;
    // abort if hash identical -> no: in iscasmc this occasionally causes timeouts?!
    const uint32_t hash_mask = (hash>>32) & 0x3fff0000;
    // if ((s & 0x7fff0000) == hash_mask) return 0;
    // use cas to claim bucket
    const uint32_t new_s = ((s+1) & 0x0000ffff) | hash_mask;
    if (!cas(s_bucket, s, new_s | 0x80000000)) return 0;
    // cas succesful: write data
    bucket->a = a;
    bucket->b = b;
    bucket->c = c;
    bucket->res = res;
    compiler_barrier();
    // after compiler_barrier(), unlock status field
    *s_bucket = new_s;
    return 1;
}

void SylvanCacheWrapper::cache_clear()
{
    // a bit silly, but this works just fine, and does not require writing 0 everywhere...
    cache_free();
    cache_create(m_cache_size, m_cache_max);
}

void SylvanCacheWrapper::cache_free()
{
    munmap(m_cache_table, m_cache_max * sizeof(struct cache_entry));
    munmap(m_cache_status, m_cache_max * sizeof(uint32_t));
}






size_t             SylvanCacheWrapper::m_cache_size;         // power of 2
size_t             SylvanCacheWrapper::m_cache_max;
cache_entry_t      SylvanCacheWrapper::m_cache_table;
uint32_t*          SylvanCacheWrapper::m_cache_status;
uint64_t           SylvanCacheWrapper::m_next_opid;