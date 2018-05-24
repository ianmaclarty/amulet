#include "amulet.h"

/*
 * This is the allocator we use for Lua. We use a custom allocator to
 * take advantage of the fact that Lua already keeps track of the sizes
 * of objects, and tells us the object's size when freeing it. We also
 * don't need to worry about thread safety, since each Lua engine only runs
 * on one thread.
 * For small objects we use a pool of freelists for each size category.
 * For large objects, and to allocate the pools themselves, we use dlmalloc.
 * We don't use the system malloc since that might have to worry about threads,
 * and also using the same malloc across platforms should be more consistent.
 */

//#define AM_PRINT_ALLOC_STATS 1
//#define AM_NO_SMALL_ALLOCATOR 1
//#define AM_USE_DLMALLOC 1

#if defined(AM_USE_DLMALLOC)

#define MSPACES 1
#define ONLY_MSPACES 1

#include "dlmalloc.inc"

#endif

// smallest object size, and size category increment, expressed as number of bits
#define CELL_SZ 3

// number of small object pools.
#define NUM_POOLS 64

// maximum contiguous freelist block size
#define MAX_BLOCK_SIZE 10240

// macro to get pool number from size
#define GET_POOL(sz) ((sz - 1) >> CELL_SZ)

#ifdef AM_PRINT_ALLOC_STATS
struct pool_stats {
    uint64_t nallocs;
    uint64_t nfrees;
    uint64_t hwm_cells;
    uint64_t hwm_sz;
    uint64_t allocsz;
    uint64_t freesz;
};

static void update_stats_alloc(pool_stats *stats, size_t sz) {
    stats->nallocs++;
    uint64_t curr = stats->nallocs - stats->nfrees;
    if (curr > stats->hwm_cells) {
        stats->hwm_cells = curr;
    }
    stats->allocsz += sz;
    curr = stats->allocsz - stats->freesz;
    if (curr > stats->hwm_sz) {
        stats->hwm_sz = curr;
    }
    //am_debug("ALLOC %d (alloced = %d, freed = %d)", (int)sz, (int)stats->allocsz, (int)stats->freesz);
    assert(stats->allocsz >= stats->freesz);
}

static void update_stats_free(pool_stats *stats, size_t sz) {
    stats->nfrees++;
    stats->freesz += sz;
    //am_debug("FREE %d (alloced = %d, freed = %d)", (int)sz, (int)stats->allocsz, (int)stats->freesz);
    assert(stats->allocsz >= stats->freesz);
}
#endif

struct am_pool {
    void **freelist;
    void **blocks;
    int num_blocks;
    size_t cellsize;
    size_t blocksize;
#ifdef AM_PRINT_ALLOC_STATS
    pool_stats stats;
#endif
};

struct am_allocator {
#ifdef AM_PRINT_ALLOC_STATS
    pool_stats heap_stats;
#endif
#ifdef AM_USE_DLMALLOC
    mspace ms;
#endif
    am_pool pools[NUM_POOLS];
};

#ifndef AM_NO_SMALL_ALLOCATOR
static void *am_malloc(am_allocator *allocator, size_t sz) {
#ifdef AM_USE_DLMALLOC
    return mspace_malloc(allocator->ms, sz);
#else
    return malloc(sz);
#endif
}
#endif

static void am_free(am_allocator *allocator, void *ptr) {
#ifdef AM_USE_DLMALLOC
    return mspace_free(allocator->ms, ptr);
#else
    return free(ptr);
#endif
}

static void *am_realloc(am_allocator *allocator, void *ptr, size_t sz) {
#ifdef AM_USE_DLMALLOC
    return mspace_realloc(allocator->ms, ptr, sz);
#else
    return realloc(ptr, sz);
#endif
}

am_allocator *am_new_allocator() {
    am_allocator *allocator = new am_allocator();
    memset(allocator, 0, sizeof(am_allocator));
#ifdef AM_USE_DLMALLOC
    allocator->ms = create_mspace(0, 0);
#endif
    int cellsize = 1 << CELL_SZ;
    for (int p = 0; p < NUM_POOLS; p++) {
        allocator->pools[p].cellsize = cellsize;
        allocator->pools[p].blocksize = (MAX_BLOCK_SIZE / cellsize) * cellsize;
        cellsize += (1 << CELL_SZ);
    }
    return allocator;
}

#ifdef AM_NO_SMALL_ALLOCATOR

void *am_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    am_allocator *allocator = (am_allocator*)ud;
    if (nsize == 0) {
        am_free(allocator, ptr);
        return NULL;
    }
    return am_realloc(allocator, ptr, nsize);
}

#else

static void free_pool_cell(am_pool *pool, void *ptr, size_t sz) {
    void **cell = (void**)ptr;
    *cell = (void*)pool->freelist;
    pool->freelist = cell;
#ifdef AM_PRINT_ALLOC_STATS
    update_stats_free(&pool->stats, sz);
#endif
}

static void do_free(am_allocator *allocator, void *ptr, size_t sz) {
    int pool = GET_POOL(sz);
    if (pool < NUM_POOLS) {
        free_pool_cell(&allocator->pools[pool], ptr, sz);
    } else {
        am_free(allocator, ptr);
#ifdef AM_PRINT_ALLOC_STATS
        update_stats_free(&allocator->heap_stats, sz);
#endif
    }
}

static void init_block(void *block, size_t cellsize, size_t blocksize) {
    void **cell = (void**)block;
    int num_cells = blocksize / cellsize;
    for (int i = 0; i < num_cells - 1; i++) {
        void **next = (void**)(((uint8_t*)cell) + cellsize);
        *cell = (void*)next;
        cell = next;
    }
    *cell = NULL; // last element
}

