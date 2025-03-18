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


//----------------------------------------------------------------------------
///
/// \file
/// This module defines a data store based on shared memory
///
//----------------------------------------------------------------------------

#ifndef __SDK_LIB_SHMSTORE_HPP__
#define __SDK_LIB_SHMSTORE_HPP__

#include <map>
#include "include/sdk/base.hpp"
#include "include/sdk/assert.hpp"
#include "include/sdk/platform.hpp"
#include "lib/shmmgr/shmmgr.hpp"

namespace sdk {
namespace lib {

// segment walk callback function
typedef shmmgr_seg_walk_cb_t shmstore_seg_walk_cb_t;
// store walk callback function
typedef void (*store_walk_cb_t)(void *ctx, const char *name);

class shmstore {
public:
    /// \brief factory method to create a store instance
    /// \return store instance
    static shmstore *factory(void);

    /// \brief destroy the store
    /// \param[in] store instance returned by factory
    static void destroy(shmstore *);

    /// \brief remove the store
    /// \param[in] created store instance
    static void remove(shmstore *);

    /// \brief remove the store for the given name
    /// \param[in] store name
    static void remove(const char *name);

    /// \brief create the store
    /// \param[in] name name of the store
    /// \param[in] size size of the store
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t create(const char *name, size_t size);

    /// \brief open a store for read
    /// \param[in] name name of the store
    /// \param[in] mode open mode
    /// \param[in] size size to passed if mode is SHM_OPEN_OR_CREATE
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t open(const char *name, enum shm_mode_e mode = SHM_OPEN_READ_ONLY,
                   size_t size = 0);

    /// \brief get the mode of the store
    /// \return shm mode type
    shm_mode_e mode(void) const { return mode_; }

    /// \brief get the size of the store
    /// \return store size
    size_t size(void) const;

    /// \brief check if a particular store exists
    /// \param[in] name store name
    /// \return true if store present, false if not
    static bool exists(const char *name);

    /// \brief check if a particular segment exists
    /// \param[in] name segment name
    /// \return true if segment present, false if not
    bool segment_exists(const char *name) const;

    /// \brief create a segment in the store which has been created.
    ///        cannot create segments on opened store.
    /// \param[in] name segment name
    /// \param[in] size segment size
    /// \param[in] alignment segment alignment
    /// \return valid segment pointer, null on failure
    void *create_segment(const char *name, size_t size, size_t alignment = 0);

    /// \brief open a segment in the given in store
    /// \param[in] name segment name
    /// \return valid segment pointer, null on failure
    void *open_segment(const char *name);

    /// \brief free a segment in the given in store
    /// \param[in] name segment name
    /// \return true if segment is freed, false if not
    bool free_segment(const char *name);

    /// \brief create or open a segment in the store which has been created.
    ///        cannot create segments on opened store.
    /// \param[in] name segment name
    /// \param[in] size segment size, valid for create case only
    /// \param[in] alignment segment alignment, valid for create case only
    /// \return valid segment pointer, null on failure
    void *create_or_open_segment(const char *name, size_t size,
                                 size_t alignment = 0);

    /// \brief get segment size
    /// \param[in] name segment name
    /// \return size of segment
    size_t segment_size(const char *name) const;

    /// \brief walk through all named segments in the store
    /// \param[in] callback context
    /// \param[in] callback function
    /// \return None
    void segment_walk(void *ctxt, shmstore_seg_walk_cb_t cb);

    /// \brief get store name
    /// \return store name
    const char *name(void) const {
        SDK_ASSERT(shmmgr_ != NULL);
        return shmmgr_->name();
    }

    /// \brief master store create. this is used where the application need to
    ///        create independent file stores from a contiguous virtual memory chunk
    /// \param[in] baseaddr master store virtual address
    /// \param[in] size master store size
    /// \return #SDK_RET_OK on success, failure status code on error
    /// \remark this should be invoked only once in the processes lifetime
    static sdk_ret_t master_store_create(void *baseaddr, size_t size);

    /// \brief master store open. this is used where the application need to
    ///        create independent file stores from a contiguous virtual memory chunk
    /// \param[in] baseaddr master store virtual address
    /// \param[in] size master store size
    /// \return #SDK_RET_OK on success, failure status code on error
    /// \remark this should be invoked only once in the processes lifetime
    static sdk_ret_t master_store_open(void *baseaddr, size_t size);

    /// \brief walk on all stores in a given path
    /// \param[in] path given path, ignored it the store is a master store
    /// \param[in] name_match  compares the store name with this, and callback
    ///                        will be invoked only if store name contains
    ///                        this substring
    /// \param[in] ctx callback context
    /// \param[in] cb callback
    static void walk(const char *path, const char *name_match,
                     void *ctx, store_walk_cb_t cb);

private:
    /// store mode
    sdk::lib::shm_mode_e mode_;
    /// shared memory manager
    sdk::lib::shmmgr *shmmgr_;
    /// master store virtual address
    static void *master_store_baseaddr_;
    /// master store virtual pool size
    static size_t master_store_size_;
    /// master store shm manager
    static shmmgr *master_store_shmmgr_;
    /// external buffer store name
    char ext_buff_store_name_[64];
private:
    sdk_ret_t file_init_(const char *name, size_t size, enum shm_mode_e mode);
    sdk_ret_t ext_buff_init_(const char *name, size_t size, enum shm_mode_e mode);
    void *segment_init_(const char *name, size_t size, bool create,
                        size_t alignment = 0);
};

}    // namespace lib
}    // namespace sdk

#endif    // __SDK_LIB_SHMSTORE_HPP__
