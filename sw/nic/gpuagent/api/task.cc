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
/// gpu entry handling
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/mem.hpp"
#include "nic/gpuagent/api/task.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/internal/aga_api_params.hpp"
#include "nic/gpuagent/api/gpu_state.hpp"
#include "nic/gpuagent/api/smi/smi_api.hpp"

namespace aga {

task *
task::factory(aga_task_spec_t *spec) {
    task *task_obj;

    task_obj = task_db()->alloc();
    if (task_obj) {
        new (task_obj) task();
    }
    return task_obj;
}

void
task::destroy(task *task_obj) {
    task_obj->~task();
    task_db()->free(task_obj);
}

sdk_ret_t
task::free(task *task_obj) {
    task_obj->~task();
    task_db()->free(task_obj);
    return SDK_RET_OK;
}

sdk_ret_t
task::delay_delete(void) {
    task::destroy(this);
    return SDK_RET_OK;
    // TODO: we can do delay delete later on
    //return delay_delete(AGA_OBJ_ID_TASK, this);
}

sdk_ret_t
task::handle_gpu_reset_task_(aga_gpu_reset_task_spec_t *spec) {
    sdk_ret_t ret;
    gpu_entry *gpu;

    for (uint32_t i = 0; i < spec->num_gpu; i++) {
        gpu = gpu_db()->find(&spec->gpu[i]);
        if (unlikely(gpu == NULL)) {
            AGA_TRACE_WARN("GPU {} not found, reset failed",
                           spec->gpu[i].str());
            return SDK_RET_ENTRY_NOT_FOUND;
        }
        AGA_TRACE_DEBUG("Handling GPU {} reset task, type {}",
                        spec->gpu[i].str(), spec->reset_type);
        // GPU found, perform the reset operation
        switch (spec->reset_type) {
        case AGA_GPU_RESET_TYPE_NONE:
            break;
        case AGA_GPU_RESET_TYPE_CLOCK:
            break;
        case AGA_GPU_RESET_TYPE_FAN:
            break;
        case AGA_GPU_RESET_TYPE_POWER_PROFILE:
            break;
        case AGA_GPU_RESET_TYPE_POWER_OVERDRIVE:
            break;
        case AGA_GPU_RESET_TYPE_XGMI_ERROR:
            break;
        case AGA_GPU_RESET_TYPE_PERF_DETERMINISM:
            break;
        case AGA_GPU_RESET_TYPE_COMPUTE_PARTITION:
            break;
        case AGA_GPU_RESET_TYPE_NPS_MODE:
            break;
        default:
            AGA_TRACE_ERR("GPU {} reset failure, unknown reset type {}",
                          spec->gpu[i].str(), spec->reset_type);
            return SDK_RET_INVALID_ARG;
        }
    }
    ret = smi_gpu_reset(gpu->handle(), spec->reset_type);
    return ret;
}

typedef struct gpu_update_watch_cb_ctxt_s {
    aga_gpu_watch_db_t *watch_db;
} gpu_update_watch_cb_ctxt_t;

static inline bool
gpu_update_watch_cb (void *obj, void *ctxt)
{
    aga_gpu_watch_db_t *watch_db;
    gpu_entry *gpu = (gpu_entry *)obj;
    gpu_update_watch_cb_ctxt_t *cb_ctxt;

    cb_ctxt = (gpu_update_watch_cb_ctxt_t *)ctxt;
    watch_db = cb_ctxt->watch_db;

    gpu->update_stats(&watch_db->watch_info[gpu->id()]);
    return false;
}

sdk_ret_t
task::handle_gpu_update_watch_db_task_(aga_gpu_watch_db_t *watch_db) {
    gpu_update_watch_cb_ctxt_t ctxt;

    ctxt.watch_db = watch_db;
    // walk over all gpus and update stats
    gpu_db()->walk_handle_db(gpu_update_watch_cb, &ctxt);
    return SDK_RET_OK;
}

sdk_ret_t
task::handle_gpu_watch_subscriber_add_task_(
          aga_gpu_watch_subscriber_spec_t *spec) {
    gpu_watch_entry *entry;

    // loop through all gpu watch objects and increment the
    // subscriber ref count
    for (uint32_t i = 0; i < spec->num_gpu_watch_ids; i++) {
        entry = gpu_watch_db()->find(&spec->gpu_watch_ids[i]);
        if (entry) {
            entry->subscriber_add();
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
task::handle_gpu_watch_subscriber_del_task_(
          aga_gpu_watch_subscriber_spec_t *spec) {
    gpu_watch_entry *entry;

    // loop through all gpu watch objects and decrement the
    // subscriber ref count
    for (uint32_t i = 0; i < spec->num_gpu_watch_ids; i++) {
        entry = gpu_watch_db()->find(&spec->gpu_watch_ids[i]);
        if (entry) {
            entry->subscriber_del();
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
task::create_handler(api_params_base *api_params) {
    sdk_ret_t ret = SDK_RET_OK;
    aga_task_spec_t *spec = AGA_TASK_SPEC(api_params);

    switch (spec->task) {
    case AGA_TASK_GPU_RESET:
        ret = handle_gpu_reset_task_(&spec->gpu_reset_task_spec);
        break;
    case AGA_TASK_GPU_WATCH_DB_UPDATE:
        ret = handle_gpu_update_watch_db_task_(&spec->watch_db);
        break;
    case AGA_TASK_GPU_WATCH_SUBSCRIBE_ADD:
        ret = handle_gpu_watch_subscriber_add_task_(&spec->subscriber_spec);
        break;
    case AGA_TASK_GPU_WATCH_SUBSCRIBE_DEL:
        ret = handle_gpu_watch_subscriber_del_task_(&spec->subscriber_spec);
        break;
    default:
        AGA_TRACE_ERR("Unknown task {}", spec->task);
        ret = SDK_RET_INVALID_ARG;
        break;
    }
    return ret;
}

}    // namespace aga
