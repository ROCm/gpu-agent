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
/// smi layer mock API definitions
///
//----------------------------------------------------------------------------

#include <random>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/gpuagent/core/aga_core.hpp"
#include "nic/gpuagent/core/ipc_msg.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"
#include "nic/gpuagent/api/include/aga_init.hpp"
#include "nic/gpuagent/api/smi/smi_api.hpp"
#include "nic/gpuagent/api/smi/smi_events.hpp"
#include "nic/gpuagent/api/smi/smi_api_mock_impl.hpp"

/// initial delay (in seconds) after which event monitoring starts
#define AGA_SMI_EVENT_MONITOR_START_DELAY    10.0
/// event monitoring frequency (in seconds)
#define AGA_SMI_EVENT_MONITOR_INTERVAL       3.0

namespace aga {

/// event database indexed by processor handle
unordered_map<aga_gpu_handle_t, gpu_event_db_entry_t> g_gpu_event_db;
/// event monitor thread instance
sdk::event_thread::event_thread *g_event_monitor_thread;

/// \brief    fill clock frequency ranges of the given GPU
/// \param[in] gpu_handle   GPU handle
/// \param[out] spec        spec to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_gpu_clock_frequency_spec_ (aga_gpu_handle_t gpu_handle,
                                    aga_gpu_spec_t *spec)
{
    // fill sClock spec
    spec->clock_freq[0].clock_type = AGA_GPU_CLOCK_TYPE_SYSTEM;
    spec->clock_freq[0].lo = 500;
    spec->clock_freq[0].hi = 1700;
    // fill mClock spec
    spec->clock_freq[1].clock_type = AGA_GPU_CLOCK_TYPE_MEMORY;
    spec->clock_freq[1].lo = 400;
    spec->clock_freq[1].hi = 1600;
    // fill video clock spec
    spec->clock_freq[2].clock_type = AGA_GPU_CLOCK_TYPE_VIDEO;
    spec->clock_freq[2].lo = 914;
    spec->clock_freq[2].hi = 1333;
    // fill data clock spec
    spec->clock_freq[3].clock_type = AGA_GPU_CLOCK_TYPE_DATA;
    spec->clock_freq[3].lo = 711;
    spec->clock_freq[3].hi = 1143;
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_spec (aga_gpu_handle_t gpu_handle, aga_gpu_spec_t *spec)
{
    spec->overdrive_level = 0;
    spec->perf_level = AGA_GPU_PERF_LEVEL_AUTO;

    // fill gpu and memory clock frequencies
    smi_fill_gpu_clock_frequency_spec_(gpu_handle, spec);
    spec->compute_partition_type = AGA_GPU_COMPUTE_PARTITION_TYPE_SPX;
    return SDK_RET_OK;
}

/// \brief    fill list of pids using the given GPU
/// \param[in] gpu_handle GPU handle
/// \param[out] status    operational status to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_gpu_kfd_pid_status_ (aga_gpu_handle_t gpu_handle,
                              aga_gpu_status_t *status)
{
    // TODO: fill kfd pids when this data is available
    return SDK_RET_OK;
}

/// \brief      function to format firmware version
/// \param[out] fw_version    firmware component/version after formatting
/// \param[in]  block         firmware component name
/// \param[in]  version       firmware version
/// \return     none
static void
fill_gpu_fw_version_ (aga_gpu_fw_version_t *fw_version, const char *block,
                      const char *version)
{
    strncpy(fw_version->firmware, block, AGA_MAX_STR_LEN);
    strncpy(fw_version->version, version, AGA_MAX_STR_LEN);
}

