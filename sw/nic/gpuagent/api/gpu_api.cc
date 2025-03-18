
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
/// this file implements gpu CRUD APIs
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/assert.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/core/api.h"
#include "nic/gpuagent/core/api_ctxt.hpp"
#include "nic/gpuagent/core/api_msg.hpp"
#include "nic/gpuagent/api/include/base.hpp"
#include "nic/gpuagent/api/internal/aga_api_params.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"
#include "nic/gpuagent/api/gpu.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/smi/smi_api.hpp"

static sdk_ret_t
aga_gpu_api_handle (api_op_t op, aga_obj_key_t *key, aga_gpu_spec_t *spec)
{
    sdk_ret_t ret;
    api_ctxt_t *api_ctxt;

    if ((ret = obj_api_validate(op, key, spec)) != SDK_RET_OK) {
        // basic API validation failure
        return ret;
    }
    // allocate API context
    api_ctxt = aga::api_ctxt_alloc(AGA_OBJ_ID_GPU, op);
    if (likely(api_ctxt != NULL)) {
        if (op == API_OP_DELETE) {
            AGA_API_PARAMS_FROM_API_CTXT(api_ctxt)->key = *key;
        } else {
            AGA_API_PARAMS_FROM_API_CTXT(api_ctxt)->gpu_spec = *spec;
        }
        return process_api(api_ctxt);
    }
    return SDK_RET_OOM;
}

static inline sdk_ret_t
aga_gpu_entry_find (_In_ aga_obj_key_t *key, _Out_ gpu_entry **gpu)
{
    gpu_entry *entry;

    *gpu = NULL;
    entry = gpu_db()->find(key);
    if (entry == NULL) {
        return SDK_RET_ENTRY_NOT_FOUND;
    }
    if (entry->in_use()) {
        return SDK_RET_IN_USE;
    }
    *gpu = entry;
    return SDK_RET_OK;
}

sdk_ret_t
aga_gpu_create (_In_ aga_gpu_spec_t *spec)
{
    return aga_gpu_api_handle(API_OP_CREATE, NULL, spec);
}

sdk_ret_t
aga_gpu_read (_In_ aga_obj_key_t *key, _Out_ aga_gpu_info_t *info)
{
    sdk_ret_t ret;
    gpu_entry *entry;

    if (unlikely((key == NULL) || (info == NULL))) {
        return SDK_RET_INVALID_ARG;
    }
    ret = aga_gpu_entry_find(key, &entry);
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }
    return entry->read(info);
}

typedef struct aga_gpu_read_args_s {
    void *ctxt;
    gpu_read_cb_t cb;
} aga_gpu_read_args_t;

bool
aga_gpu_info_from_entry (void *entry, void *ctxt)
{
    aga_gpu_info_t info;
    gpu_entry *gpu = (gpu_entry *)entry;
    aga_gpu_read_args_t *args = (aga_gpu_read_args_t *)ctxt;

    if (gpu->in_use()) {
        // some API operation is in progress on this object, skip it
        return false;
    }
    memset(&info, 0, sizeof(aga_gpu_info_t));
    // call entry read
    gpu->read(&info);
    // call cb on info
    args->cb(&info, args->ctxt);
    return false;
}

sdk_ret_t
aga_gpu_read_all (gpu_read_cb_t gpu_read_cb, void *ctxt)
{
    aga_gpu_read_args_t args = { 0 };

    args.ctxt = ctxt;
    args.cb = gpu_read_cb;
    return gpu_db()->walk(aga_gpu_info_from_entry, &args);
}

sdk_ret_t
aga_gpu_compute_partition_read (_In_ aga_obj_key_t *key,
                                _Out_ aga_gpu_compute_partition_info_t *info)
{
    sdk_ret_t ret;
    gpu_entry *entry;

    if (unlikely((key == NULL) || (info == NULL))) {
        return SDK_RET_INVALID_ARG;
    }
    ret = aga_gpu_entry_find(key, &entry);
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }
    // skip non-partitioned GPUs
    if (!entry->is_parent_gpu()) {
        return SDK_RET_ENTRY_NOT_FOUND;
    }
    // fill compute partition info
    info->physical_gpu = *key;
    info->compute_partition_type = entry->compute_partition_type();
    auto child_gpus = entry->child_gpus();
    info->num_gpu_partition = child_gpus.size();
    for (uint32_t i = 0; i < child_gpus.size(); i++) {
        info->gpu_partition[i] = child_gpus[i];
    }
    return SDK_RET_OK;
}

typedef struct aga_gpu_compute_partition_read_args_s {
    void *ctxt;
    gpu_compute_partition_read_cb_t cb;
} aga_gpu_compute_partition_read_args_t;

