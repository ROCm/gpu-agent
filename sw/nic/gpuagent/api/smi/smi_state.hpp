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
/// smi library state
///
//----------------------------------------------------------------------------

#ifndef __AGA_SMI_STATE_HPP__
#define __AGA_SMI_STATE_HPP__

#include <unordered_map>
#include <set>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/thread/thread.hpp"
#include "nic/sdk/include/sdk/timestamp.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/sdk/include/sdk/lock.hpp"
#include "nic/gpuagent/api/include/aga_init.hpp"
#include "nic/gpuagent/api/include/aga_event.hpp"
#include "nic/gpuagent/api/internal/aga_event.hpp"
#include "nic/gpuagent/api/smi/smi_api.hpp"
#include "nic/gpuagent/api/smi/smi_events.hpp"
#include "nic/gpuagent/api/smi/smi_watch.hpp"

using std::set;
using std::unordered_map;

/// \defgroup AGA_SMI_STATE global state for smi interactions
/// \ingroup AGA
/// @{

namespace aga {

/// \brief event db map with processor handle as the key
typedef unordered_map<aga_gpu_handle_t, gpu_event_db_entry_t> gpu_event_db_t;

/// \brief gpu counter handle map
typedef unordered_map<uint64_t, uintptr_t> gpu_counter_handle_t;

/// \brief  smi_state class contains state of smi client
class smi_state {
public:
    /// \brief constructor
    smi_state() {
        num_gpu_ = 0;
    }

    /// \brief    destructor
    ~smi_state() {}

    /// \brief    initialization routine
    /// \param[in] init_params    initialization parameters
    /// \return SDK_RET_OK or error status in case of failure
    sdk_ret_t init(aga_api_init_params_t *init_params);

    /// \brief    event database and monitoring infra initialization
    /// \return SDK_RET_OK or error status in case of failure
    sdk_ret_t event_monitor_init(void);

    /// \brief    cleanup all the event monitoring state
    /// \return SDK_RET_OK or error status in case of failure
    sdk_ret_t event_monitor_cleanup(void);

    /// \brief    process incoming event subscription requests
    /// \param[in] req    event subscription request
    /// \return SDK_RET_OK or error status in case of failure
    sdk_ret_t process_event_subscribe_req(aga_event_subscribe_args_t *args);

    /// \brief    handle GPU events
    /// \param[in] num_events    number of events in the event buffer
    /// \param[in] event_buffer  event buffer containing the event information
    /// \return SDK_RET_OK or error status in case of failure
    sdk_ret_t handle_events(uint32_t num_events, void *event_buffer);

    /// \brief    return number of GPUs in the node
    /// \return    number of GPUs
    uint32_t num_gpu(void) const { return num_gpu_; }

    /// \brief    read all the events and invokve the callback provided for each
    /// \param[in] cb    callback function pointer
    /// \param[in] ctxt  opaque context passed back to the callback
    /// \return     SDK_RET_OK or error code in case of failure
    sdk_ret_t event_read(aga_event_read_cb_t cb, void *ctxt);

    /// \brief    process incoming event generate requests
    /// \param[in] args    pointer to event generate request
    /// \return SDK_RET_OK or error status in case of failure
    sdk_ret_t process_event_gen_req(aga_event_gen_args_t *args);

    /// \brief  process and store gpu watch subscriber info
    /// \param[in] args    pointer to the gpu watch subscriber info
    /// \return SDK_RET_OK or error status in case of failure
    sdk_ret_t process_gpu_watch_subscribe_req(
                  aga_gpu_watch_subscribe_args_t *req);

    /// \brief  notify all gpu watch subscribers
    /// \return SDK_RET_OK or error status in case of failure
    sdk_ret_t gpu_watch_notify_subscribers(void);

     /// \brief    watcher infra initialization
     /// \return SDK_RET_OK or error status in case of failure
     sdk_ret_t watcher_init(void);

     /// \brief    destroy watcher infra
     /// \return SDK_RET_OK or error status in case of failure
     sdk_ret_t watcher_destroy(void);

     /// \brief    get and update watch fields for all GPUs
     /// \return SDK_RET_OK or error status in case of failure
     sdk_ret_t watcher_update_watch_db(aga_gpu_watch_db_t *watch_db);

private:
    /// \brief spawn event monitor thread
    /// \return SDK_RET_OK or error status in case of failure
    sdk_ret_t spawn_event_monitor_thread_(void);

    /// \brief spawn watcher thread
    /// \return SDK_RET_OK or error status in case of failure
    sdk_ret_t spawn_watcher_thread_(void);

    /// \brief given list of inactive listeners, prune them from the state
    /// \param[in] listeners    list of listener information records
    /// \return SDK_RET_OK or error status in case of failure
    sdk_ret_t cleanup_event_listeners_(
                  vector<aga_event_listener_info_t>& listeners);

    /// \brief  clearnup inactive gpu watch subscribers
    /// \param[in] subscribers    list of inactive subscribers
    /// \return SDK_RET_OK or error status in case of failure
    sdk_ret_t cleanup_gpu_watch_inactive_subscribers_(
                  vector<gpu_watch_subscriber_info_t>& subscribers);

    /// \brief    update watcher fields of interest
    /// \param[in]  gpu_id      GPU id
    /// \param[in]  gpu_handle  GPU handle
    /// \param[out] watch_db    db to be updated
    /// \return SDK_RET_OK or error status in case of failure
    sdk_ret_t smi_watcher_update_all_watch_fields_(uint32_t gpu_id,
                  aga_gpu_handle_t gpu_handle, aga_gpu_watch_db_t *watch_db);

private:
    /// no. of GPUs in the system
    uint32_t num_gpu_;
    /// gpu handles
    aga_gpu_handle_t gpu_handles_[AGA_MAX_GPU];
    /// gpu cpunter handles
    gpu_counter_handle_t counter_handle_;
    /// event monitor thread instance
    sdk::event_thread::event_thread *event_monitor_thread_;
    /// watcher thread instance
    sdk::event_thread::event_thread *watcher_thread_;
    /// event database map
    gpu_event_db_t gpu_event_db_;
    /// gpu watch database
    gpu_watch_subscriber_db_t gpu_watch_subscriber_db_;
};

/// global singleton smi state class instance
extern smi_state g_smi_state;

/// \@}

}    // namespace aga

using aga::smi_state;

#endif    // __AGA_SMI_STATE_HPP__
