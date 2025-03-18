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


#ifndef __SDK_MTRACK_HPP__
#define __SDK_MTRACK_HPP__

#ifdef __cplusplus
#include <map>
#endif    // #ifdef __cplusplus

#include "lib/logger/logger.h"
#include "include/sdk/lock.hpp"

#ifdef __cplusplus
using namespace std;
namespace sdk {
namespace utils {

// info stored by memory tracker per allocation id
typedef struct mtrack_info_s {
    uint32_t    num_allocs;
    uint32_t    num_frees;
} __PACK__ mtrack_info_t;
typedef std::map<uint32_t, mtrack_info_t *> mtrack_map_t;
typedef std::map<uint32_t, mtrack_info_t *>::iterator mtrack_map_it_t;

class mem_mgr {
public:
    mem_mgr() : mtrack_map_() {
        SDK_SPINLOCK_INIT(&mtrack_map_slock_, PTHREAD_PROCESS_PRIVATE);
        enabled_ = true;
    }

    ~mem_mgr() {
        mtrack_map_it_t    it;

        for (it = mtrack_map_.begin(); it != mtrack_map_.end(); ) {
            free(it->second);
            mtrack_map_.erase(it++);
        }
    }

    void enable(void) { enabled_ = true; }
    bool enabled(void) const { return enabled_; }
    void disable(void) { enabled_ = false; }

    void *mtrack_alloc(uint32_t alloc_id, bool zero,
                       uint32_t size, const char *func, uint32_t line) {
        mtrack_info_t       *mtrack_info;
        mtrack_map_it_t     it;
        void                *mem;
        bool                free_mem = false;

        (void) func;
        (void) line;
        // HAL_TRACE_DEBUG("MEMORY_ALLOC for id: {}, func:{}, line: {}, size: {}",
        //                alloc_id, func, line, size);

        // SDK_ASSERT(alloc_id < hal::HAL_MEM_ALLOC_OTHER);
        mem = zero ? calloc(1, size) : malloc(size);
        if (!mem) {
            return NULL;
        }

        // if mem tracking is not enabled, nothing more to do
        if (!enabled_) {
            return mem;
        }

        SDK_SPINLOCK_LOCK(&mtrack_map_slock_);
        it = mtrack_map_.find(alloc_id);
        if (it != mtrack_map_.end()) {
            it->second->num_allocs++;
            //HAL_TRACE_DEBUG("mem_allocation for id: {}, allocs:{}",
                            //alloc_id, it->second->num_allocs);
        } else {
            mtrack_info = (mtrack_info_t *)calloc(1, sizeof(mtrack_info_t));
            if (mtrack_info == NULL) {
                free_mem = true;
            } else {
                mtrack_info->num_allocs = 1;
                mtrack_map_[alloc_id] = mtrack_info;
            }
        }
        SDK_SPINLOCK_UNLOCK(&mtrack_map_slock_);

        if (free_mem) {
            free(mem);
            return NULL;
        }
        return mem;
    }

    void mtrack_free(uint32_t alloc_id, void *ptr,
                     const char *func, uint32_t line) {
        bool                free_mem = false;
        mtrack_info_t       *mtrack_info;
        mtrack_map_it_t     it;

        (void) func;
        (void) line;
        if (!ptr) {
            return;
        }
        ::free(ptr);

        // if mem tracking is not enabled, nothing more to do
        if (!enabled_) {
            return;
        }

        SDK_SPINLOCK_LOCK(&mtrack_map_slock_);
        it = mtrack_map_.find(alloc_id);
        if (it != mtrack_map_.end()) {
            it->second->num_frees++;
            //HAL_TRACE_DEBUG("mem_free for id: {}, frees:{}",
                            //alloc_id, it->second->num_frees);
            if (it->second->num_frees == it->second->num_allocs) {
                // we can free this state now
                free_mem = true;
                mtrack_info = it->second;
                mtrack_map_.erase(it);
            }
        } else {
            // this can happen if mtrack is enabled on the fly and some memory
            // allocations happened before that and are now being freed
            SDK_TRACE_ERR("Freed mem %p with alloc id %u without mtrack info",
                          ptr, alloc_id);
        }
        SDK_SPINLOCK_UNLOCK(&mtrack_map_slock_);

        if (free_mem) {
            free(mtrack_info);
        }

        return;
    }

    typedef bool (*walk_cb_t)(void *ctxt, uint32_t alloc_id, mtrack_info_t *minfo);
    void walk(void *ctxt, walk_cb_t walk_cb) {
        mtrack_map_it_t     it;

        SDK_SPINLOCK_LOCK(&mtrack_map_slock_);
        for (it = mtrack_map_.begin(); it != mtrack_map_.end(); ++it) {
            if (!walk_cb(ctxt, it->first, it->second)) {
                // if callback returns false, stop iterating
                break;
            }
        }
        SDK_SPINLOCK_UNLOCK(&mtrack_map_slock_);
    }

private:
    mtrack_map_t      mtrack_map_;
    sdk_spinlock_t    mtrack_map_slock_;
    bool              enabled_;
};

extern sdk::utils::mem_mgr g_sdk_mem_mgr;
}    // namespace utils
}    // namespace sdk

extern sdk::utils::mem_mgr g_sdk_mem_mgr;
#endif    // __cplusplus
#endif    // __SDK_MTRACK_HPP__

