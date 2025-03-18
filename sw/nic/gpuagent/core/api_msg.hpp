
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
/// API message definition and helper functions
///
//----------------------------------------------------------------------------

#ifndef __AGA_CORE_API_MSG_HPP__
#define __AGA_CORE_API_MSG_HPP__

#include <vector>
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/gpuagent/include/globals.hpp"
#include "nic/gpuagent/core/ipc_msg.hpp"
#include "nic/gpuagent/core/api_ctxt.hpp"

using std::vector;

namespace aga {

/// async response callback type
/// \param[in] status    status/result of the API operation
/// \param[in] cookie    opaque cookie passed back to the client to correlate
///                      async response to the original request
typedef void (*aga_async_rsp_cb_t)(sdk_ret_t status, const void *cookie);

/// API message containing API associated information
typedef struct aga_api_cfg_req_s {
    /// process this batch synchronously or asynchronously
    bool async;
    /// the callback to call once this batch finishes processing
    aga_async_rsp_cb_t response_cb;
    /// cookie is something that client uses to correlate
    /// API batch request and response in case batch is requested
    /// to be processed asynchronously
    void *cookie;
    /// list of api calls to process
    // NOTE:
    // currently only one API is supported as there is no batching requirement
    vector<api_ctxt_t *> apis;
} aga_api_cfg_req_t;

/// \brief    IPC message sent to API thread for API processing
typedef struct api_msg_s {
    aga_ipc_msg_id_t msg_id;
    /// API batch request contents
    aga_api_cfg_req_t req;
    /// API processing result
    sdk_ret_t status;
} api_msg_t;

/// \brief    wrapper function to process all API calls
/// \param[in]  msg_id      unique message identifier
/// \param[in]  api_ctxt    api specific context to be added to batch or
///                         processed individually if batch context in invalid
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t process_api(api_ctxt_t *api_ctxt);

}    // namespace aga

using aga::api_msg_t;

#endif    // __AGA_CORE_API_MSG_HPP__
