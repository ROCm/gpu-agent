
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
/// gpu watch entry handling
///
//----------------------------------------------------------------------------

#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/mem.hpp"
#include "nic/gpuagent/api/gpu_watch.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/internal/aga_api_params.hpp"

namespace aga {

gpu_watch_entry::gpu_watch_entry() {
}

gpu_watch_entry::~gpu_watch_entry() {
}

gpu_watch_entry *
gpu_watch_entry::factory(aga_gpu_watch_spec_t *spec) {
    gpu_watch_entry *entry;

    entry = gpu_watch_db()->alloc();
    if (entry) {
        new (entry) gpu_watch_entry();
    }
    return entry;
}

void
gpu_watch_entry::destroy(gpu_watch_entry *entry) {
    entry->~gpu_watch_entry();
    gpu_watch_db()->free(entry);
}

sdk_ret_t
gpu_watch_entry::free(gpu_watch_entry *entry) {
    entry->~gpu_watch_entry();
    gpu_watch_db()->free(entry);
    return SDK_RET_OK;
}

sdk_ret_t
gpu_watch_entry::add_to_db(void) {
    return gpu_watch_db()->insert(this);
}

sdk_ret_t
gpu_watch_entry::del_from_db(void) {
    if (gpu_watch_db()->remove(this)) {
        return SDK_RET_OK;
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

sdk_ret_t
gpu_watch_entry::delay_delete(void) {
    return aga::delay_delete(AGA_OBJ_ID_GPU_WATCH, this);
}

sdk_ret_t
gpu_watch_entry::create_handler(api_params_base *api_params) {
    aga_gpu_watch_spec_t *spec = AGA_GPU_WATCH_SPEC(api_params);

    for (uint8_t i = 0; i < spec->num_gpu; i++) {
        auto gpu = gpu_db()->find(&spec->gpu[i]);
        if (unlikely(gpu == NULL)) {
            AGA_TRACE_ERR("Failed to create GPU watch {}, GPU {} not found",
                          spec->key.str(), spec->gpu[i].str());
            return SDK_RET_INVALID_ARG;
        }
        gpu->gpu_watch_add();
    }
    key_ = spec->key;
    memcpy(&spec_, spec, sizeof(aga_gpu_watch_spec_t));
    return SDK_RET_OK;
}

sdk_ret_t
gpu_watch_entry::delete_handler(api_params_base *api_params) {
    if (unlikely(num_subscriber_ > 0)) {
        // gpu watch has active subscribers
        AGA_TRACE_ERR("Failed to delete GPU watch {}, there are currently {} "
                      "active subscribers for this GPU watch", key_.str(),
                      num_subscriber_);
        return SDK_RET_IN_USE;
    }
    for (uint8_t i = 0; i < spec_.num_gpu; i++) {
        auto gpu = gpu_db()->find(&spec_.gpu[i]);
        if (gpu) {
            gpu->gpu_watch_dec();
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
gpu_watch_entry::update_handler(api_params_base *api_params) {
    return SDK_RET_INVALID_OP;
}

void
gpu_watch_entry::fill_spec_(aga_gpu_watch_spec_t *spec) {
    memcpy(spec, &spec_, sizeof(aga_gpu_watch_spec_t));
}

void
gpu_watch_entry::fill_status_(aga_gpu_watch_status_t *status) {
    status->num_subscribers = num_subscriber_;
}

void
gpu_watch_entry::fill_stats_(aga_gpu_watch_stats_t *stats) {
    gpu_entry *entry;

    stats->num_gpu = spec_.num_gpu;
    for (auto gid = 0; gid < spec_.num_gpu; gid++) {
        entry = gpu_db()->find(&spec_.gpu[gid]);
        if (entry == NULL) {
            // should not happen unless gpu uuid is unknown
            continue;
        }
        stats->gpu_watch_attr[gid].gpu = spec_.gpu[gid];
        stats->gpu_watch_attr[gid].num_attrs = spec_.num_attrs;
        for (auto i = 0; i < spec_.num_attrs; i++) {
            stats->gpu_watch_attr[gid].attr[i].id = spec_.attr_id[i];
        }
        entry->fill_gpu_watch_stats(&stats->gpu_watch_attr[gid]);
    }
}

sdk_ret_t
gpu_watch_entry::read(aga_gpu_watch_info_t *info) {
    fill_spec_(&info->spec);
    fill_status_(&info->status);
    fill_stats_(&info->stats);
    return SDK_RET_OK;
}

}    // namespace aga
