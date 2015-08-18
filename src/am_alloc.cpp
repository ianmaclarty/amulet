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

#define TINY_CELL_SZ 32
#define SMALL_CELL_SZ 80
#define MEDIUM_CELL_SZ 160

#define TINY_BLOCK_SZ (TINY_CELL_SZ * 1024)
#define SMALL_BLOCK_SZ (SMALL_CELL_SZ * 1024)
#define MEDIUM_BLOCK_SZ (MEDIUM_CELL_SZ * 1024)

struct am_pool {
    void **freelist;
    void **blocks;
    int num_blocks;
    size_t cellsize;
    size_t blocksize;
#ifdef AM_PRINT_ALLOC_STATS
    uint64_t nallocs;
    uint64_t nfrees;
    uint64_t hwm;
#endif
};

struct am_allocator {
    am_pool tiny_pool;
    am_pool small_pool;
    am_pool medium_pool;
#ifdef AM_PRINT_ALLOC_STATS
    uint64_t nlarge_allocs;
    uint64_t nlarge_frees;
    uint64_t large_hwm;
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

static void free_pool_cell(am_pool *pool, void *ptr) {
    void **cell = (void**)ptr;
    *cell = (void*)pool->freelist;
    pool->freelist = cell;
#ifdef AM_PRINT_ALLOC_STATS
    pool->nfrees++;
#endif
}

static void do_free(am_allocator *allocator, void *ptr, size_t sz) {
    if (sz <= TINY_CELL_SZ) {
        free_pool_cell(&allocator->tiny_pool, ptr);
        return;
    }
    if (sz <= SMALL_CELL_SZ) {
        free_pool_cell(&allocator->small_pool, ptr);
        return;
    }
    if (sz <= MEDIUM_CELL_SZ) {
        free_pool_cell(&allocator->medium_pool, ptr);
        return;
    }
    free(ptr);
#ifdef AM_PRINT_ALLOC_STATS
    allocator->nlarge_frees++;
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

static void *alloc_pool_cell(am_pool *pool) {
#ifdef AM_PRINT_ALLOC_STATS
    pool->nallocs++;
    uint64_t curr = pool->nallocs - pool->nfrees;
    if (curr > pool->hwm) {
        pool->hwm = curr;
    }
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

/*
static void *am_alloc_2(void *ud, void *ptr, size_t osize, size_t nsize);

static void *dumb_realloc(am_allocator *allocator, void *ptr, size_t osize, size_t nsize) {
    void *new_ptr = am_alloc_2(allocator, NULL, 0, nsize);
    memcpy(new_ptr, ptr, am_min(osize, nsize));
    do_free(allocator, ptr, osize);
    return new_ptr;
}
*/

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
                return alloc_pool_cell(&allocator->tiny_pool);
            } 
            if (nsize <= SMALL_CELL_SZ) {
                return alloc_pool_cell(&allocator->small_pool);
            }
            if (nsize <= MEDIUM_CELL_SZ) {
                return alloc_pool_cell(&allocator->medium_pool);
            }
#ifdef AM_PRINT_ALLOC_STATS
            allocator->nlarge_allocs++;
            uint64_t curr = allocator->nlarge_allocs - allocator->nlarge_frees;
            if (curr > allocator->large_hwm) {
                allocator->large_hwm = curr;
            }
#endif
            return malloc(nsize);
        } else {
            // resize existing object
            if (nsize <= TINY_CELL_SZ) {
                if (osize > TINY_CELL_SZ) {
                    // move to tiny pool
                    void *new_ptr = alloc_pool_cell(&allocator->tiny_pool);
                    memcpy(new_ptr, ptr, nsize);
                    do_free(allocator, ptr, osize);
                    return new_ptr;
                }
                // keep existing cell
                return ptr;
            }
            if (nsize <= SMALL_CELL_SZ) {
                if (osize <= TINY_CELL_SZ) {
                    // move from tiny to small pool
                    void *new_ptr = alloc_pool_cell(&allocator->small_pool);
                    memcpy(new_ptr, ptr, osize);
                    do_free(allocator, ptr, osize);
                    return new_ptr;
                }
                if (osize <= SMALL_CELL_SZ) {
                    // keep existing cell
                    return ptr;
                }
                // move from larger pool to small pool 
                void *new_ptr = alloc_pool_cell(&allocator->small_pool);
                memcpy(new_ptr, ptr, nsize);
                do_free(allocator, ptr, osize);
                return new_ptr;
            }
            if (nsize <= MEDIUM_CELL_SZ) {
                if (osize <= SMALL_CELL_SZ) {
                    // move from smaller pool to medium pool
                    void *new_ptr = alloc_pool_cell(&allocator->medium_pool);
                    memcpy(new_ptr, ptr, osize);
                    do_free(allocator, ptr, osize);
                    return new_ptr;
                }
                if (osize <= MEDIUM_CELL_SZ) {
                    // keep existing cell
                    return ptr;
                }
                // move from heap to medium pool 
                void *new_ptr = alloc_pool_cell(&allocator->medium_pool);
                memcpy(new_ptr, ptr, nsize);
                free(ptr);
#ifdef AM_PRINT_ALLOC_STATS
                allocator->nlarge_frees++;
#endif
                return new_ptr;
            }
            // new object goes on heap
            if (osize <= MEDIUM_CELL_SZ) {
                // move from pool to heap
                void *new_ptr = malloc(nsize);
#ifdef AM_PRINT_ALLOC_STATS
                allocator->nlarge_allocs++;
                uint64_t curr = allocator->nlarge_allocs - allocator->nlarge_frees;
                if (curr > allocator->large_hwm) {
                    allocator->large_hwm = curr;
                }
#endif
                memcpy(new_ptr, ptr, osize);
                do_free(allocator, ptr, osize);
                return new_ptr;
            }
            // ptr was already on heap
#ifdef AM_PRINT_ALLOC_STATS
            allocator->nlarge_allocs++;
            allocator->nlarge_frees++;
#endif
            return realloc(ptr, nsize);
        }
    }
}

