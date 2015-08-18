#include "amulet.h"

//#define AM_PRINT_ALLOC_INFO 1
#define AM_MALLOC_ONLY 1

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

#define TINY_CELL_SZ 32
#define SMALL_CELL_SZ (UD_SZ + sizeof(am_vec4))
#define MEDIUM_CELL_SZ (UD_SZ + 120)

#define TINY_BLOCK_SZ (TINY_CELL_SZ * 1024)
#define SMALL_BLOCK_SZ (SMALL_CELL_SZ * 1024)
#define MEDIUM_BLOCK_SZ (MEDIUM_CELL_SZ * 1024)

struct am_pool {
    void **freelist;
    void **blocks;
    int num_blocks;
    size_t cellsize;
    size_t blocksize;
};

struct am_allocator {
    am_pool tiny_pool;
    am_pool small_pool;
    am_pool medium_pool;
};

am_allocator *am_new_allocator() {
    am_allocator *allocator = new am_allocator();
    allocator->tiny_pool.freelist = NULL;
    allocator->tiny_pool.blocks = NULL;
    allocator->tiny_pool.num_blocks = 0;
    allocator->tiny_pool.cellsize = TINY_CELL_SZ;
    allocator->tiny_pool.blocksize = TINY_BLOCK_SZ;
    allocator->small_pool.freelist = NULL;
    allocator->small_pool.blocks = NULL;
    allocator->small_pool.num_blocks = 0;
    allocator->small_pool.cellsize = SMALL_CELL_SZ;
    allocator->small_pool.blocksize = SMALL_BLOCK_SZ;
    allocator->medium_pool.freelist = NULL;
    allocator->medium_pool.blocks = NULL;
    allocator->medium_pool.num_blocks = 0;
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
}

static void init_block(void *block, size_t cellsize, size_t blocksize) {
    void **cell = (void**)block;
    size_t num_cells = blocksize / cellsize;
    for (int i = 0; i < num_cells - 1; i++) {
        void **next = (void**)(((char*)cell) + cellsize);
        *cell = (void*)next;
        cell = next;
    }
    *cell = NULL; // last element
}

static void *alloc_pool_cell(am_pool *pool) {
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
        return first;
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
                return alloc_pool_cell(&allocator->tiny_pool);
            } 
            if (nsize <= SMALL_CELL_SZ) {
                return alloc_pool_cell(&allocator->small_pool);
            }
            if (nsize <= MEDIUM_CELL_SZ) {
                return alloc_pool_cell(&allocator->medium_pool);
            }
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
                void *new_ptr = alloc_pool_cell(&allocator->small_pool);
                memcpy(new_ptr, ptr, nsize);
                free(ptr);
                return new_ptr;
            }
            // new object goes on heap
            if (osize <= MEDIUM_CELL_SZ) {
                // move from pool to heap
                void *new_ptr = malloc(nsize);
                memcpy(new_ptr, ptr, osize);
                do_free(allocator, ptr, osize);
                return new_ptr;
            }
            // ptr was already on heap
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

void am_destroy_allocator(am_allocator *allocator) {
#ifdef AM_PRINT_ALLOC_INFO
    am_log0("tiny cell size:    %4d", (int)TINY_CELL_SZ);
    am_log0("small cell size:   %4d", (int)SMALL_CELL_SZ);
    am_log0("medium cell size:  %4d", (int)MEDIUM_CELL_SZ);
#endif 
    free_pool_blocks(&allocator->tiny_pool);
    free_pool_blocks(&allocator->small_pool);
    free_pool_blocks(&allocator->medium_pool);
    delete allocator;
}
