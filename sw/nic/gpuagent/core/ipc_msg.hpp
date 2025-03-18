
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
/// IPC message identifiers
///
//----------------------------------------------------------------------------

#ifndef __AGA_CORE_IPC_MSG_HPP__
#define __AGA_CORE_IPC_MSG_HPP__

#include "nic/gpuagent/include/globals.hpp"

namespace aga {

typedef enum aga_ipc_msg_id_e {
    /// IPC msg to API thread
    AGA_IPC_MSG_ID_CFG = AGA_IPC_MSG_ID_MIN,
    /// IPC msg for event subscription
    AGA_IPC_MSG_ID_EVENT_SUBSCRIBE,
    /// IPC msg for GPU watch subscription
    AGA_IPC_MSG_ID_GPU_WATCH_SUBSCRIBE,
    /// IPC msg for event generation
    AGA_IPC_MSG_ID_EVENT_GEN,
} aga_ipc_msg_id_t;

}    // namespace aga

using aga::aga_ipc_msg_id_t;

#endif    // __AGA_CORE_IPC_MSG_HPP__
