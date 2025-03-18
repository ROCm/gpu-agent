
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
/// API processing framework/engine functionality
///
//----------------------------------------------------------------------------

#ifndef __AGA_CORE_API_ENGINER_HPP__
#define __AGA_CORE_API_ENGINER_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/core/api_ctxt.hpp"
#include "nic/gpuagent/core/state_base.hpp"

using sdk::ipc::ipc_msg_ptr;

namespace aga {

/// \brief    initialize the API engine
/// \param[in] state    pointer to application specific state (base) class
///                     instance
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t api_engine_init(state_base *state);

/// \brief    handle the incoming API message
/// \param[in] api_msg    API msg received
/// \param[in] ipc_msg    IPC msg received that contains the API context(s)
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t api_msg_handle_cb(api_msg_t *api_msg, sdk::ipc::ipc_msg_ptr ipc_msg);

}    // namespace aga

#endif    //  __AGA_CORE_API_ENGINER_HPP__