bool
aga_gpu_compute_partition_info_from_entry (void *entry, void *ctxt)
{
    gpu_entry *gpu = (gpu_entry *)entry;
    aga_gpu_compute_partition_info_t info;
    aga_gpu_compute_partition_read_args_t *args =
        (aga_gpu_compute_partition_read_args_t *)ctxt;

    if (gpu->in_use()) {
        // some API operation is in progress on this object, skip it
        return false;
    }
    // skip non-partitioned GPUs
    if (!gpu->is_parent_gpu()) {
        return false;
    }
    memset(&info, 0, sizeof(aga_gpu_compute_partition_info_t));
    // fill compute partition info
    info.physical_gpu = gpu->key();
    info.compute_partition_type = gpu->compute_partition_type();
    auto child_gpus = gpu->child_gpus();
    info.num_gpu_partition = child_gpus.size();
    for (uint32_t i = 0; i < child_gpus.size(); i++) {
        info.gpu_partition[i] = child_gpus[i];
    }
    // call cb on info
    args->cb(&info, args->ctxt);
    return false;
}

sdk_ret_t
aga_gpu_compute_partition_read_all (gpu_compute_partition_read_cb_t read_cb,
                                    void *ctxt)
{
    aga_gpu_compute_partition_read_args_t args = { 0 };

    args.ctxt = ctxt;
    args.cb = read_cb;
    return gpu_db()->walk(aga_gpu_compute_partition_info_from_entry, &args);
}

typedef struct aga_gpu_bad_page_read_args_s {
    void *ctxt;
    gpu_bad_page_read_cb_t cb;
} aga_gpu_bad_page_read_args_t;

bool
aga_gpu_bad_page_info_from_entry (void *entry, void *ctxt)
{
    sdk_ret_t ret;
    uint32_t num_bad_pages = 0;
    aga_gpu_bad_page_record_t *records;
    gpu_entry *gpu = (gpu_entry *)entry;
    aga_gpu_bad_page_read_args_t *args = (aga_gpu_bad_page_read_args_t *)ctxt;

    if (gpu->in_use()) {
        // some API operation is in progress on this object, skip it
        return false;
    }
    if (gpu->is_parent_gpu()) {
        // partition parent GPU objects can be skipped
        return false;
    }
    // first get number of bad page records for GPU
    ret = smi_gpu_get_bad_page_count(gpu, &num_bad_pages);
    if ((ret != SDK_RET_OK) || num_bad_pages == 0) {
        goto done;
    }
    // allocate memory for the records
    records = (aga_gpu_bad_page_record_t *)
                  calloc(num_bad_pages, sizeof(aga_gpu_bad_page_record_t));
    if (records == NULL) {
        goto done;
    }
    // get bad page records
    ret = smi_gpu_get_bad_page_records(gpu, num_bad_pages, records);
    // call cb on info
    args->cb(num_bad_pages, records, args->ctxt);
    // free memory
    free(records);
done:
    return false;
}

sdk_ret_t
aga_gpu_bad_page_read (aga_obj_key_t *key, gpu_bad_page_read_cb_t cb,
                       void *ctxt)
{
    gpu_entry *gpu;
    aga_gpu_bad_page_read_args_t args = { 0 };

    args.ctxt = ctxt;
    args.cb = cb;
    if (*key == k_aga_obj_key_invalid) {
        return gpu_db()->walk(aga_gpu_bad_page_info_from_entry, &args);
    } else {
        gpu = gpu_db()->find(key);
        if (gpu) {
            return aga_gpu_bad_page_info_from_entry(gpu, &args);
        }
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

typedef struct aga_gpu_topology_read_args_s {
    void *ctxt;
    device_topology_read_cb_t cb;
} aga_gpu_topology_read_args_t;

bool
aga_gpu_topology_info_from_entry (void *entry, void *ctxt)
{
    aga_device_topology_info_t info;
    gpu_entry *gpu = (gpu_entry *)entry;
    aga_gpu_topology_read_args_t *args = (aga_gpu_topology_read_args_t *)ctxt;

    if (gpu->in_use()) {
        // some API operation is in progress on this object, skip it
        return false;
    }
    if (gpu->is_parent_gpu()) {
        // partition parent GPU objects can be skipped
        return false;
    }
    memset(&info, 0, sizeof(aga_device_topology_info_t));
    // call entry read
    gpu->read_topology(&info);
    // call cb on info
    args->cb(&info, args->ctxt);
    return false;
}

sdk_ret_t
aga_gpu_read_topology_all (device_topology_read_cb_t gpu_topology_read_cb,
                           void *ctxt)
{
    aga_gpu_topology_read_args_t args = { 0 };

    args.ctxt = ctxt;
    args.cb = gpu_topology_read_cb;
    return gpu_db()->walk(aga_gpu_topology_info_from_entry, &args);
}

sdk_ret_t
aga_gpu_update (_In_ aga_gpu_spec_t *spec)
{
    return aga_gpu_api_handle(API_OP_UPDATE, NULL, spec);
}

sdk_ret_t
aga_gpu_delete (_In_ aga_obj_key_t *key)
{
    return aga_gpu_api_handle(API_OP_DELETE, key, NULL);
}
