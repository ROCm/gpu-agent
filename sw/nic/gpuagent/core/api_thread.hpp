
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
/// API thread specific helper functions
///
//----------------------------------------------------------------------------

#ifndef __AGA_CORE_API_THREAD_HPP__
#define __AGA_CORE_API_THREAD_HPP__

#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/gpuagent/core/state_base.hpp"

namespace aga {

/// \defgroup AGA_API_THREAD API thread helper functions/wrappers
/// @{

/// \brief    callback function invoked during api thread initialization
/// \param[in] ctxt    opaque context
void api_thread_init_fn(void *ctxt);

/// \brief    callback function invoked during api thread exit
/// \param[in] ctxt    opaque context
void api_thread_exit_fn(void *ctxt);

/// \brief    callback function invoked to process events received by API thread
/// \param[in] msg     message carrying the event information
/// \param[in] ctxt    opaque context
void api_thread_event_cb(void *msg, void *ctx);

/// \brief    return true if API thread is ready to receive events
/// \return true or false based on whether API thread is ready or not
bool is_api_thread_ready(void);

/// \brief    spawn the API thread
/// \param[in] state    agent state instance
/// \return SDK_RET_OK or error code in case of failure
sdk_ret_t spawn_api_thread(state_base *state);

/// \@}

}    // namespace aga

#endif    // __AGA_CORE_API_THREAD_HPP__
