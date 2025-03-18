
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
/// task spec, status, stats and APIs
///
//----------------------------------------------------------------------------

#ifndef __API_INCLUDE_AGA_TASK_HPP__
#define __API_INCLUDE_AGA_TASK_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/api/internal/aga_gpu_watch.hpp"

/// \brief    type of GPU reset
typedef enum aga_gpu_reset_type_e {
    // if no specific reset is request, GPU device will be reset
    AGA_GPU_RESET_TYPE_NONE = 0,
    // reset clocks
    AGA_GPU_RESET_TYPE_CLOCK,
    // reset fans
    AGA_GPU_RESET_TYPE_FAN,
    // reset power profile
    AGA_GPU_RESET_TYPE_POWER_PROFILE,
    // reset power overdrive
    AGA_GPU_RESET_TYPE_POWER_OVERDRIVE,
    // reset XGMI error status
    AGA_GPU_RESET_TYPE_XGMI_ERROR,
    // reset/disable performance determinism
    AGA_GPU_RESET_TYPE_PERF_DETERMINISM,
    // reset/boot compute partition
    AGA_GPU_RESET_TYPE_COMPUTE_PARTITION,
    // reset to boot NPS mode state
    AGA_GPU_RESET_TYPE_NPS_MODE,
} aga_gpu_reset_type_t;

/// \brief    GPU reset task specification
typedef struct aga_gpu_reset_task_spec_s {
    // number of GPUs to perform the reset task on
    uint8_t num_gpu;
    // list of GPU identifiers
    aga_obj_key_t gpu[AGA_MAX_GPU];
    // type of the reset being requested
    aga_gpu_reset_type_t reset_type;
} aga_gpu_reset_task_spec_t;

/// \brief    GPU watch subscribe task specification
typedef struct aga_gpu_watch_subscriber_spec_s {
     /// number of gpu-watch ids
     uint8_t num_gpu_watch_ids;
     /// list of gpu watch uuids
     aga_obj_key_t gpu_watch_ids[AGA_MAX_GPU_WATCH];
} aga_gpu_watch_subscriber_spec_t;

/// \brief    supported tasks
typedef enum aga_task_e {
    /// invalid task
    AGA_TASK_NONE = 0,
    /// GPU reset task
    AGA_TASK_GPU_RESET,
    /// update GPU stats
    AGA_TASK_GPU_WATCH_DB_UPDATE,
    /// gpu watch subscribe add task
    AGA_TASK_GPU_WATCH_SUBSCRIBE_ADD,
    /// gpu watch subscribe delete task
    AGA_TASK_GPU_WATCH_SUBSCRIBE_DEL,
} aga_task_t;

/// \brief task specification
typedef struct aga_task_spec_s {
    /// unique identifier for task object
    aga_obj_key_t key;
    /// task to be executed
    aga_task_t task;
    /// task specific information
    union {
        /// GPU reset task
        aga_gpu_reset_task_spec_t gpu_reset_task_spec;
        /// GPU update watch db task
        aga_gpu_watch_db_t watch_db;
        /// gpu watch subscribe add/del tasks
        aga_gpu_watch_subscriber_spec_t subscriber_spec;
    };
} aga_task_spec_t;

/// \brief    create task
/// \param[in]  spec  task specification
/// \return     #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_task_create(_In_ aga_task_spec_t *spec);

#endif    // __API_INCLUDE_AGA_TASK_HPP__
