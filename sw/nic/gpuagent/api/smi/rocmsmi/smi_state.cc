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
/// smi library state implementation
///
//----------------------------------------------------------------------------

#include <vector>
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/core/aga_core.hpp"
#include "nic/gpuagent/core/ipc_msg.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/smi/smi_state.hpp"
#include "nic/gpuagent/api/smi/rocmsmi/smi_utils.hpp"

using std::vector;

/// \defgroup AGA_SMI_STATE global state for smi interactions
/// @{

namespace event = sdk::event_thread;

/// initial delay after which event monitoring starts
#define AGA_SMI_EVENT_MONITOR_START_DELAY    10.0
/// event monitoring frequency (in seconds)
#define AGA_SMI_EVENT_MONITOR_INTERVAL       3.0
/// all rsmi events of interest
#define RSMI_EVENT_MASK_ALL                  \
            ((1 << RSMI_EVT_NOTIF_VMFAULT)          |    \
             (1 << RSMI_EVT_NOTIF_THERMAL_THROTTLE) |    \
             (1 << RSMI_EVT_NOTIF_GPU_PRE_RESET)    |    \
             (1 << RSMI_EVT_NOTIF_GPU_POST_RESET))
/// timeout to wait to gather outstanding events (in milliseconds)
#define RSMI_EVENT_NTFN_TIMEOUT              0

