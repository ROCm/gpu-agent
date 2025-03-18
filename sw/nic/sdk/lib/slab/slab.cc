/*
Copyright (c) Advanced Micro Devices, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


#include <string.h>
#include <stdlib.h>
#include "include/sdk/mem.hpp"
#include "include/sdk/assert.hpp"
#include "lib/mmgr/mmgr.hpp"
#include "lib/slab/slab.hpp"

#define SDK_DEBUG 0

namespace sdk {
namespace lib {

// per element meta that is added internally
typedef struct slab_emeta_s {
    uint32_t        in_use:1;
    uint32_t        rsvd:31;               // for future use
} __PACK__ slab_emeta_t;
#define SLAB_ELEM_META_SIZE        sizeof(slab_emeta_t)

//------------------------------------------------------------------------------
// slab instance initialization
//------------------------------------------------------------------------------
bool
slab::init(const char *name, slab_id_t slab_id, uint32_t elem_sz,
           uint32_t elems_per_block, bool thread_safe, bool grow_on_demand,
           bool zero_on_alloc, shmmgr *mmgr)
{
    mmgr_ = mmgr;
    if (thread_safe) {
        if (mmgr) {
            SDK_SPINLOCK_INIT(&slock_, PTHREAD_PROCESS_SHARED);
        } else {
            SDK_SPINLOCK_INIT(&slock_, PTHREAD_PROCESS_PRIVATE);
        }

    }
    this->thread_safe_ = thread_safe;

    strncpy(this->name_, name, SLAB_NAME_MAX_LEN);
    this->name_[SLAB_NAME_MAX_LEN] = '\0';
    this->slab_id_ = slab_id;
    elem_sz += SLAB_ELEM_META_SIZE;
    this->elem_sz_ = (elem_sz + 7) & ~0x07;
    this->elems_per_block_ = elems_per_block;
    this->raw_block_sz_ = (elem_sz_ * elems_per_block_) + sizeof(slab_block_t);
    this->grow_on_demand_ = grow_on_demand;
    this->zero_on_alloc_ = zero_on_alloc;

    this->num_in_use_ = 0;
    this->num_allocs_ = 0;
    this->num_frees_ = 0;
    this->num_alloc_fails_ = 0;
    this->num_blocks_ = 0;

#if 0
    // allocate a block and keep it ready
    this->block_head_ = alloc_block_();
    if (this->block_head_ == NULL) {
        return -1;
    }
#endif

    this->block_head_ = NULL;

    return true;
}

//------------------------------------------------------------------------------
// factory method
//------------------------------------------------------------------------------
slab *
slab::factory(const char *name, slab_id_t slab_id, uint32_t elem_sz,
              uint32_t elems_per_block, bool thread_safe, bool grow_on_demand,
              bool zero_on_alloc, shmmgr *mmgr)
{
    void    *mem;
    slab    *new_slab = NULL;

    if ((elems_per_block <= 1) || (elem_sz < 8)) {
        return NULL;
    }

#if SDK_DEBUG
    SDK_TRACE_DEBUG("slab name %s, slab id %u, elem sz %u, "
                    "elems per block %u", name, slab_id, elem_sz,
                    elems_per_block);
#endif

    if (mmgr) {
        mem = mmgr->alloc(sizeof(slab), 4, true);
    } else {
        mem = SDK_CALLOC(SDK_MEM_ALLOC_LIB_SLAB, sizeof(slab));
    }
    if (mem == NULL) {
        SDK_TRACE_ERR("Failed to create slab %s, id %u, elem sz %u, "
                      "elems per block %u", name, slab_id, elem_sz,
                      elems_per_block);
        return NULL;
    }

    new_slab = new (mem) slab();
    if (new_slab->init(name, slab_id, elem_sz, elems_per_block, thread_safe,
                       grow_on_demand, zero_on_alloc, mmgr) == false) {
        SDK_TRACE_ERR("Failed to initialize slab %s, id %u", name, slab_id);
        new_slab->~slab();
        if (mmgr) {
            mmgr->free(mem);
        } else {
            SDK_FREE(SDK_MEM_ALLOC_LIB_SLAB, mem);
        }
        return NULL;
    }

    return new_slab;
}

//------------------------------------------------------------------------------
// destructor
//------------------------------------------------------------------------------
slab::~slab()
{
    slab_block_t    *block;

    // free all the blocks allocated so far
    if (thread_safe_) {
        SDK_SPINLOCK_LOCK(&slock_);
    }
    block = this->block_head_;
    while (block) {
        this->block_head_ = block->next_;
        if (mmgr_) {
            mmgr_->free(block);
        } else {
            SDK_FREE(SDK_MEM_ALLOC_LIB_SLAB, block);
        }
        block = this->block_head_;
    }
    if (thread_safe_) {
        SDK_SPINLOCK_DESTROY(&slock_);
    }
}

void
slab::destroy(slab *slb)
{
    shmmgr    *mmgr;

    if (!slb) {
        return;
    }
    mmgr = slb->mmgr_;
    slb->~slab();
    if (mmgr) {
        mmgr->free(slb);
    } else {
        SDK_FREE(SDK_MEM_ALLOC_LIB_SLAB, slb);
    }
}

//------------------------------------------------------------------------------
// private function to allocate and initialize a new block
//------------------------------------------------------------------------------
slab_block_t *
slab::alloc_block_(void)
{
    slab_block_t    *block;
    slab_emeta_t    *emeta;
    uint8_t         *ptr;

#if SDK_DEBUG
    SDK_TRACE_DEBUG("Allocating block for slab %s", name_);
#endif

    if (mmgr_) {
        block = (slab_block_t *)mmgr_->alloc(raw_block_sz_, 4, true);
    } else {
        block = (slab_block_t *)SDK_MALLOC(SDK_MEM_ALLOC_LIB_SLAB,
                                           raw_block_sz_);
    }
    if (block == NULL) {
        SDK_TRACE_ERR("Failed to allocate block for slab %s, slab id %u",
                      name_, slab_id_);
        return NULL;
    }
    block->prev_ = block->next_ = NULL;
    block->free_head_ = block->elems_;
    block->num_in_use_ = 0;

    ptr = block->elems_;
    for (uint32_t i = 0; i < elems_per_block_ - 1; i++) {
        emeta = (slab_emeta_t *)ptr;
        emeta->in_use = FALSE;
        *(void **)(emeta + 1) = ptr + elem_sz_;
        ptr += elem_sz_;
    }

    // initialize the last element
    emeta = (slab_emeta_t *)ptr;
    emeta->in_use = FALSE;
    *(void **)(emeta + 1) = NULL;
    this->num_blocks_++;

    return block;
}

//------------------------------------------------------------------------------
// allocate an object
//------------------------------------------------------------------------------
void *
slab::alloc(void)
{
    void            *elem = NULL;
    slab_block_t    *block;
    slab_emeta_t    *emeta;

    //return calloc(1, this->elem_sz_);

    if (thread_safe_) {
        SDK_SPINLOCK_LOCK(&slock_);
    }

    block = this->block_head_;
    while (block && (block->free_head_  == NULL)) {
        block = block->next_;
    }

    // allocate a new block if all blocks are fully utilized
    if (block == NULL) {
        if (grow_on_demand_ || (this->block_head_ == NULL)) {
            block = alloc_block_();
            if (block) {
                block->next_ = this->block_head_;
                if (this->block_head_) {
                    this->block_head_->prev_ = block;
                }
                this->block_head_ = block;
            } else {
                goto cleanup;
            }
        } else {
            goto cleanup;
        }
    }

    emeta = (slab_emeta_t *)block->free_head_;
    emeta->in_use = TRUE;
    elem = emeta + 1;
    block->free_head_ = *(void **)elem;
    this->num_allocs_++;
    this->num_in_use_++;
    block->num_in_use_++;

    if (thread_safe_) {
        SDK_SPINLOCK_UNLOCK(&slock_);
    }

    if (this->zero_on_alloc_) {
        memset(elem, 0, (this->elem_sz_ - SLAB_ELEM_META_SIZE));
    }

#if SDK_DEBUG
    SDK_TRACE_DEBUG("Alloc called for slab %s, ret elem 0x%x, "
                    "block 0x%x", name_, elem, block);
#endif
    return elem;

cleanup:

    this->num_alloc_fails_++;
    if (thread_safe_) {
        SDK_SPINLOCK_UNLOCK(&slock_);
    }
    return NULL;
}

void
slab::free_block_(slab_block_t *block) {
    if (block_head_ == block) {
        block_head_ = block->next_;
    } else {
        block->prev_->next_ = block->next_;
        if (block->next_) {
            block->next_->prev_ = block->prev_;
        }
    }
    if (mmgr_) {
        mmgr_->free(block);
    } else {
        SDK_FREE(SDK_MEM_ALLOC_LIB_SLAB, block);
    }
    this->num_blocks_--;
}

//------------------------------------------------------------------------------
// internal function to free an object
//------------------------------------------------------------------------------
void
slab::free_(void *elem)
{
    slab_block_t    *block;
    slab_emeta_t    *emeta;
    void            *ptr;

    SDK_ASSERT(elem != NULL);
    ptr = emeta = (slab_emeta_t *)(((uint8_t *)elem) - SLAB_ELEM_META_SIZE);
    SDK_ASSERT(emeta->in_use == TRUE);

    // find the block this element belongs to
    block = this->block_head_;
    while (block) {
        if ((ptr > block) &&
            (ptr < ((uint8_t *)block + raw_block_sz_))) {
            break;
        }
        block = block->next_;
    }

    if (block) {
        *(void **)elem = block->free_head_;
        block->free_head_ = emeta;
        emeta->in_use = FALSE;
        this->num_frees_++;
        this->num_in_use_--;
        block->num_in_use_--;
        if ((block->num_in_use_ == 0) && grow_on_demand_ && block->next_) {
            // free the block
            free_block_(block);
#if SDK_DEBUG
            SDK_TRACE_DEBUG("Free block to slab %s", name_);
#endif
        }
    } else {
        // this elem doesn't belong to any of this slab's active blocks
        SDK_ASSERT(FALSE);
    }
}

//------------------------------------------------------------------------------
// free an object
//------------------------------------------------------------------------------
void
slab::free(void *elem)
{
#if SDK_DEBUG
    SDK_TRACE_DEBUG("Free called for slab %s, elem 0x%x", name_, elem);
#endif

    //return ::free(elem);

    if (thread_safe_) {
        SDK_SPINLOCK_LOCK(&slock_);
    }
    free_(elem);
    if (thread_safe_) {
        SDK_SPINLOCK_UNLOCK(&slock_);
    }
}

sdk_ret_t
slab::walk(slab_walk_cb_t walk_cb, void *ctxt)
{
    uint32_t        i;
    uint8_t         *ptr;
    slab_block_t    *block;
    slab_emeta_t    *emeta;
    bool            stop_walk = false;

    SDK_ASSERT_RETURN((walk_cb != NULL), SDK_RET_INVALID_ARG);
    for (block = this->block_head_; block; block = block->next_) {
        if (block->num_in_use_ == 0) {
            continue;
        }
        ptr = block->elems_;
        for (i = 0; i < elems_per_block_; i++) {
            emeta = (slab_emeta_t *)ptr;
            if (emeta->in_use == TRUE) {
                stop_walk = walk_cb(emeta + 1, ctxt);
                if (stop_walk) {
                    goto end;
                }
            }
            ptr += elem_sz_;
        }
    }

end:

    return SDK_RET_OK;
}

}    // namespace lib
}    // namespace sdk
