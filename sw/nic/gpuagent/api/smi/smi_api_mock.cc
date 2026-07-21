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
#include <sstream>
#include <iomanip>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
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

/// number of GPUs mocked; by default 16, when PLATFORM is set to be helios the
/// number of GPUs is 4
uint16_t g_num_gpu_mock = 16;
/// global array of GPU handles
aga_gpu_handle_t g_gpu_handles[AGA_MAX_GPU];

/// struct to hold GPU information
typedef struct gpu_cfg_s {
    /// GPU id
    uint32_t gpu_id;
    /// GPU uuid
    aga_obj_key_t key;
    /// bdf
    std::string bdf;
    /// GPU handle
    aga_gpu_handle_t handle;
} gpu_cfg_t;

/// unordered map to store GPU config info
std::unordered_map<aga_gpu_handle_t, gpu_cfg_t> g_gpu_map;

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
smi_gpu_init_immutable_attrs (aga_gpu_handle_t gpu_handle,
                              const aga_obj_key_t *gpu_key,
                              aga_gpu_spec_t *spec,
                              aga_gpu_status_t *status)
{
    // no need to do anything for mock
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_init_attrs (aga_gpu_handle_t gpu_handle, const aga_obj_key_t *gpu_key,
                    aga_gpu_spec_t *spec, aga_gpu_status_t *status)
{
    smi_gpu_init_immutable_attrs(gpu_handle, gpu_key, spec, status);
    smi_gpu_fill_spec(gpu_handle, gpu_key, spec);
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_spec (aga_gpu_handle_t gpu_handle,
                   const aga_obj_key_t *gpu_key,
                   aga_gpu_spec_t *spec)
{
    spec->overdrive_level = 0;
    spec->perf_level = AGA_GPU_PERF_LEVEL_AUTO;

    // fill gpu and memory clock frequencies
    smi_fill_gpu_clock_frequency_spec_(gpu_handle, spec);
    spec->compute_partition_type = AGA_GPU_COMPUTE_PARTITION_TYPE_SPX;
    return SDK_RET_OK;
}

/// \brief    fill GPU enumeration ids info using the given GPU
/// \param[in] gpu_handle    GPU handle
/// \param[out] status    operational status to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_gpu_enumeration_id_status_ (aga_gpu_handle_t gpu_handle,
                                     aga_gpu_status_t *status)
{
    status->kfd_id = 58934;
    status->node_id = 3;
    status->drm_render_id = 128;
    status->drm_card_id = 3;
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
smi_gpu_fill_status (aga_gpu_handle_t gpu_handle,
                     const aga_obj_key_t *gpu_key,
                     uint32_t gpu_id,
                     aga_gpu_spec_t *spec, aga_gpu_status_t *status)
{
    status->index = gpu_id;
    status->handle = gpu_handle;
    // fill the GPU serial number
    strncpy(status->serial_num, "PCB046982-0071", AGA_MAX_STR_LEN);
    // fill the GPU card series
    strncpy(status->card_series, "AMD INSTINCT MI300 (MCM) OAM AC MBA MSFT",
            AGA_MAX_STR_LEN);
    // fill the GPU card model
    strncpy(status->card_model, "102-G30211-00", AGA_MAX_STR_LEN);
    // fill the GPU vendor information
    strncpy(status->card_vendor, "Advanced Micro Devices, Inc. [AMD/ATI]",
            AGA_MAX_STR_LEN);
    // fill the driver version
    strncpy(status->driver_version, "7.0.0", AGA_MAX_STR_LEN);
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
    strncpy(status->pcie_status.pcie_bus_id, g_gpu_map[gpu_handle].bdf.c_str(),
            AGA_MAX_STR_LEN);
    status->pcie_status.slot_type = AGA_PCIE_SLOT_TYPE_OAM;
    status->pcie_status.width = 16;
    status->pcie_status.max_width = 16;
    status->pcie_status.speed = 16;
    status->pcie_status.max_speed = 32;
    status->pcie_status.bandwidth = 315;
    // fill VRAM status
    status->vram_status.type = AGA_VRAM_TYPE_HBM;
    strcpy(status->vram_status.vendor, "hynix");
    status->vram_status.size = 196592;
    // fill VRAM max bandwidth mock value
    status->vram_status.max_bandwidth = 3276800;
    // fill the xgmi error count
    status->xgmi_status.error_status = AGA_GPU_XGMI_STATUS_NO_ERROR;
    // fill total memory
    // fill kfd pid info
    smi_fill_gpu_kfd_pid_status_(gpu_handle, status);
    status->partition_id = 0;
    smi_fill_gpu_enumeration_id_status_(gpu_handle, status);
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_stats (aga_gpu_handle_t gpu_handle,
                    const aga_obj_key_t *gpu_key,
                    bool is_partitioned,
                    uint32_t partition_id,
                    aga_gpu_handle_t first_partition_handle,
                    aga_gpu_stats_t *stats)
{
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(0, 90);

    // fill the avg package power
    stats->avg_package_power = 90 + distr(gen) - distr(gen);
    // fill the current package power
    stats->package_power = 90 + distr(gen) - distr(gen);
    // fill the GPU usage
    stats->usage.gfx_activity = distr(gen) % 100;
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
    // fill the PCIe stats
    ++stats->pcie_stats.replay_count;
    ++stats->pcie_stats.tx_bytes;
    ++stats->pcie_stats.recovery_count;
    ++stats->pcie_stats.replay_rollover_count;
    ++stats->pcie_stats.nack_sent_count;
    ++stats->pcie_stats.nack_received_count;
    ++stats->pcie_stats.rx_bytes;
    ++stats->pcie_stats.tx_bytes;
    ++stats->pcie_stats.bidir_bandwidth;
    // fill the energy consumed
    stats->energy_consumed = 25293978861568 + distr(gen) - distr(gen);
    for (uint16_t i = 0; i < AMDSMI_MAX_NUM_XCC; i++) {
        stats->usage.gfx_busy_inst[i] = distr(gen) % 100 ;
    }
    // fill violation stats
    stats->violation_stats.current_accumulated_counter = 123456 + distr(gen) - distr(gen);
    stats->violation_stats.processor_hot_residency_accumulated = 23456 + distr(gen) - distr(gen);
    stats->violation_stats.ppt_residency_accumulated = 34567 + distr(gen) - distr(gen);
    stats->violation_stats.socket_thermal_residency_accumulated = 45678 + distr(gen) - distr(gen);
    stats->violation_stats.vr_thermal_residency_accumulated = 56789 + distr(gen) - distr(gen);
    stats->violation_stats.hbm_thermal_residency_accumulated = 67890 + distr(gen) - distr(gen);
    stats->violation_stats.processor_hot_residency_percentage = distr(gen) % 100;
    stats->violation_stats.ppt_residency_percentage = distr(gen) % 100;
    stats->violation_stats.socket_thermal_residency_percentage = distr(gen) % 100;
    stats->violation_stats.vr_thermal_residency_percentage = distr(gen) % 100;
    stats->violation_stats.hbm_thermal_residency_percentage = distr(gen) % 100;

    for (uint16_t i = 0; i < AMDSMI_MAX_NUM_XCC; i++) {
        stats->violation_stats.gfx_clk_below_host_limit_power_accumulated[i] =
            1234 + distr(gen) - distr(gen);
        stats->violation_stats.gfx_clk_below_host_limit_thermal_accumulated[i] =
            2345 + distr(gen) - distr(gen);
        stats->violation_stats.gfx_low_utilization_accumulated[i] =
            3456 + distr(gen) - distr(gen);
        stats->violation_stats.gfx_clk_below_host_limit_total_accumulated[i] =
            4567 + distr(gen) - distr(gen);
        stats->violation_stats.gfx_clk_below_host_limit_power_percentage[i] =
            distr(gen) % 100;
        stats->violation_stats.gfx_clk_below_host_limit_thermal_percentage[i] =
            distr(gen) % 100;
        stats->violation_stats.gfx_low_utilization_percentage[i] =
            distr(gen) % 100;
        stats->violation_stats.gfx_clk_below_host_limit_total_percentage[i] =
            distr(gen) % 100;
    }
    return SDK_RET_OK;
}

typedef struct gpu_event_cb_ctxt_s {
    aga_event_read_cb_t cb;
    void *ctxt;
} gpu_event_cb_ctxt_t;

// generate one event for each GPU
static inline bool
gpu_event_read_cb (void *obj, void *ctxt)
{
    timespec_t ts;
    aga_event_t event = {};
    aga_event_id_t event_id;
    void *event_buffer = event_get();
    gpu_entry *gpu = (gpu_entry *)obj;
    gpu_event_cb_ctxt_t *walk_ctxt = (gpu_event_cb_ctxt_t *)ctxt;

    event_id = event_buffer_get_event_id(event_buffer, 0);

    // get current time
    clock_gettime(CLOCK_REALTIME, &ts);
    // fill the event information
    event.id = event_id;
    event.timestamp = ts;
    event.gpu = gpu->key();
    strncpy(event.message, event_buffer_get_message(event_buffer, 0),
            AGA_MAX_EVENT_STR);
    event.message[AGA_MAX_EVENT_STR] = '\0';
    // call the callback now
    walk_ctxt->cb(&event, walk_ctxt->ctxt);
    return false;
}

sdk_ret_t
event_read (aga_event_read_cb_t cb, void *ctxt)
{
    gpu_event_cb_ctxt_t event_ctxt;

    event_ctxt.cb = cb;
    event_ctxt.ctxt = ctxt;
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
smi_teardown (void)
{
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
smi_gpu_update (aga_gpu_handle_t gpu_handle,
                const aga_obj_key_t *gpu_key,
                aga_gpu_spec_t *spec,
                uint64_t upd_mask)
{
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_device_topology (aga_gpu_handle_t gpu_handle,
                              const aga_obj_key_t *gpu_key,
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

static inline sdk_ret_t
parse_uuid_to_key_and_handle (const std::string& uuid, aga_obj_key_t& key,
                              aga_gpu_handle_t& handle)
{
    uint32_t byte_val;
    uint64_t gpu_handle;
    std::string hex_str;

    // remove "-" from the uuid string
    for (char c : uuid) {
        if (c != '-') {
            hex_str += c;
        }
    }
    if (hex_str.size() < 32) {
        return SDK_RET_ERR;
    }

    // parse the uuid string and construct aga_obj_key_t
    for (uint32_t i = 0; i < OBJ_MAX_KEY_LEN; i++) {
        if (sscanf(hex_str.c_str() + (i * 2), "%2x", &byte_val) != 1) {
            AGA_TRACE_ERR("Invalid GPU UUID {} parsed", uuid.c_str());
            return SDK_RET_ERR;
        }
        key.id[i] = static_cast<char>(byte_val);
    }

    // compute GPU handle from the uuid
    gpu_handle = 0;

    std::string handle_str = hex_str.substr(0, 16);
    if (sscanf(handle_str.c_str(), "%16lx", &gpu_handle) != 1) {
        AGA_TRACE_ERR("Failed to generate GPU handle for GPU {}", uuid.c_str());
        return SDK_RET_ERR;
    }
    handle = aga_gpu_handle_t(gpu_handle);
    return SDK_RET_OK;
}

sdk_ret_t
smi_get_parent_gpu_uuid (aga_gpu_handle_t gpu_handle,
                         const aga_obj_key_t *gpu_key,
                         aga_obj_key_t *parent_key)
{
    *parent_key = g_gpu_map[gpu_handle].key;
    return SDK_RET_OK;
}

sdk_ret_t
parse_gpu_config_file (const char *cfg_file, uint32_t *num_gpu,
                       aga_gpu_profile_t *gpu)
{
    sdk_ret_t ret;
    gpu_cfg_t cfg;
    std::string uuid;
    uint32_t gpu_id = 0;
    boost::property_tree::ptree pt;
    std::string file_path(cfg_file);

    try {
        boost::property_tree::read_json(cfg_file, pt);

        g_num_gpu_mock = pt.get<uint32_t>("gpu_count");
        *num_gpu = g_num_gpu_mock;
        for (const auto& item : pt.get_child("gpu")) {
            uuid = item.second.get<std::string>("uuid");
            cfg.bdf = item.second.get<std::string>("bdf");
            cfg.gpu_id = gpu_id;
            // get aga_obj_key_t and handle from uuid string
            ret = parse_uuid_to_key_and_handle(uuid, cfg.key, cfg.handle);
            if (ret != SDK_RET_OK) {
                return ret;
            }
            // insert into a global map
            g_gpu_map[cfg.handle] = cfg;
            g_gpu_handles[gpu_id] = cfg.handle;
            gpu[gpu_id].handle = cfg.handle;
            gpu[gpu_id].key = cfg.key;
            gpu[gpu_id].id = cfg.gpu_id;
            gpu[gpu_id].partition_capable = true;
            gpu[gpu_id].compute_partition = AGA_GPU_COMPUTE_PARTITION_TYPE_SPX;
            gpu[gpu_id].memory_partition = AGA_GPU_MEMORY_PARTITION_TYPE_NPS1;
            // increment GPU id
            gpu_id++;
        }
    } catch (const std::exception& e) {
        AGA_TRACE_ERR("Unable to parse GPU config file {}", cfg_file);
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_discover_gpus (uint32_t *num_gpu, aga_gpu_profile_t *gpu)
{
    sdk_ret_t ret;
    gpu_cfg_t cfg;
    const char *platform, *cfg_file;
    // this function is called periodically by the device discovery thread; we
    // don't want to generate new GPUs each time this function is called
    static bool gpu_gen_completed = false;

     if (!num_gpu) {
        return SDK_RET_ERR;
    }

    // if env variable GPU_CONFIG_FILE is set, we get the number of GPUs, their
    // uuids and BDFs from it and derive the GPU handle from the uuid; this
    // supercedes PLATFORM env
    cfg_file = std::getenv("GPU_CONFIG_FILE");
    if (cfg_file != NULL) {
        ret = parse_gpu_config_file(cfg_file, num_gpu, gpu);
        if (ret == SDK_RET_OK) {
            return ret;
        }
    }

    // the number of GPUs depends on the platform being mocked; if the platform
    // is helios the number of GPUs will be 4 otherwise it will be 16 (default)
    platform = std::getenv("PLATFORM");
    if (platform != NULL) {
        if (strcmp(platform, "helios") == 0) {
            g_num_gpu_mock = 4;
        }
    }

    *num_gpu = AGA_MOCK_NUM_GPU;

    // generate unique ids for each GPU
    if (!gpu_gen_completed) {
        gpu_gen_unique_ids();
        gpu_gen_completed = true;
    }

    // set GPU ids
    for (uint32_t i = 0; i < *num_gpu; i++) {
        gpu[i].handle = gpu_get_handle(i);
        // set GPU uuids
        gpu[i].key = gpu_uuid(i, gpu_get_unique_id(i));
        // set GPU ids
        gpu[i].id = i;
        // set partition information
        gpu[i].partition_capable = true;
        gpu[i].compute_partition = AGA_GPU_COMPUTE_PARTITION_TYPE_SPX;
        gpu[i].memory_partition = AGA_GPU_MEMORY_PARTITION_TYPE_NPS1;
        // construct GPU config and add to map only once
        if (!gpu_gen_completed) {
            cfg.key = gpu[i].key;
            cfg.gpu_id = gpu[i].id;
            cfg.handle = gpu[i].handle;
            cfg.bdf = "0000:" + std::to_string(cfg.gpu_id + 1) + ":00.0";
            g_gpu_map[cfg.handle] = cfg;
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

sdk_ret_t
smi_gpu_get_cper_entries (aga_gpu_handle_t gpu_handle,
                          const aga_obj_key_t *gpu_key,
                          aga_cper_severity_t severity, aga_cper_info_t *info)
{
    uint64_t handle_val;
    std::ostringstream oss;
    auto cper_entry = &info->cper_entry[info->num_cper_entry++];

    handle_val = (uint64_t)gpu_handle;
    oss << (handle_val % 8) + 1 << ":" << (handle_val + 5) % 8 + 1;
    cper_entry->record_id = oss.str();
    cper_entry->severity = AGA_CPER_SEVERITY_FATAL;
    cper_entry->revision = 256;

    oss.str("");
    oss << std::setfill('0') << "2025-09-" << std::setw(2) <<
        (handle_val % 31) + 1 << " 15:00:" << std::setw(2) << (handle_val % 60) + 1;
    cper_entry->timestamp = oss.str();
    cper_entry->notification_type = AGA_CPER_NOTIFICATION_TYPE_MCE;
    cper_entry->creator_id = "amdgpu";
    cper_entry->num_af_id = 1;
    cper_entry->af_id[0] = 30;
    return SDK_RET_OK;
}

}    // namespace aga
