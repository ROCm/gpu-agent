
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
/// API handlers for all supported objects
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/core/api.h"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/core/api_base.hpp"
#include "nic/gpuagent/core/api_ctxt.hpp"
#include "nic/gpuagent/core/api_cb.hpp"
#include "nic/gpuagent/api/gpu.hpp"
#include "nic/gpuagent/api/gpu_watch.hpp"
#include "nic/gpuagent/api/task.hpp"

namespace aga {

static inline sdk_ret_t
aga_invalid_op_cb (api_base *api_obj, api_params_base *api_params)
{
    return SDK_RET_INVALID_OP;
}

aga_obj_api_cb_t g_api_obj_cb[] = {
    // AGA_OBJ_ID_NONE
    {
        NULL,
        NULL,
        NULL,
    },
    // AGA_OBJ_ID_GPU
    {
        aga_gpu_create_cb,
        aga_gpu_delete_cb,
        aga_gpu_update_cb,
    },
    // AGA_OBJ_ID_TASK
    {
        aga_task_create_cb,
        aga_invalid_op_cb,
        aga_invalid_op_cb,
    },
    // AGA_OBJ_ID_GPU_WATCH
    {
        aga_gpu_watch_create_cb,
        aga_gpu_watch_delete_cb,
        aga_gpu_watch_update_cb,
    }
};

}    // namespace aga