/// \brief    fill supported and current frequencies of system clocks
/// \param[in] gpu_handle GPU handle
/// \param[out] status    operational status to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_clock_status_ (aga_gpu_handle_t gpu_handle, aga_gpu_status_t *status)
{
    for (uint32_t i = 0; i < AGA_GPU_MAX_CLOCK; i++) {
        auto clock_status = &status->clock_status[i];
        if (i < AGA_GPU_GFX_MAX_CLOCK) {
            // gfx clock
            clock_status->clock_type = AGA_GPU_CLOCK_TYPE_SYSTEM;
            clock_status->frequency = 138 + i;
            clock_status->locked = (i % 2);
            clock_status->deep_sleep =
                (clock_status->frequency <= 140) ? true : false;
        } else if (i < (AGA_GPU_GFX_MAX_CLOCK + AGA_GPU_MEM_MAX_CLOCK)) {
            // memory clock
            clock_status->clock_type = AGA_GPU_CLOCK_TYPE_MEMORY;
            clock_status->frequency = 900;
            clock_status->locked = false;
            clock_status->deep_sleep = false;
        } else if (i < (AGA_GPU_GFX_MAX_CLOCK + AGA_GPU_MEM_MAX_CLOCK +
                            AGA_GPU_VIDEO_MAX_CLOCK)) {
            // video clock
            clock_status->clock_type = AGA_GPU_CLOCK_TYPE_VIDEO;
            clock_status->frequency = 29;
            clock_status->locked = false;
            clock_status->deep_sleep = true;
        } else {
            // data clock
            clock_status->clock_type = AGA_GPU_CLOCK_TYPE_DATA;
            clock_status->frequency = 22;
            clock_status->locked = false;
            clock_status->deep_sleep = true;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_status (aga_gpu_handle_t gpu_handle, uint32_t gpu_id,
                     aga_gpu_status_t *status)
{
    status->index = gpu_id;
    status->handle = gpu_handle;
    // fill the GPU serial number
    strncpy(status->serial_num, "PCB046982-0071", AGA_MAX_STR_LEN);
    // fill the GPU card series
    strncpy(status->card_series, "AMD INSTINCT MI200 (MCM) OAM AC MBA MSFT",
            AGA_MAX_STR_LEN);
    // fill the GPU card model
    strncpy(status->card_model, "0x0b0c", AGA_MAX_STR_LEN);
    // fill the GPU vendor information
    strncpy(status->card_vendor, "Advanced Micro Devices, Inc. [AMD/ATI]",
            AGA_MAX_STR_LEN);
    // fill the driver version
    strncpy(status->driver_version, "6.3.6", AGA_MAX_STR_LEN);
    // fill the vbios part number
    strncpy(status->vbios_part_number, "113-D65205-107", AGA_MAX_STR_LEN);
    // fill the vbios version
    strncpy(status->vbios_version, "022.040.003.041.000001", AGA_MAX_STR_LEN);
    // fill sku
    strncpy(status->card_sku, "D65205", AGA_MAX_STR_LEN);
    // fill the firmware version
    fill_gpu_fw_version_(&status->fw_version[1], "MEC2", "78");
    fill_gpu_fw_version_(&status->fw_version[2], "RLC", "17");
    fill_gpu_fw_version_(&status->fw_version[4], "SDMA2", "8");
    fill_gpu_fw_version_(&status->fw_version[7], "TA_RAS", "27.00.01.60");
    fill_gpu_fw_version_(&status->fw_version[8], "TA_XGMI", "32.00.00.19");
    fill_gpu_fw_version_(&status->fw_version[9], "VCN", "0x0110101b");
    // fill the memory vendor
    strncpy(status->memory_vendor, "hynix", AGA_MAX_STR_LEN);
    smi_fill_clock_status_(gpu_handle, status);
    // fill the PCIe bus id
    strncpy(status->pcie_status.pcie_bus_id, "0000:59:00.0", AGA_MAX_STR_LEN);
    status->pcie_status.slot_type = AGA_PCIE_SLOT_TYPE_OAM;
    status->pcie_status.width = 16;
    status->pcie_status.max_width = 16;
    status->pcie_status.speed = 16;
    status->pcie_status.max_speed = 32;
    status->pcie_status.bandwidth = 315;
    // fill VRAM status
    status->vram_status.type = AGA_VRAM_TYPE_HBM;
    status->vram_status.vendor = AGA_VRAM_VENDOR_HYNIX;
    status->vram_status.size = 196592;
    // fill the xgmi error count
    status->xgmi_status.error_status = AGA_GPU_XGMI_STATUS_NO_ERROR;
    // fill total memory
    // fill kfd pid info
    smi_fill_gpu_kfd_pid_status_(gpu_handle, status);
    status->partition_id = 0;
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_stats (aga_gpu_handle_t gpu_handle, aga_gpu_stats_t *stats)
{
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(0, 90);

    // fill the avg package power
    stats->avg_package_power = 90 + distr(gen) - distr(gen);
    // fill the current package power
    stats->package_power = 90 + distr(gen) - distr(gen);
    // fill the GPU usage
    stats->usage.gfx_activity = 21282136 + distr(gen) - distr(gen);
    // fill VRAM usage
    stats->vram_usage.total_vram = 196592;
    stats->vram_usage.used_vram = 1273;
    stats->vram_usage.free_vram =
        stats->vram_usage.total_vram - stats->vram_usage.used_vram;
    stats->vram_usage.total_visible_vram = 196592;
    stats->vram_usage.used_visible_vram = 1273;
    stats->vram_usage.free_visible_vram =
        stats->vram_usage.total_visible_vram -
            stats->vram_usage.used_visible_vram;
    stats->vram_usage.total_gtt = 128716;
    stats->vram_usage.used_gtt = 20;
    stats->vram_usage.free_gtt =
        stats->vram_usage.total_gtt - stats->vram_usage.used_gtt;
    // fill the energy consumed
    stats->energy_consumed = 25293978861568 + distr(gen) - distr(gen);
    return SDK_RET_OK;
}

typedef struct gpu_event_cb_ctxt_s {
    aga_event_read_cb_t cb;
} gpu_event_cb_ctxt_t;

static inline bool
gpu_event_read_cb (void *obj, void *ctxt)
{
    aga_event_t event;
    gpu_entry *gpu = (gpu_entry *)obj;
    gpu_event_cb_ctxt_t *walk_ctxt = (gpu_event_cb_ctxt_t *)ctxt;

    auto& event_map = g_gpu_event_db[gpu->handle()].event_map;
    // lock the event map for this device
    SDK_SPINLOCK_LOCK(&g_gpu_event_db[gpu->handle()].slock);
    for (auto it = event_map.begin(); it != event_map.end(); it++) {
        event = {};
        auto& event_record = it->second;

        // fill the event information
        event.id = it->first;
        event.timestamp = event_record.timestamp;
        event.gpu = gpu->key();
        strncpy(event.message, event_record.message, AGA_MAX_EVENT_STR);
        event.message[AGA_MAX_EVENT_STR] = '\0';
        // call the callback now
        walk_ctxt->cb(&event, ctxt);
    }
    // unlock the event map for this device
    SDK_SPINLOCK_UNLOCK(&g_gpu_event_db[gpu->handle()].slock);
    return false;
}

sdk_ret_t
event_read (aga_event_read_cb_t cb, void *ctxt)
{
    gpu_event_cb_ctxt_t event_ctxt;

    event_ctxt.cb = cb;
    gpu_db()->walk(gpu_event_read_cb, &event_ctxt);
    return SDK_RET_OK;
}

sdk_ret_t
smi_event_read_all (aga_event_read_cb_t cb, void *ctxt)
{
    return event_read(cb, ctxt);
}

sdk_ret_t
event_monitor_init (void)
{
    gpu_event_record_t null_event_record = {};

    // initialize the s/w state
    for (uint32_t d = 0; d < AGA_MOCK_NUM_GPU; d++) {
        SDK_SPINLOCK_INIT(&g_gpu_event_db[gpu_get_handle(d)].slock,
                          PTHREAD_PROCESS_SHARED);
    }
    return SDK_RET_OK;
}

sdk_ret_t
cleanup_event_listeners (vector<aga_event_listener_info_t>& listeners)
{
    aga_event_listener_info_t listener;

    for (auto it = listeners.begin(); it != listeners.end(); it++) {
        listener = *it;

        // if client context of one gpu is inactive,
        // we should erase the client context from all gpus
        // and all events related to this gRPC stream before
        // waking up the front end, otherwise the client contexts
        // stored for other gpus for the same subscribe request
        // will eventually lead to agent crash

        for (uint32_t d = 0; d < AGA_MOCK_NUM_GPU; d++) {
            // lock the event state for this device
            SDK_SPINLOCK_LOCK(&g_gpu_event_db[gpu_get_handle(d)].slock);
            for (uint32_t e = (AGA_EVENT_ID_NONE + 1); e <= AGA_EVENT_ID_MAX;
                 e++) {
                auto& event_record =
                    g_gpu_event_db[gpu_get_handle(d)].event_map[(aga_event_id_t)e];
                // erase the client
                event_record.client_info.client_set.erase(listener.client_ctxt);
            }
            // unlock the event state for this device
            SDK_SPINLOCK_UNLOCK(&g_gpu_event_db[gpu_get_handle(d)].slock);
        }
        // wakeup the front end thread so it can exit
        listener.client_ctxt->client_inactive = true;
        AGA_TRACE_INFO("Signaling frontend gRPC thread to quit, client {}, "
                       "client ctxt {}, stream {}",
                       listener.client_ctxt->client.c_str(),
                       (void *)listener.client_ctxt,
                       listener.client_ctxt->stream);
        pthread_cond_signal(&listener.client_ctxt->cond);
    }
    return SDK_RET_OK;
}

static sdk_ret_t
handle_events (uint32_t num_events, void *event_buffer)
{
    sdk_ret_t ret;
    timespec_t ts;
    gpu_entry *gpu;
    aga_gpu_handle_t gpu_handle;
    aga_event_t event = {};
    aga_event_id_t event_id;
    aga_event_client_ctxt_t *client_ctxt;
    aga_event_listener_info_t inactive_listener;
    vector<aga_event_listener_info_t> inactive_listeners;

    // get current time
    clock_gettime(CLOCK_REALTIME, &ts);
    // start processing all the events
    for (uint32_t i = 0; i < num_events; i++) {
        gpu_handle = event_buffer_get_gpu_handle(event_buffer, i);
        gpu = gpu_db()->find(gpu_handle);
        if (gpu == NULL) {
            continue;
        }
        event_id = event_buffer_get_event_id(event_buffer, i);
        auto& event_map = g_gpu_event_db[gpu_handle].event_map;

        // lock the event state for this device
        SDK_SPINLOCK_LOCK(&g_gpu_event_db[gpu_handle].slock);
        // update our event state
        auto& event_record = event_map[event_id];
        event_record.timestamp = ts;
        strncpy(event_record.message, event_buffer_get_message(event_buffer, i),
                AGA_MAX_EVENT_STR);
        event_record.message[AGA_MAX_EVENT_STR] = '\0';
        // fill the event record
        event.id = event_id;
        event.timestamp = ts;
        event.gpu = gpu->key();
        strncpy(event.message, event_buffer_get_message(event_buffer, i),
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
                inactive_listener.gpu_id = gpu->id();
                inactive_listener.event = event_id;
                inactive_listener.client_ctxt = *client_set_it;
                inactive_listeners.push_back(inactive_listener);
            }
        }
        // unlock the event state maintained for this device
        SDK_SPINLOCK_UNLOCK(&g_gpu_event_db[gpu_handle].slock);
    }
    // handle all the dead clients now
    cleanup_event_listeners(inactive_listeners);
    return SDK_RET_OK;
}

static void
event_monitor_timer_cb (sdk::event_thread::timer_t *timer)
{
    // handle all the events
    handle_events(1, event_get());
}

/// \brief process an event subscribe request from client
/// \param[in] req    pointer to incoming request
/// \return SDK_RET_OK if success or error code in case of failure
sdk_ret_t
process_event_subscribe_req (aga_event_subscribe_args_t *req)
{
    gpu_event_record_t event_record = {};

    for (size_t i = 0; i < req->events.size(); i++) {
        AGA_TRACE_DEBUG("Rcvd event {} subscribe request, client {}, "
                        "client ctxt {}, stream {}",  req->events[i],
                        req->client_ctxt->client.c_str(),
                        (void *)req->client_ctxt,
                        (void *)req->client_ctxt->stream);
        for (size_t g = 0; g < req->gpu_ids.size(); g++) {
            uint32_t d = req->gpu_ids[g];
            auto& event_map = g_gpu_event_db[gpu_get_handle(d)].event_map;

            // lock the event map for this device
            SDK_SPINLOCK_LOCK(&g_gpu_event_db[gpu_get_handle(d)].slock);
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
            SDK_SPINLOCK_UNLOCK(&g_gpu_event_db[gpu_get_handle(d)].slock);
        }
    }
    return SDK_RET_OK;
}

/// \brief callback function to process IPC msg from gRPC thread
///        to handle event subscription requests
/// \param[in] msg    received IPC message
/// \param[in] ctxt   opaque context (used when callback was registered)
static void
event_subscribe_ipc_cb (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    sdk_ret_t ret;
    aga_event_subscribe_args_t *req;

    req = *(aga_event_subscribe_args_t **)msg->data();
    if (req == NULL) {
        AGA_TRACE_ERR("Ignoring NULL event subscribe request received");
        return;
    }
    ret = process_event_subscribe_req(req);
    sdk::ipc::respond(msg, &ret, sizeof(ret));
}

static void
event_monitor_thread_init (void *ctxt)
{
    static sdk::event_thread::timer_t event_monitor_timer;

    // initialize event monitoring state
    event_monitor_init();
    // subscribe to all IPC msgs of interest
    sdk::ipc::reg_request_handler(AGA_IPC_MSG_ID_EVENT_SUBSCRIBE,
                                  event_subscribe_ipc_cb, NULL);
    // start event monitoring timer
    sdk::event_thread::timer_init(&event_monitor_timer, event_monitor_timer_cb,
                                  AGA_SMI_EVENT_MONITOR_START_DELAY,
                                  AGA_SMI_EVENT_MONITOR_INTERVAL);
    sdk::event_thread::timer_start(&event_monitor_timer);
}

static void
event_monitor_thread_exit (void *ctxt)
{
    // cleanup the event state
    for (uint32_t d = 0; d < AGA_MOCK_NUM_GPU; d++) {
        SDK_SPINLOCK_LOCK(&g_gpu_event_db[gpu_get_handle(d)].slock);
        g_gpu_event_db[gpu_get_handle(d)].event_map.clear();
        SDK_SPINLOCK_UNLOCK(&g_gpu_event_db[gpu_get_handle(d)].slock);
    }
}

sdk_ret_t
spawn_event_monitor_thread (void)
{
    g_event_monitor_thread =
        sdk::event_thread::event_thread::factory(
            "event-monitor", AGA_THREAD_ID_EVENT_MONITOR,
            sdk::lib::THREAD_ROLE_CONTROL, 0x0, event_monitor_thread_init,
            event_monitor_thread_exit, NULL, // message
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            (THREAD_YIELD_ENABLE | THREAD_SYNC_IPC_ENABLE));
    SDK_ASSERT_TRACE_RETURN((g_event_monitor_thread != NULL), SDK_RET_ERR,
                            "GPU event monitor thread create failure");
    g_event_monitor_thread->start(NULL);
    return SDK_RET_OK;
}

sdk_ret_t
smi_init (aga_api_init_params_t *init_params)
{
    // spawn event monitor thread
    spawn_event_monitor_thread();
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_reset (aga_gpu_handle_t gpu_handle, aga_gpu_reset_type_t reset_type)
{
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_update (aga_gpu_handle_t gpu_handle, aga_gpu_spec_t *spec,
                uint64_t upd_mask)
{
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_device_topology (aga_gpu_handle_t gpu_handle,
                              aga_device_topology_info_t *info)
{
    uint32_t gpu_id;
    uint32_t cnt = 0;
    static std::string name = "GPU";

    // get linear GPU index from device name
    sscanf(info->device.name, "GPU%u", &gpu_id);
    for (uint32_t i = 0; i < AGA_MOCK_NUM_GPU; i++) {
        if (gpu_handle != gpu_get_handle(i)) {
            info->peer_device[cnt].peer_device.type = AGA_DEVICE_TYPE_GPU;
            strcpy(info->peer_device[cnt].peer_device.name,
                   (name + std::to_string(i)).c_str());
            info->peer_device[cnt].num_hops = 1;
            info->peer_device[cnt].connection.type = AGA_IO_LINK_TYPE_XGMI;
            info->peer_device[cnt].link_weight = 15 + (15 * ((i + gpu_id) % 5));
            info->peer_device[cnt].valid = true;
            cnt++;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_get_gpu_partition_id (aga_gpu_handle_t gpu_handle, uint32_t *partition_id)
{
    *partition_id = 0;
    return SDK_RET_OK;
}

sdk_ret_t
smi_discover_gpus (uint32_t *num_gpus, aga_gpu_handle_t *gpu_handles,
                   aga_obj_key_t *gpu_keys)
{
    if (!num_gpus) {
        return SDK_RET_ERR;
    }
    *num_gpus = AGA_MOCK_NUM_GPU;
    for (uint32_t i = 0; i < *num_gpus; i++) {
        gpu_handles[i] = gpu_get_handle(i);
    }
    if (gpu_keys) {
        for (uint32_t i = 0; i < *num_gpus; i++) {
            gpu_keys[i] = gpu_uuid(i, gpu_get_unique_id(i));
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_get_bad_page_count (void *gpu_obj,
                            uint32_t *num_bad_pages)
{
    *num_bad_pages = 1;
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_get_bad_page_records (void *gpu_obj,
                              uint32_t num_bad_pages,
                              aga_gpu_bad_page_record_t *records)
{
    gpu_entry *gpu = (gpu_entry *)gpu_obj;

    records[0].key = gpu->key();
    records[0].page_address = 0x5c70ec;
    records[0].page_size = 4096;
    records[0].page_status = AGA_GPU_PAGE_STATUS_UNRESERVABLE;
    return SDK_RET_OK;
}

}    // namespace aga
