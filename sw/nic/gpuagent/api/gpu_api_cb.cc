
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
/// gpu object CRUD API handlers
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/core/api_base.hpp"
#include "nic/gpuagent/core/api_params.hpp"
#include "nic/gpuagent/api/internal/aga_api_params.hpp"

namespace aga {

sdk_ret_t
aga_gpu_create_cb (api_base *api_obj, api_params_base *api_params)
{
    return api_obj->create_handler(api_params);
}

sdk_ret_t
aga_gpu_delete_cb (api_base *api_obj, api_params_base *api_params)
{
    return api_obj->delete_handler(api_params);
}

sdk_ret_t
aga_gpu_update_cb (api_base *api_obj, api_params_base *api_params)
{
    return api_obj->update_handler(api_params);
}

}    // namespace aga
