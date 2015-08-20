#include "amulet.h"

//#define AM_PRINT_ALLOC_STATS 1
//#define AM_MALLOC_ONLY 1

#ifdef AM_LUAJIT

#include "lj_gc.h"
#include "lj_obj.h"
#include "lj_state.h"

#define UD_SZ sizeof(GCudata)

#else // AM_LUAJIT

#include "lapi.h"
#include "lgc.h"
#include "lobject.h"

#define UD_SZ sizeof(Udata)

#endif // AM_LUAJIT

// XXX Tweak these for 32/64 bit + lua/luajit:

#define TINY_CELL_SZ 64
#define SMALL_CELL_SZ 72
#define MEDIUM_CELL_SZ 160

#define TINY_BLOCK_SZ (TINY_CELL_SZ * 2048)
#define SMALL_BLOCK_SZ (SMALL_CELL_SZ * 1024)
#define MEDIUM_BLOCK_SZ (MEDIUM_CELL_SZ * 1024)

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
    am_pool tiny_pool;
    am_pool small_pool;
    am_pool medium_pool;
#ifdef AM_PRINT_ALLOC_STATS
    pool_stats heap_stats;
#endif
};

am_allocator *am_new_allocator() {
    am_allocator *allocator = new am_allocator();
    memset(allocator, 0, sizeof(am_allocator));
    allocator->tiny_pool.cellsize = TINY_CELL_SZ;
    allocator->tiny_pool.blocksize = TINY_BLOCK_SZ;
    allocator->small_pool.cellsize = SMALL_CELL_SZ;
    allocator->small_pool.blocksize = SMALL_BLOCK_SZ;
    allocator->medium_pool.cellsize = MEDIUM_CELL_SZ;
    allocator->medium_pool.blocksize = MEDIUM_BLOCK_SZ;
    return allocator;
}

#ifdef AM_MALLOC_ONLY

void *am_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    if (nsize == 0) {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, nsize);
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
    if (sz <= TINY_CELL_SZ) {
        free_pool_cell(&allocator->tiny_pool, ptr, sz);
        return;
    }
    if (sz <= SMALL_CELL_SZ) {
        free_pool_cell(&allocator->small_pool, ptr, sz);
        return;
    }
    if (sz <= MEDIUM_CELL_SZ) {
        free_pool_cell(&allocator->medium_pool, ptr, sz);
        return;
    }
    free(ptr);
#ifdef AM_PRINT_ALLOC_STATS
    update_stats_free(&allocator->heap_stats, sz);
#endif
}

static void init_block(void *block, size_t cellsize, size_t blocksize) {
    void **cell = (void**)block;
    int num_cells = blocksize / cellsize;
    for (int i = 0; i < num_cells - 1; i++) {
        void **next = (void**)(((char*)cell) + cellsize);
        *cell = (void*)next;
        cell = next;
    }
    *cell = NULL; // last element
}

