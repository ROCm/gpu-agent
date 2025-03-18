
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
/// GPU watch state handling
///
//----------------------------------------------------------------------------

#ifndef __AGA_GPU_WATCH_STATE_HPP__
#define __AGA_GPU_WATCH_STATE_HPP__

#include "nic/gpuagent/api/gpu_watch.hpp"
#include "nic/gpuagent/core/state_base.hpp"
#include "nic/gpuagent/api/aga_state.hpp"

/// \defgroup AGA_GPU_WATCH_STATE - GPU state functionality
/// \ingroup AGA
/// @{

namespace aga {

/// \brief gpu watch entry map with uuid as the key
typedef std::unordered_map<aga_obj_key_t, gpu_watch_entry *,
                           aga_obj_key_hash> gpu_watch_key_db_t;

/// \brief state maintained for GPUs
class gpu_watch_state : public state_base {
public:
    /// \brief constructor
    gpu_watch_state() {}

    /// \brief destructor
    ~gpu_watch_state() {}

    /// \brief  allocate memory required for a gpu watch object
    // \return pointer to the allocated gpu watch object, null if no memory
    gpu_watch_entry *alloc(void);

    /// \brief     insert given gpu watch  instance into the db
    /// \param[in] gpu_watch entry to be added to the db
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t insert(gpu_watch_entry *gpu_watch);

    /// \brief     remove the given instance of gpu watch object from db
    /// \param[in] gpu_watch  entry to be deleted from the db
    /// \return    pointer to the removed gpu watch instance or NULL,
    ///            if not found
    gpu_watch_entry *remove(gpu_watch_entry *gpu_watch);

    /// \brief     free gpu watch instance
    /// \param[in] gpu_watch    pointer to the allocated gpu watch instance
    void free(gpu_watch_entry *gpu_watch);

    /// \brief     lookup a gpu in database given the key
    /// \param[in] key key of the gpu watch object
    /// \return    pointer to the gpu watch instance found or NULL
    gpu_watch_entry *find(aga_obj_key_t *key) const;

    /// \brief API to walk all the db elements
    /// \param[in] walk_cb    callback to be invoked for every node
    /// \param[in] ctxt       opaque context passed back to the callback
    /// \return   SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t walk(state_walk_cb_t walk_cb, void *ctxt) override;

    friend void delay_delete_cb(void *timer, uint32_t obj_id, void *elem);

private:
    /// gpu_watch_entry is friend of gpu_watch_state
    friend class gpu_watch_entry;

private:
    /// map to store GPU watch objects keyed by uuid
    gpu_watch_key_db_t gpu_watch_key_db_;
    /// mutex to be used when adding/deleting/walking GPU watch key db
    mutable std::mutex gpu_watch_key_lock_;
};

/// \brief   return gpu watch object given the key
/// \param[in] key   gpu watch objet key
/// \return    gpu watch object instance corresponding to the key
static inline gpu_watch_entry *
gpu_watch_find (aga_obj_key_t *key)
{
    return (gpu_watch_entry *)api_base::find_obj(AGA_OBJ_ID_GPU_WATCH, key);
}

/// \@}

}    // namespace aga

using aga::gpu_watch_state;

#endif    // __AGA_GPU_WATCH_STATE_HPP__
