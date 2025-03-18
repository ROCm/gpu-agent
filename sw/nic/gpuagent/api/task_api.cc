
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
/// this file implements task CRUD APIs
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/core/api.h"
#include "nic/gpuagent/core/api_ctxt.hpp"
#include "nic/gpuagent/core/api_msg.hpp"
#include "nic/gpuagent/api/include/base.hpp"
#include "nic/gpuagent/api/internal/aga_api_params.hpp"
#include "nic/gpuagent/api/include/aga_task.hpp"

static sdk_ret_t
aga_task_api_handle (api_op_t op, aga_obj_key_t *key, aga_task_spec_t *spec)
{
    sdk_ret_t ret;
    api_ctxt_t *api_ctxt;

    if ((ret = obj_api_validate(op, key, spec)) != SDK_RET_OK) {
        // basic API validation failure
        return ret;
    }
    // allocate API context
    api_ctxt = aga::api_ctxt_alloc(AGA_OBJ_ID_TASK, op);
    if (likely(api_ctxt != NULL)) {
        if (op == API_OP_DELETE) {
            AGA_API_PARAMS_FROM_API_CTXT(api_ctxt)->key = *key;
        } else {
            AGA_API_PARAMS_FROM_API_CTXT(api_ctxt)->task_spec = *spec;
        }
        return process_api(api_ctxt);
    }
    return SDK_RET_OOM;
}

sdk_ret_t
aga_task_create (_In_ aga_task_spec_t *spec)
{
    return aga_task_api_handle(API_OP_CREATE, NULL, spec);
}
