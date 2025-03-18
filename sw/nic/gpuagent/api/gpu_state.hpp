
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
/// GPU state handling
///
//----------------------------------------------------------------------------

#ifndef __AGA_GPU_STATE_HPP__
#define __AGA_GPU_STATE_HPP__

#include <unordered_map>
#include "nic/gpuagent/api/gpu.hpp"
#include "nic/gpuagent/core/state_base.hpp"
#include "nic/gpuagent/api/aga_state.hpp"

namespace aga {

/// \brief gpu entry map with handle as the key
typedef std::unordered_map<aga_gpu_handle_t, gpu_entry *> gpu_db_t;

/// \brief gpu entry map with uuid as the key
typedef std::unordered_map<aga_obj_key_t, gpu_entry *,
                           aga_obj_key_hash> gpu_key_db_t;

/// \defgroup AGA_GPU_STATE - GPU state functionality
/// \ingroup AGA
/// @{

/// \brief state maintained for GPUs
class gpu_state : public state_base {
public:
    /// \brief constructor
    gpu_state() {}

    /// \brief destructor
    ~gpu_state() {}

    /// \brief  allocate memory required for a GPU object
    // \return pointer to the allocated GPU object, NULL if no memory
    gpu_entry *alloc(void);

    /// \brief     insert given GPU instance into the db indexed by its key
    /// \param[in] gpu GPU entry to be added to the db
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t insert(gpu_entry *gpu);

    /// \brief     insert given GPU instance to db indexed by its handle
    /// \param[in] gpu GPU entry to be added to the db
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t insert_in_handle_db(gpu_entry *gpu);

    /// \brief     remove given GPU instance from all dbs
    /// \param[in] gpu gpu entry to be deleted from the db
    /// \return    pointer to the removed gpu instance or NULL, if not found
    gpu_entry *remove(gpu_entry *gpu);

    /// \brief     free gpu instance
    /// \param[in] gpu pointer to the allocated gpu instance
    void free(gpu_entry *gpu);

    /// \brief     lookup a gpu in database given the key
    /// \param[in] key key of the gpu object
    /// \return    pointer to the gpu instance found or NULL
    gpu_entry *find(aga_obj_key_t *key) const;

    /// \brief     lookup a gpu in database given the handle
    /// \param[in] handle     handle of the GPU
    /// \return    pointer to the gpu instance found or NULL
    gpu_entry *find(aga_gpu_handle_t handle) const;

    /// \brief API to walk all the db elements
    /// \param[in] walk_cb    callback to be invoked for every node
    /// \param[in] ctxt       opaque context passed back to the callback
    /// \return   SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t walk(state_walk_cb_t walk_cb, void *ctxt) override;

    /// \brief API to walk all the handle db elements without taking a lock; use
    ///        only to access the key fields in the GPU entry
    /// \param[in] walk_cb    callback to be invoked for every node
    /// \param[in] ctxt       opaque context passed back to the callback
    /// \return   SDK_RET_OK on success, failure status code on error
    sdk_ret_t walk_handle_db(state_walk_cb_t walk_cb, void *ctxt);

    friend void delay_delete_cb(void *timer, uint32_t obj_id, void *elem);

private:
    /// gpu_entry is friend of gpu_state
    friend class gpu_entry;

private:
    /// map to store GPU objects keyed by uuid
    gpu_key_db_t gpu_key_db_;
    /// map to store GPU objects keyed by handle
    gpu_db_t gpu_db_;
};

/// \brief   return GPU object given the key
/// \param[in] key   GPU objet key
/// \return    GPU object instance corresponding to the key
static inline gpu_entry *
gpu_find (aga_obj_key_t *key)
{
    return (gpu_entry *)api_base::find_obj(AGA_OBJ_ID_GPU, key);
}

/// \@}

}    // namespace aga

using aga::gpu_state;

#endif    // __AGA_GPU_STATE_HPP__
