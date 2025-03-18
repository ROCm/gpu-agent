
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
/// API params specific APIs
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/core/api.h"
#include "nic/gpuagent/core/api_params.hpp"
#include "nic/gpuagent/api/mem.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/internal/aga_api_params.hpp"

namespace aga {

api_params_base *
api_params_base::factory(void) {
    void *mem;

    mem = g_aga_state.aga_api_params_alloc();
    new (mem) aga_api_params();
    return (api_params_base *)mem;
}

void
api_params_base::destroy(obj_id_t obj_id, api_op_t api_op,
                         api_params_base *api_params) {
    //aga_api_params *params;

    //params = (aga_api_params *)api_params;
    switch (obj_id) {
        default:
            break;
    }
    g_aga_state.aga_api_params_free(api_params);
}

const aga_obj_key_t&
aga_api_params::obj_key(obj_id_t obj_id, api_op_t api_op) {
    // for delete operations, key is stored in one for all objects
    if (api_op == API_OP_DELETE) {
        return key;
    }
    // for create & update, look into the object
    switch (obj_id) {
    case AGA_OBJ_ID_GPU:
        return gpu_spec.key;
    case AGA_OBJ_ID_TASK:
        return task_spec.key;
    case AGA_OBJ_ID_GPU_WATCH:
        return gpu_watch_spec.key;
    default:
        AGA_TRACE_ERR("Unknown object id {}", obj_id);
        break;
    }
    SDK_ASSERT(FALSE);
    return k_aga_obj_key_invalid;
}

}    // namespace aga