/*
void *am_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    void *r = am_alloc_2(ud, ptr, osize, nsize);
    if (nsize > 0) {
        void *buf = malloc(nsize);
        memcpy(buf, r, nsize);
        memcpy(r, buf, nsize);
        free(buf);
    }
    return r;
}
*/

#endif

static void free_pool_blocks(am_pool *pool) {
    if (pool->num_blocks > 0) {
        for (int i = 0; i < pool->num_blocks; i++) {
            free(pool->blocks[i]);
        }
        free(pool->blocks);
    }
}

void am_destroy_allocator(am_allocator *allocator) {
#ifdef AM_PRINT_ALLOC_STATS
    am_log0("%s", "Allocation stats:");
    am_log0("%s", "            allocs      frees        hwm     blocks     cellsz    blocksz");
    am_log0(      "tiny:   %10lu %10lu %10lu %10d %10lu %10lu",
        allocator->tiny_pool.nallocs,
        allocator->tiny_pool.nfrees,
        allocator->tiny_pool.hwm,
        allocator->tiny_pool.num_blocks,
        allocator->tiny_pool.cellsize,
        allocator->tiny_pool.blocksize);
    am_log0(      "small:  %10lu %10lu %10lu %10d %10lu %10lu",
        allocator->small_pool.nallocs,
        allocator->small_pool.nfrees,
        allocator->small_pool.hwm,
        allocator->small_pool.num_blocks,
        allocator->small_pool.cellsize,
        allocator->small_pool.blocksize);
    am_log0(      "medium: %10lu %10lu %10lu %10d %10lu %10lu",
        allocator->medium_pool.nallocs,
        allocator->medium_pool.nfrees,
        allocator->medium_pool.hwm,
        allocator->medium_pool.num_blocks,
        allocator->medium_pool.cellsize,
        allocator->medium_pool.blocksize);
    am_log0(      "large:  %10lu %10lu %10lu",
        allocator->nlarge_allocs,
        allocator->nlarge_frees,
        allocator->large_hwm);
#endif 
    free_pool_blocks(&allocator->tiny_pool);
    free_pool_blocks(&allocator->small_pool);
    free_pool_blocks(&allocator->medium_pool);
    delete allocator;
}
