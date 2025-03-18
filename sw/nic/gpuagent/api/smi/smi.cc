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
/// smi layer initialization
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/assert.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/smi/smi_state.hpp"

namespace aga {

sdk_ret_t
smi_init (aga_api_init_params_t *init_params)
{
    sdk_ret_t ret;

    // initialize rocm-smi library
    ret = g_smi_state.init(init_params);
    return ret;
}

}    // namespace aga
