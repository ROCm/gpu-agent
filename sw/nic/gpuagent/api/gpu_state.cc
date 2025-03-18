
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
/// gpu state handling
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/mem.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/mem.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/gpu_state.hpp"

namespace aga {

/// \defgroup AGA_GPU_STATE - gpu database functionality
/// \ingroup AGA_GPU
/// \@{

gpu_entry *
gpu_state::alloc(void) {
    return (gpu_entry *)calloc(1, sizeof(gpu_entry));
}

sdk_ret_t
gpu_state::insert(gpu_entry *gpu) {
    sdk_ret_t ret;

    AGA_TRACE_DEBUG("Inserting GPU {} in db", gpu->key_.str());
    auto it = gpu_key_db_.find(gpu->key_);
    if (it != gpu_key_db_.end()) {
        AGA_STATE_CNTR_INSERT_ERR_INC();
        ret = SDK_RET_ENTRY_EXISTS;
    } else {
        gpu_key_db_[gpu->key_] = gpu;
        AGA_STATE_CNTR_INSERT_OK_INC();
        AGA_STATE_CNTR_NUM_ELEMS_INC();
        ret = SDK_RET_OK;
    }
    return ret;
}

sdk_ret_t
gpu_state::insert_in_handle_db(gpu_entry *gpu) {
    sdk_ret_t ret;

    AGA_TRACE_DEBUG("Inserting GPU {} with handle {} in db", gpu->key_.str(),
                    gpu->handle_);
    gpu_db_[gpu->handle_] = gpu;
    return ret;
}

gpu_entry *
gpu_state::remove(gpu_entry *gpu) {
    void *rv;

    auto it = gpu_key_db_.find(gpu->key_);
    if (it == gpu_key_db_.end()) {
        AGA_STATE_CNTR_REMOVE_ERR_INC();
        rv = NULL;
    } else {
        rv = it->second;
        gpu_key_db_.erase(gpu->key_);
        // remove from handles db
        gpu_db_.erase(gpu->handle_);
        AGA_STATE_CNTR_REMOVE_OK_INC();
        AGA_STATE_CNTR_NUM_ELEMS_DEC();
    }
    return (gpu_entry *)rv;
}

void
gpu_state::free(gpu_entry *gpu) {
    free(gpu);
}

gpu_entry *
gpu_state::find(aga_obj_key_t *key) const {
    auto it = gpu_key_db_.find(*key);
    if (it != gpu_key_db_.end()) {
        return it->second;
    }
    return NULL;
}

gpu_entry *
gpu_state::find(aga_gpu_handle_t handle) const {
    auto it = gpu_db_.find(handle);
    if (it != gpu_db_.end()) {
        return it->second;
    }
    return NULL;
}

sdk_ret_t
gpu_state::walk(state_walk_cb_t walk_cb, void *ctxt) {
    bool stop;

    for (auto iter = gpu_key_db_.begin(); iter != gpu_key_db_.end(); ++iter) {
        stop = walk_cb(iter->second, ctxt);
        if (stop) {
            break;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
gpu_state::walk_handle_db(state_walk_cb_t walk_cb, void *ctxt) {
    bool stop;

    for (auto iter = gpu_db_.begin(); iter != gpu_db_.end(); ++iter) {
        stop = walk_cb(iter->second, ctxt);
        if (stop) {
            break;
        }
    }
    return SDK_RET_OK;
}

/// \@}    // end of AGA_GPU_STATE

}    // namespace aga
