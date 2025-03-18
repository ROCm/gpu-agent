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
/// API layer init/cleanup routines
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/assert.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/core/api_thread.hpp"
#include "nic/gpuagent/api/include/aga_init.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/smi/smi_api.hpp"

sdk_ret_t
aga_api_init (aga_api_init_params_t *init_params)
{
    sdk_ret_t ret;

    // initialize the internal state
    ret = aga::g_aga_state.init();
    SDK_ASSERT(ret() == SDK_RET_OK);
    // spawn the API thread
    aga::spawn_api_thread(&aga::g_aga_state);
    // wait for the API thread to be ready
    while (!aga::is_api_thread_ready()) {
        sched_yield();
    }
    // initialize rocm-smi library
    ret = aga::smi_init(init_params);
    if (unlikely(ret != SDK_RET_OK)) {
        AGA_TRACE_ERR("Failed to initialize smi library, err {}", ret());
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

sdk_ret_t
aga_api_teardown (void)
{
    return SDK_RET_OK;
}
