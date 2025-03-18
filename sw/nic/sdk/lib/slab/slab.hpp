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


//------------------------------------------------------------------------------

//
// slab library that provides an object cache for faster alloc/free operations
//------------------------------------------------------------------------------

#ifndef __SDK_SLAB_HPP__
#define __SDK_SLAB_HPP__

#include "include/sdk/base.hpp"
#include "include/sdk/lock.hpp"
#include "include/sdk/mem.hpp"
#include "lib/shmmgr/shmmgr.hpp"

namespace sdk {
namespace lib {

typedef bool (slab_walk_cb_t)(void *elem, void *ctxt);

// slab id servers as unique identifier for a slab instance, this id is not used
// by the library itself
typedef uint32_t slab_id_t;
enum {
    SDK_SLAB_ID_TWHEEL,
    SDK_SLAB_ID_PORT_PD,

    // hbm hash slabs
    SDK_SLAB_ID_HBM_HASH_ENTRY,
    SDK_SLAB_ID_HBM_HASH_SW_KEY,
    SDK_SLAB_ID_HBM_HASH_HW_KEY,
    SDK_SLAB_ID_HBM_HASH_SW_DATA,
    SDK_SLAB_ID_HBM_HASH_HINT_GROUP,
    SDK_SLAB_ID_HBM_HASH_TABLE_ENTRY,
    SDK_SLAB_ID_HBM_HASH_SPINE_ENTRY,
    SDK_SLAB_ID_CRYPTO_PEND_REQ_PD,
    SDK_SLAB_ID_EVENT_MAP,
    SDK_SLAB_ID_EVENT_LISTENER_STATE,
    SDK_SLAB_ID_EVENT_LISTENER,


    SDK_SLAB_ID_RSVD,   // Note: Add slabs above this.
};

#define SLAB_NAME_MAX_LEN 24

typedef struct slab_block_s slab_block_t;
struct slab_block_s {
    slab_block_t    *prev_;         // previous block pointer
    slab_block_t    *next_;         // next block pointer
    void            *free_head_;    // pointer to free element in block
    uint32_t        num_in_use_;    // no. of elements in use in this block
    uint8_t         elems_[0];      // memory for block elements
} __PACK__;

class slab {
public:
    // static methods for instantiating and destroying slabs
    // slab_id is unique id to identify this slab (library doesn't use this)
    static slab *factory(const char *name, slab_id_t slab_id, uint32_t elem_sz,
                         uint32_t elems_per_block, bool thread_safe=false,
                         bool grow_on_demand=true, bool zero_on_alloc=false,
                         shmmgr *mmgr = NULL);
    static void destroy(slab *slb);

    // per instance member functions
    void *alloc(void);
    void free(void *elem);
    // walk elements of the slab, if callback returns true, walk is stopped
    sdk_ret_t walk(slab_walk_cb_t walk_cb, void *ctxt);
    uint32_t num_in_use(void) const { return num_in_use_; }
    uint32_t num_allocs(void) const { return num_allocs_; }
    uint32_t num_frees(void) const { return num_frees_; }
    uint32_t num_alloc_fails(void) const { return num_alloc_fails_; }
    uint32_t num_blocks(void) const { return num_blocks_; }

    slab_id_t slab_id(void) const { return slab_id_; }
    const char *name(void) const { return name_; }
    uint32_t elem_sz(void) const { return elem_sz_; }
    uint32_t elems_per_block(void) const { return elems_per_block_; }
    uint32_t raw_block_sz(void) const {return raw_block_sz_; }
    bool thread_safe(void) const { return thread_safe_; }
    bool grow_on_demand(void) const { return grow_on_demand_; }
    bool zero_on_alloc(void) const { return zero_on_alloc_; }

private:
    // slab private state
    sdk_spinlock_t    slock_;
    char              name_[SLAB_NAME_MAX_LEN+1];
    slab_id_t         slab_id_;
    uint32_t          elem_sz_;
    uint32_t          elems_per_block_;
    uint32_t          raw_block_sz_;
    bool              thread_safe_;
    bool              grow_on_demand_;
    bool              zero_on_alloc_;

    // statistics
    uint32_t          num_in_use_;
    uint32_t          num_allocs_;
    uint32_t          num_frees_;
    uint32_t          num_alloc_fails_;
    uint32_t          num_blocks_;

    // meta data
    slab_block_t      *block_head_;
    shmmgr            *mmgr_;

private:
    slab() {};
    ~slab();
    bool init(const char *name, slab_id_t slab_id, uint32_t elem_sz,
              uint32_t elems_per_block, bool thread_safe, bool grow_on_demand,
              bool zero_on_alloc, shmmgr *mmgr);
    void free_block_(slab_block_t *block);
    void free_(void *elem);
    slab_block_t *alloc_block_(void);
};

}    // namespace lib
}    // namespace sdk

using sdk::lib::slab;

#endif    // __SDK_SLAB_HPP__