namespace aga {

/// global singleton smi state class instance
smi_state g_smi_state;

sdk_ret_t
smi_state::event_monitor_init(void) {
    rsmi_status_t status;
    gpu_event_record_t null_event_record = {};

    // initialize the s/w state
    for (uint32_t d = 0; d < num_gpu_; d++) {
        SDK_SPINLOCK_INIT(&gpu_event_db_[gpu_handles_[d]].slock,
                          PTHREAD_PROCESS_SHARED);
    }
    // initialize event monitoring for all the devices
    for (uint32_t d = 0; d < num_gpu_; d++) {
        // initialize the event monitoring for the 1st time for all devices
        status = rsmi_event_notification_init(d);
        if (unlikely(status != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to do event notification initialization, "
                          "GPU {}, err {}", status);
            return rsmi_ret_to_sdk_ret(status);
        }
        status = rsmi_event_notification_mask_set(d, RSMI_EVENT_MASK_ALL);
        if (unlikely(status != RSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to set event notification mask, "
                          "GPU {}, err {}", status);
            return rsmi_ret_to_sdk_ret(status);
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::event_monitor_cleanup(void) {
    // stop monitoring
    for (uint32_t d = 0; d < num_gpu_; d++) {
        rsmi_event_notification_stop(d);
    }
    // cleanup the event state
    for (uint32_t d = 0; d < AGA_MAX_GPU; d++) {
        SDK_SPINLOCK_LOCK(&gpu_event_db_[gpu_handles_[d]].slock);
        gpu_event_db_[gpu_handles_[d]].event_map.clear();
        SDK_SPINLOCK_UNLOCK(&gpu_event_db_[gpu_handles_[d]].slock);
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::cleanup_event_listeners_(
               vector<aga_event_listener_info_t>& listeners) {
    aga_event_listener_info_t listener;
    aga_event_client_ctxt_t *client_ctxt;
    set<aga_event_client_ctxt_t *> client_set;

    for (auto it = listeners.begin(); it != listeners.end(); it++) {
        listener = *it;

        // if client context of one gpu is inactive,
        // we should erase the client context from all gpus
        // and all events related to this gRPC stream before
        // waking up the front end, otherwise the client contexts
        // stored for other gpus for the same subscribe request
        // will eventually lead to agent crash

        for (uint32_t d = 0; d < num_gpu_; d++) {
            // lock the event state for this device
            SDK_SPINLOCK_LOCK(&gpu_event_db_[gpu_handles_[d]].slock);
            for (uint32_t e = (AGA_EVENT_ID_NONE + 1); e <= AGA_EVENT_ID_MAX;
                 e++) {
                auto& event_record =
                    gpu_event_db_[gpu_handles_[d]].event_map[(aga_event_id_t)e];
                // erase the client
                event_record.client_info.client_set.erase(listener.client_ctxt);
            }
            // unlock the event state for this device
            SDK_SPINLOCK_UNLOCK(&gpu_event_db_[gpu_handles_[d]].slock);
        }
        client_set.insert(listener.client_ctxt);
    }
    for (auto it = client_set.begin(); it!= client_set.end(); it++) {
        client_ctxt = *it;
        // wakeup the front end thread so it can exit
        client_ctxt->client_inactive = true;
        AGA_TRACE_INFO("Signaling frontend gRPC thread to quit, client {}, "
                       "client ctxt {}, stream {}",
                       client_ctxt->client.c_str(),
                       (void *)client_ctxt,
                       client_ctxt->stream);
        pthread_cond_signal(&client_ctxt->cond);
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::handle_events(uint32_t num_events, void *event_buffer_) {
    sdk_ret_t ret;
    timespec_t ts;
    uint32_t gpu_id;
    aga_event_t event = {};
    aga_event_id_t event_id;
    aga_event_client_ctxt_t *client_ctxt;
    rsmi_evt_notification_data_t *event_buffer;
    aga_event_listener_info_t inactive_listener;
    vector<aga_event_listener_info_t> inactive_listeners;

    event_buffer = (rsmi_evt_notification_data_t *)event_buffer_;
    // get current time
    clock_gettime(CLOCK_REALTIME, &ts);
    // start processing all the events
    for (uint32_t i = 0; i < num_events; i++) {
        gpu_id = event_buffer[i].dv_ind;
        event_id = aga_event_id_from_smi_event_id(event_buffer[i].event);
        auto& event_map = gpu_event_db_[gpu_handles_[gpu_id]].event_map;

        // lock the event state for this device
        SDK_SPINLOCK_LOCK(&gpu_event_db_[gpu_handles_[gpu_id]].slock);
        // update our event state
        auto& event_record = event_map[event_id];
        event_record.timestamp = ts;
        strncpy(event_record.message, event_buffer[i].message,
                AGA_MAX_EVENT_STR);
        event_record.message[AGA_MAX_EVENT_STR] = '\0';
        // fill the event record
        event.id = event_id;
        event.timestamp = ts;
        event.gpu = gpu_db()->find(gpu_id)->key();
        strncpy(event.message, event_buffer[i].message,
                AGA_MAX_EVENT_STR);
        event.message[AGA_MAX_EVENT_STR] = '\0';
        // walk thru all the clients that are interested in this event and
        // notify them
        for (auto client_set_it = event_record.client_info.client_set.begin();
             client_set_it != event_record.client_info.client_set.end();
             client_set_it++) {
             client_ctxt = *client_set_it;
            // invoke the event notification callback
            ret = client_ctxt->notify_cb(&event, *client_set_it);
            if (unlikely(ret != SDK_RET_OK)) {
                // add to list of clients not reachable
                inactive_listener.gpu_id = gpu_id;
                inactive_listener.event = event_id;
                inactive_listener.client_ctxt = *client_set_it;
                inactive_listeners.push_back(inactive_listener);
            }
        }
        // unlock the event state maintained for this device
        SDK_SPINLOCK_UNLOCK(&gpu_event_db_[gpu_handles_[gpu_id]].slock);
    }
    // handle all the dead clients now
    cleanup_event_listeners_(inactive_listeners);
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::event_read(aga_event_read_cb_t cb, void *ctxt) {
    aga_event_t event;

    // traverse the event database per device
    for (uint32_t d = 0; d < num_gpu_; d++) {
        auto& event_map = gpu_event_db_[gpu_handles_[d]].event_map;
        // lock the event map for this device
        SDK_SPINLOCK_LOCK(&gpu_event_db_[gpu_handles_[d]].slock);
        for (auto it = event_map.begin(); it != event_map.end(); it++) {
            event = {};
            auto& event_record = it->second;

            // fill the event information
            event.id = it->first;
            event.timestamp = event_record.timestamp;
            event.gpu = gpu_db()->find(d)->key();
            strncpy(event.message, event_record.message, AGA_MAX_EVENT_STR);
            event.message[AGA_MAX_EVENT_STR] = '\0';
            // call the callback now
            cb(&event, ctxt);
        }
        // unlock the event map for this device
        SDK_SPINLOCK_UNLOCK(&gpu_event_db_[gpu_handles_[d]].slock);
    }
    return SDK_RET_OK;
}

static void
event_monitor_timer_cb_ (event::timer_t *timer)
{
    rsmi_status_t status;
    uint32_t num_elem = AGA_MAX_GPU * AGA_EVENT_ID_MAX;
    rsmi_evt_notification_data_t event_ntfn_data[num_elem];
    // get event information
    status = rsmi_event_notification_get(AGA_SMI_EVENT_MONITOR_INTERVAL,
                                         &num_elem, event_ntfn_data);
    if (unlikely(status != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get event notification data, err {}", status);
        return;
    }

    // handle all the events
    g_smi_state.handle_events(num_elem, event_ntfn_data);
}

/// \brief process an event subscribe request from client
/// \param[in] req    pointer to incoming request
/// \return SDK_RET_OK if success or error code in case of failure
sdk_ret_t
smi_state::process_event_subscribe_req(aga_event_subscribe_args_t *req) {
    gpu_event_record_t event_record = {};

    for (size_t i = 0; i < req->events.size(); i++) {
        AGA_TRACE_DEBUG("Rcvd event {} subscribe request, client {}, "
                        "client ctxt {}, stream {}",  req->events[i],
                        req->client_ctxt->client.c_str(),
                        (void *)req->client_ctxt,
                        (void *)req->client_ctxt->stream);
        for (size_t g = 0; g < req->gpu_ids.size(); g++) {
            uint32_t d = req->gpu_ids[g];
            auto& event_map = gpu_event_db_[gpu_handles_[d]].event_map;

            // lock the event map for this device
            SDK_SPINLOCK_LOCK(&gpu_event_db_[gpu_handles_[d]].slock);
            // check if this event was of interest to any client or happened
            // already
            auto event_map_it = event_map.find(req->events[i]);
            if (event_map_it == event_map.end()) {
                // 1st time anyone is subscribing to this event
                event_record.client_info.client_set.insert(req->client_ctxt);
                event_map[req->events[i]] = event_record;
            } else {
                // atleast one client is already interested in this event, check
                // if this particular client already subscribed to this event
                auto set_it = event_map_it->second.client_info.client_set.find(
                                                       req->client_ctxt);
                if (set_it ==
                        event_map_it->second.client_info.client_set.end()) {
                    // this client is a new listener for this event
                    event_map_it->second.client_info.client_set.insert(
                                                         req->client_ctxt);
                } else {
                    // this client is already subscribed to this event
                }
            }
            // unlock the event map for this device
            SDK_SPINLOCK_UNLOCK(&gpu_event_db_[gpu_handles_[d]].slock);
        }
    }
    return SDK_RET_OK;
}

/// \brief callback function to process IPC msg from gRPC thread
///        to handle event subscription requests
/// \param[in] msg    received IPC message
/// \param[in] ctxt   opaque context (used when callback was registered)
static void
event_subscribe_ipc_cb_ (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    sdk_ret_t ret;
    aga_event_subscribe_args_t *req;

    req = *(aga_event_subscribe_args_t **)msg->data();
    if (req == NULL) {
        AGA_TRACE_ERR("Ignoring NULL event subscribe request received");
        return;
    }
    ret = g_smi_state.process_event_subscribe_req(req);
    sdk::ipc::respond(msg, &ret, sizeof(ret));
}

static inline const char *
event_description_ (aga_event_id_t event_id)
{
    switch (event_id) {
    case AGA_EVENT_ID_VM_PAGE_FAULT:
        return "Test event: VM page fault occured";
    case AGA_EVENT_ID_THERMAL_THROTTLE:
        return
            "Test event: clock frequency has decreased due to temperature rise";
    case AGA_EVENT_ID_GPU_PRE_RESET:
        return "Teset event: GPU reset about to happen";
    case AGA_EVENT_ID_GPU_POST_RESET:
        return "Test event: GPU reset happened";
    default:
        break;
    }
    return "Test event: unknown event";
}

sdk_ret_t
smi_state::process_event_gen_req(aga_event_gen_args_t *args) {
    sdk_ret_t ret;
    uint32_t num_elem = 0;
    rsmi_evt_notification_type_t smi_event;
    rsmi_evt_notification_data_t event_data[AGA_MAX_GPU * AGA_EVENT_ID_MAX];

    for (size_t i = 0; i < args->gpu_ids.size(); i++) {
        for (size_t e = 0; e < args->events.size(); e++) {
            ret = aga_event_id_to_smi_event_id(args->events[e], &smi_event);
            if (unlikely(ret != SDK_RET_OK)) {
                AGA_TRACE_ERR("Failed to generate event {}, smi event not "
                              "found", args->events[e]);
                return ret;
            }
            event_data[num_elem].event = smi_event;
            event_data[num_elem].dv_ind = args->gpu_ids[i];
            strncpy(event_data[num_elem].message,
                    event_description_(args->events[e]),
                    MAX_EVENT_NOTIFICATION_MSG_SIZE);
            num_elem++;
        }
    }
    // handle all the events
    g_smi_state.handle_events(num_elem, event_data);
    return SDK_RET_OK;
}

/// \brief callback function to process IPC msg from gRPC thread
///        to handle event generate requests
/// \param[in] msg    received IPC message
/// \param[in] ctxt   opaque context (used when callback was registered)
static void
event_gen_ipc_cb_ (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    sdk_ret_t ret;
    aga_event_gen_args_t *args;

    args = (aga_event_gen_args_t *)msg->data();
    if (args == NULL) {
        AGA_TRACE_ERR("Ignoring NULL event generate request received");
        return;
    }
    ret = g_smi_state.process_event_gen_req(args);
    sdk::ipc::respond(msg, &ret, sizeof(ret));
}

static void
event_monitor_thread_init_ (void *ctxt)
{
    static event::timer_t event_monitor_timer;

    // initialize event monitoring state
    g_smi_state.event_monitor_init();
    // subscribe to all IPC msgs of interest
    sdk::ipc::reg_request_handler(AGA_IPC_MSG_ID_EVENT_SUBSCRIBE,
                                  event_subscribe_ipc_cb_, NULL);
    sdk::ipc::reg_request_handler(AGA_IPC_MSG_ID_EVENT_GEN,
                                  event_gen_ipc_cb_, NULL);
    // start event monitoring timer
    event::timer_init(&event_monitor_timer, event_monitor_timer_cb_,
                      AGA_SMI_EVENT_MONITOR_START_DELAY,
                      AGA_SMI_EVENT_MONITOR_INTERVAL);
    event::timer_start(&event_monitor_timer);
}

void
event_monitor_thread_exit_ (void *ctxt)
{
    // cleanup the event monitoring state
    g_smi_state.event_monitor_cleanup();
}

sdk_ret_t
smi_state::spawn_event_monitor_thread_(void) {
    event_monitor_thread_ =
        sdk::event_thread::event_thread::factory(
            "event-monitor", AGA_THREAD_ID_EVENT_MONITOR,
            sdk::lib::THREAD_ROLE_CONTROL, 0x0, event_monitor_thread_init_,
            event_monitor_thread_exit_, NULL, // message
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(
                                  sdk::lib::THREAD_ROLE_CONTROL),
            (THREAD_YIELD_ENABLE | THREAD_SYNC_IPC_ENABLE));
    SDK_ASSERT_TRACE_RETURN((event_monitor_thread_ != NULL), SDK_RET_ERR,
                            "GPU event monitor thread create failure");
    event_monitor_thread_->start(NULL);
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::init(aga_api_init_params_t *init_params) {
    sdk_ret_t ret;
    rsmi_status_t status;

    // initialize smi library
    status = rsmi_init(0);
    if (unlikely(status != RSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to initialize rocm smi library, err {}", status);
        return rsmi_ret_to_sdk_ret(status);
    }
    // discover gpus
    ret = aga::smi_discover_gpus(&num_gpu_, gpu_handles_, NULL);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    // spawn event monitor thread
    spawn_event_monitor_thread_();
    return SDK_RET_OK;
}

/// \@}

}    // namespace aga