static void *alloc_pool_cell(am_allocator *allocator, am_pool *pool, size_t sz) {
#ifdef AM_PRINT_ALLOC_STATS
    update_stats_alloc(&pool->stats, sz);
#endif
    if (pool->freelist != NULL) {
        void *ptr = (void*)pool->freelist;
        pool->freelist = (void**)(*(pool->freelist));
        return ptr;
    } else {
        // need to allocate new block
        pool->num_blocks++;
        pool->blocks = (void**)am_realloc(allocator, pool->blocks, sizeof(void*) * pool->num_blocks);
        void *block = am_malloc(allocator, pool->blocksize);
        pool->blocks[pool->num_blocks - 1] = block;
        init_block(block, pool->cellsize, pool->blocksize);
        void **first = (void**)block;
        pool->freelist = (void**)(*first);
        return (void*)first;
    }
}

void *am_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    //am_debug("am_alloc(%p, %p, %d, %d)", ud, ptr, (int)osize, (int)nsize);
    am_allocator *allocator = (am_allocator*)ud;
    if (nsize == 0) {
        if (ptr == NULL) return NULL;
        do_free(allocator, ptr, osize);
        return NULL;
    } else {
        if (ptr == NULL) {
            // new object
            int pool = GET_POOL(nsize);
            if (pool < NUM_POOLS) {
                return alloc_pool_cell(allocator, &allocator->pools[pool], nsize);
            } else {
#ifdef AM_PRINT_ALLOC_STATS
                update_stats_alloc(&allocator->heap_stats, nsize);
#endif
                return am_malloc(allocator, nsize);
            }
        } else {
            // resize existing object
            int opool = GET_POOL(osize);
            int npool = GET_POOL(nsize);
            if (opool >= NUM_POOLS && npool >= NUM_POOLS) {
#ifdef AM_PRINT_ALLOC_STATS
                update_stats_free(&allocator->heap_stats, osize);
                update_stats_alloc(&allocator->heap_stats, nsize);
#endif
                return am_realloc(allocator, ptr, nsize);
            } else {
                void *newcell;
                if (npool < NUM_POOLS) {
                    newcell = alloc_pool_cell(allocator, &allocator->pools[npool], nsize);
                } else {
#ifdef AM_PRINT_ALLOC_STATS
                    update_stats_alloc(&allocator->heap_stats, nsize);
#endif
                    newcell = am_malloc(allocator, nsize);
                }
                memcpy(newcell, ptr, am_min(osize, nsize));
                do_free(allocator, ptr, osize);
                return newcell;
            }
        }
    }
}

#endif

static void free_pool_blocks(am_allocator *allocator, am_pool *pool) {
    if (pool->num_blocks > 0) {
        for (int i = 0; i < pool->num_blocks; i++) {
            am_free(allocator, pool->blocks[i]);
        }
        am_free(allocator, pool->blocks);
    }
}

#ifdef AM_PRINT_ALLOC_STATS
static void log_pool_stats(const char *name, am_pool *pool) {
    am_log0(      "%-6s: %6u %6u %10u %10u %10u %6d %10uk %10uk %10uk   %4.0f%%",
        name,
        (unsigned)pool->cellsize,
        (unsigned)pool->blocksize,
        (unsigned)pool->stats.nallocs,
        (unsigned)pool->stats.nfrees,
        (unsigned)pool->stats.hwm_cells,
        pool->num_blocks,
        (unsigned)pool->stats.allocsz / 1024,
        (unsigned)pool->stats.freesz / 1024,
        (unsigned)pool->stats.hwm_sz / 1024,
        (pool->stats.nallocs == 0 ? 100.0 : 
            (double)pool->stats.allocsz / (double)(pool->stats.nallocs * pool->cellsize) * 100.0));
}
#endif

void am_destroy_allocator(am_allocator *allocator) {
#ifdef AM_PRINT_ALLOC_STATS
    am_log0("%s", "-------------------------------------------------");
    am_log0("%s", "Allocation stats:");
    am_log0(      "%-6s  %6s %6s %10s %10s %10s %6s  %10s  %10s  %10s    %4s ",
        "", "cellsz", "blksz", "nallocs", "nfrees", "hwm", "blks", "allock", "freesk", " hwmk", "util");
    char poolnm[7];
    int total_blks = 0;
    int total_blk_sz = 0;
    for (int p = 0; p < NUM_POOLS; p++) {
        snprintf(poolnm, 6, "p%d", p);
        log_pool_stats(poolnm, &allocator->pools[p]);
        total_blks += allocator->pools[p].num_blocks;
        total_blk_sz += allocator->pools[p].num_blocks * allocator->pools[p].blocksize;
    }
    am_log0(      "total pool blocks: %d, total pool blocks size: %d", total_blks, total_blk_sz);
    am_log0(      "%-6s: %6s %6s %10u %10u %10u %6s %10uk %10uk %10uk   %4s",
        "large", "-", "-", 
        (unsigned)allocator->heap_stats.nallocs,
        (unsigned)allocator->heap_stats.nfrees,
        (unsigned)allocator->heap_stats.hwm_cells,
        "-",
        (unsigned)allocator->heap_stats.allocsz / 1024,
        (unsigned)allocator->heap_stats.freesz / 1024,
        (unsigned)allocator->heap_stats.hwm_sz / 1024,
        "-");
#endif 
    for (int p = 0; p < NUM_POOLS; p++) {
        free_pool_blocks(allocator, &allocator->pools[p]);
    }
#ifdef AM_USE_DLMALLOC
    destroy_mspace(allocator->ms);
#endif
    delete allocator;
}