static void *alloc_pool_cell(am_pool *pool, size_t sz) {
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
        pool->blocks = (void**)realloc(pool->blocks, sizeof(void*) * pool->num_blocks);
        void *block = malloc(pool->blocksize);
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
            if (nsize <= TINY_CELL_SZ) {
                return alloc_pool_cell(&allocator->tiny_pool, nsize);
            } 
            if (nsize <= SMALL_CELL_SZ) {
                return alloc_pool_cell(&allocator->small_pool, nsize);
            }
            if (nsize <= MEDIUM_CELL_SZ) {
                return alloc_pool_cell(&allocator->medium_pool, nsize);
            }
#ifdef AM_PRINT_ALLOC_STATS
            update_stats_alloc(&allocator->heap_stats, nsize);
#endif
            return malloc(nsize);
        } else {
            // resize existing object
            if (nsize <= TINY_CELL_SZ) {
                if (osize > TINY_CELL_SZ) {
                    // move to tiny pool
                    void *new_ptr = alloc_pool_cell(&allocator->tiny_pool, nsize);
                    memcpy(new_ptr, ptr, nsize);
                    do_free(allocator, ptr, osize);
                    return new_ptr;
                }
                // keep existing cell
#ifdef AM_PRINT_ALLOC_STATS
                update_stats_free(&allocator->tiny_pool.stats, osize);
                update_stats_alloc(&allocator->tiny_pool.stats, nsize);
#endif
                return ptr;
            }
            if (nsize <= SMALL_CELL_SZ) {
                if (osize <= TINY_CELL_SZ) {
                    // move from tiny to small pool
                    void *new_ptr = alloc_pool_cell(&allocator->small_pool, nsize);
                    memcpy(new_ptr, ptr, osize);
                    do_free(allocator, ptr, osize);
                    return new_ptr;
                }
                if (osize <= SMALL_CELL_SZ) {
                    // keep existing cell
#ifdef AM_PRINT_ALLOC_STATS
                    update_stats_free(&allocator->small_pool.stats, osize);
                    update_stats_alloc(&allocator->small_pool.stats, nsize);
#endif
                    return ptr;
                }
                // move from larger pool to small pool 
                void *new_ptr = alloc_pool_cell(&allocator->small_pool, nsize);
                memcpy(new_ptr, ptr, nsize);
                do_free(allocator, ptr, osize);
                return new_ptr;
            }
            if (nsize <= MEDIUM_CELL_SZ) {
                if (osize <= SMALL_CELL_SZ) {
                    // move from smaller pool to medium pool
                    void *new_ptr = alloc_pool_cell(&allocator->medium_pool, nsize);
                    memcpy(new_ptr, ptr, osize);
                    do_free(allocator, ptr, osize);
                    return new_ptr;
                }
                if (osize <= MEDIUM_CELL_SZ) {
                    // keep existing cell
#ifdef AM_PRINT_ALLOC_STATS
                    update_stats_free(&allocator->medium_pool.stats, osize);
                    update_stats_alloc(&allocator->medium_pool.stats, nsize);
#endif
                    return ptr;
                }
                // move from heap to medium pool 
                void *new_ptr = alloc_pool_cell(&allocator->medium_pool, nsize);
                memcpy(new_ptr, ptr, nsize);
                free(ptr);
#ifdef AM_PRINT_ALLOC_STATS
                update_stats_free(&allocator->heap_stats, osize);
#endif
                return new_ptr;
            }
            // new object goes on heap
            if (osize <= MEDIUM_CELL_SZ) {
                // move from pool to heap
                void *new_ptr = malloc(nsize);
#ifdef AM_PRINT_ALLOC_STATS
                update_stats_alloc(&allocator->heap_stats, nsize);
#endif
                memcpy(new_ptr, ptr, osize);
                do_free(allocator, ptr, osize);
                return new_ptr;
            }
            // ptr was already on heap
#ifdef AM_PRINT_ALLOC_STATS
            update_stats_free(&allocator->heap_stats, osize);
            update_stats_alloc(&allocator->heap_stats, nsize);
#endif
            return realloc(ptr, nsize);
        }
    }
}

#endif

static void free_pool_blocks(am_pool *pool) {
    if (pool->num_blocks > 0) {
        for (int i = 0; i < pool->num_blocks; i++) {
            free(pool->blocks[i]);
        }
        free(pool->blocks);
    }
}

#ifdef AM_PRINT_ALLOC_STATS
static void log_pool_stats(const char *name, am_pool *pool) {
    am_log0(      "%-6s: %6lu %6lu %10lu %10lu %10lu %6d %10luk %10luk %10luk   %4.0f%%",
        name,
        pool->cellsize,
        pool->blocksize,
        pool->stats.nallocs,
        pool->stats.nfrees,
        pool->stats.hwm_cells,
        pool->num_blocks,
        pool->stats.allocsz / 1024,
        pool->stats.freesz / 1024,
        pool->stats.hwm_sz / 1024,
        (double)pool->stats.allocsz / (double)(pool->stats.nallocs * pool->cellsize) * 100.0);
}
#endif

void am_destroy_allocator(am_allocator *allocator) {
#ifdef AM_PRINT_ALLOC_STATS
    am_log0("%s", "Allocation stats:");
    am_log0(      "%-6s  %6s %6s %10s %10s %10s %6s  %10s  %10s  %10s    %4s ",
        "", "cellsz", "blksz", "nallocs", "nfrees", "hwm", "blks", "allock", "freesk", " hwmk", "util");
    log_pool_stats("tiny", &allocator->tiny_pool);
    log_pool_stats("small", &allocator->small_pool);
    log_pool_stats("medium", &allocator->medium_pool);
    am_log0(      "%-6s: %6s %6s %10lu %10lu %10lu %6s %10luk %10luk %10luk   %4s",
        "large", "-", "-", 
        allocator->heap_stats.nallocs,
        allocator->heap_stats.nfrees,
        allocator->heap_stats.hwm_cells,
        "-",
        allocator->heap_stats.allocsz / 1024,
        allocator->heap_stats.freesz / 1024,
        allocator->heap_stats.hwm_sz / 1024,
        "-");
#endif 
    free_pool_blocks(&allocator->tiny_pool);
    free_pool_blocks(&allocator->small_pool);
    free_pool_blocks(&allocator->medium_pool);
    delete allocator;
}
