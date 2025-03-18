
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
/// gpu watch state handling
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/mem.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/mem.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/gpu_watch_state.hpp"

namespace aga {

/// \defgroup AGA_GPU_WATCH_STATE - gpu watch database functionality
/// \ingroup AGA_GPU_WATCH
/// \@{

gpu_watch_entry *
gpu_watch_state::alloc(void) {
    return (gpu_watch_entry *)calloc(1, sizeof(gpu_watch_entry));
}

sdk_ret_t
gpu_watch_state::insert(gpu_watch_entry *entry) {
    sdk_ret_t ret;

    // take lock before inserting entry
    std::lock_guard<std::mutex> lock(gpu_watch_key_lock_);
    auto it = gpu_watch_key_db_.find(entry->key_);
    if (it != gpu_watch_key_db_.end()) {
        AGA_STATE_CNTR_INSERT_ERR_INC();
        ret = SDK_RET_ENTRY_EXISTS;
    } else {
        gpu_watch_key_db_[entry->key_] = entry;
        AGA_STATE_CNTR_INSERT_OK_INC();
        AGA_STATE_CNTR_NUM_ELEMS_INC();
        ret = SDK_RET_OK;
    }
    return ret;
}

gpu_watch_entry *
gpu_watch_state::remove(gpu_watch_entry *entry) {
    void *rv;

    // take lock before removing entry
    std::lock_guard<std::mutex> lock(gpu_watch_key_lock_);
    auto it = gpu_watch_key_db_.find(entry->key_);
    if (it == gpu_watch_key_db_.end()) {
        AGA_STATE_CNTR_REMOVE_ERR_INC();
        rv = NULL;
    } else {
        rv = it->second;
        gpu_watch_key_db_.erase(entry->key_);
        AGA_STATE_CNTR_REMOVE_OK_INC();
        AGA_STATE_CNTR_NUM_ELEMS_DEC();
    }
    return (gpu_watch_entry *)rv;
}

void
gpu_watch_state::free(gpu_watch_entry *entry) {
    ::free(entry);
}

gpu_watch_entry *
gpu_watch_state::find(aga_obj_key_t *key) const {
    // take lock before searching for gpu watch entries
    std::lock_guard<std::mutex> lock(gpu_watch_key_lock_);
    auto it = gpu_watch_key_db_.find(*key);
    if (it != gpu_watch_key_db_.end()) {
        return it->second;
    }
    return NULL;
}

sdk_ret_t
gpu_watch_state::walk(state_walk_cb_t walk_cb, void *ctxt) {
    bool stop;

    // take lock before walking gpu watch entries
    std::lock_guard<std::mutex> lock(gpu_watch_key_lock_);
    for (auto iter = gpu_watch_key_db_.begin(); iter != gpu_watch_key_db_.end();
         ++iter) {
        stop = walk_cb(iter->second, ctxt);
        if (stop) {
            break;
        }
    }
    return SDK_RET_OK;
}

/// \@}    // end of AGA_GPU_WATCH_STATE

}    // namespace aga
