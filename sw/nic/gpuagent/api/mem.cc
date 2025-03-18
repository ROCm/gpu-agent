
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
/// APIs for memory mgmt.
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/mem.hpp"
#include "nic/gpuagent/api/gpu.hpp"
#include "nic/gpuagent/api/gpu_watch.hpp"

namespace aga {

/// \brief callback invoked by the delay delete function to release an object
/// \param[in]    obj_id     object identifier
/// \param[in]    elem       element to free
static inline void
delay_delete_cb (uint32_t obj_id, void *elem)
{
    AGA_TRACE_VERBOSE("obj {}, elem {}", obj_id, elem);
    switch (obj_id) {
    case AGA_OBJ_ID_GPU:
        gpu_entry::destroy((gpu_entry *)elem);
        break;
    case AGA_OBJ_ID_GPU_WATCH:
        gpu_watch_entry::destroy((gpu_watch_entry *)elem);
        break;
    default:
        AGA_TRACE_ERR("Unknown object {}", obj_id);
        SDK_ASSERT(FALSE);
    }
    return;
}

/// \brief entry function for delay delete thread
/// \param[in] obj_id           object identifier
/// \param[in] elem             element to free
/// \param[in] timeout_secs     delay in secs
static void
delay_delete_thread_entry (uint32_t obj_id, void *elem, uint64_t timeout_secs)
{
    // sleep for specific delay secs
    sleep(timeout_secs);
    AGA_TRACE_VERBOSE("Deleting object {} type {}", elem, obj_id);
    // delete the object
    delay_delete_cb(obj_id, elem);
}

/// \brief function to delete element after delay
/// \param[in] obj_id           object identifier
/// \param[in] elem             element to free
/// \param[in] timeout_secs     delay in secs
/// \return SDK_RET_OK on success, failure status code on error
sdk_ret_t
delay_delete (uint32_t obj_id, void *elem, uint64_t timeout_secs)
{
    if (obj_id >= AGA_OBJ_ID_MAX) {
        AGA_TRACE_ERR("Unexpected object {}", obj_id);
        return SDK_RET_INVALID_ARG;
    }
    AGA_TRACE_VERBOSE("Scheduling object {} type {} for delay delete, "
                      "after {} seconds", elem, obj_id, timeout_secs);
    // start a thread to handle delay delete
    std::thread(delay_delete_thread_entry, obj_id, elem, timeout_secs).detach();
    return SDK_RET_OK;
}

}    // namespace aga
