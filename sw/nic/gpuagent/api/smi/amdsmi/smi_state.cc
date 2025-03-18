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
#include "nic/third-party/rocm/amd_smi_lib/include/amd_smi/amdsmi.h"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/core/aga_core.hpp"
#include "nic/gpuagent/core/ipc_msg.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/smi/smi_state.hpp"
#include "nic/gpuagent/api/smi/smi_watch.hpp"
#include "nic/gpuagent/api/smi/amdsmi/smi_utils.hpp"

using std::vector;

/// \defgroup AGA_SMI_STATE global state for smi interactions
/// @{

namespace event = sdk::event_thread;

/// initial delay after which event monitoring starts (in seconds)
#define AGA_SMI_EVENT_MONITOR_START_DELAY    10.0
/// event monitoring frequency (in seconds)
#define AGA_SMI_EVENT_MONITOR_INTERVAL       3.0
/// all amdsmi events of interest
#define AMDSMI_EVENT_MASK_ALL                  \
            ((1 << AMDSMI_EVT_NOTIF_VMFAULT)          |    \
             (1 << AMDSMI_EVT_NOTIF_THERMAL_THROTTLE) |    \
             (1 << AMDSMI_EVT_NOTIF_GPU_PRE_RESET)    |    \
             (1 << AMDSMI_EVT_NOTIF_GPU_POST_RESET)   |    \
             (1 << AMDSMI_EVT_NOTIF_RING_HANG))
/// timeout to wait to gather outstanding events (in milliseconds)
#define AMDSMI_EVENT_NTFN_TIMEOUT              0

/// initial delay after which watch field update starts
#define AGA_WATCHER_START_DELAY            10.0
/// watch field frequency (in seconds)
#define AGA_WATCHER_INTERVAL               1.0
/// watcher gpu group name
#define AGA_WATCHER_GPU_GROUP_NAME         "AGA_GPU_GROUP"
/// watcher field group name
#define AGA_WATCHER_FIELD_GROUP_NAME       "AGA_FIELD_GROUP"
/// update frequency of the watch fields in auto mode (micro seconds)
#define AGA_WATCHER_UPDATE_FREQUENCY_IN_MS 1000000
/// max age time in seconds for a field value after update
#define AGA_WATCHER_MAX_KEEP_AGE           60
/// max samples of a field value
#define AGA_WATCHER_MAX_KEEP_SAMPLES       10
/// gpu watch subscriber notify frequency (in seconds)
#define AGA_WATCHER_GPU_WATCH_UPDATE_FREQ  5

namespace aga {

/// global singleton smi state class instance
smi_state g_smi_state;

/// vector of all watchable GPU attrs
static std::vector<aga_gpu_watch_attr_id_t> g_watch_field_list;

static void
smi_watch_field_list_init (void)
{
    // initialize GPU watchable attrs list
    // these attributes can be obtained by calls to amdsmi library without any
    // pre-reqs
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_GPU_CLOCK);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_MEM_CLOCK);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_MEMORY_TEMP);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_GPU_TEMP);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_POWER_USAGE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_PCIE_TX);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_PCIE_RX);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_PCIE_BANDWIDTH);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_GPU_UTIL);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_CORRECT_TOTAL);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_UNCORRECT_TOTAL);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_SDMA_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_SDMA_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_GFX_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_GFX_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_MMHUB_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_MMHUB_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_ATHUB_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_ATHUB_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_HDP_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_HDP_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_DF_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_DF_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_SMN_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_SMN_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_SEM_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_SEM_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_MP0_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_MP0_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_MP1_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_MP1_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_FUSE_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_FUSE_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_UMC_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_UMC_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_MCA_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_MCA_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_VCN_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_VCN_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_JPEG_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_JPEG_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_IH_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_IH_UE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_MPIO_CE);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_ECC_MPIO_UE);
    // the following attributes require that a counter is created and started
    // before amdsmi API calls can be used to read their values; counter is
    // created and started during watcher initialization
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_XGMI_0_NOP_TX);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_XGMI_0_REQ_TX);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_XGMI_0_RESP_TX);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_XGMI_0_BEATS_TX);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_XGMI_1_NOP_TX);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_XGMI_1_REQ_TX);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_XGMI_1_RESP_TX);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_XGMI_1_BEATS_TX);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_XGMI_0_THRPUT);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_XGMI_1_THRPUT);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_XGMI_2_THRPUT);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_XGMI_3_THRPUT);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_XGMI_4_THRPUT);
    g_watch_field_list.push_back(AGA_GPU_WATCH_ATTR_ID_XGMI_5_THRPUT);
}

sdk_ret_t
smi_state::smi_watcher_update_all_watch_fields_(uint32_t gpu_id,
               amdsmi_processor_handle gpu_handle,
               aga_gpu_watch_db_t *watch_db) {
    double time_sec;
    uint64_t counter_key;
    int64_t int64_val = 0;
    amdsmi_error_count_t ec;
    uint64_t uint64_val = 0;
    amdsmi_clk_type_t clk_type;
    amdsmi_status_t amdsmi_ret;
    bool bulk_get_succeeded = false;
    amdsmi_pcie_info_t pcie_info = { 0 };
    uint64_t total_correctable_count = 0;
    amdsmi_temperature_type_t sensor_type;
    uint64_t total_uncorrectable_count = 0;
    amdsmi_frequencies_t freq_info = { 0 };
    amdsmi_power_info_t power_info = { 0 };
    amdsmi_engine_usage_t usage_info = { 0 };
    amdsmi_gpu_metrics_t gpu_metrics = { 0 };
    uint64_t pcie_tx = 0, pcie_rx = 0;
    amdsmi_counter_value_t counter_value = { 0 };

    watch_db->watch_info[gpu_id] = { 0 };

    // get GPU metrics, which can be used to bulk fill a few fields
    amdsmi_ret = amdsmi_get_gpu_metrics_info(gpu_handle, &gpu_metrics);
    if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
        // mark bulk get as succeeded
        bulk_get_succeeded = true;
    }

    // get correctable and uncorrectable total error count beforehand
    for (uint32_t b = AMDSMI_GPU_BLOCK_FIRST; b <= AMDSMI_GPU_BLOCK_LAST;
         b = b * 2) {
        // initialize ec to all 0s
        ec = { 0 };
        amdsmi_ret = amdsmi_get_gpu_ecc_count(gpu_handle,
                                              (amdsmi_gpu_block_t)(b), &ec);
        if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
            total_correctable_count += ec.correctable_count;
            total_uncorrectable_count += ec.uncorrectable_count;
            switch (b) {
            case AMDSMI_GPU_BLOCK_UMC:
                watch_db->watch_info[gpu_id].umc_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].umc_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_SDMA:
                watch_db->watch_info[gpu_id].sdma_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].sdma_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_GFX:
                watch_db->watch_info[gpu_id].gfx_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].gfx_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_MMHUB:
                watch_db->watch_info[gpu_id].mmhub_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].mmhub_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_ATHUB:
                watch_db->watch_info[gpu_id].athub_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].athub_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_PCIE_BIF:
                watch_db->watch_info[gpu_id].bif_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].bif_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_HDP:
                watch_db->watch_info[gpu_id].hdp_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].hdp_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_XGMI_WAFL:
                watch_db->watch_info[gpu_id].xgmi_wafl_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].xgmi_wafl_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_DF:
                watch_db->watch_info[gpu_id].df_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].df_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_SMN:
                watch_db->watch_info[gpu_id].smn_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].smn_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_SEM:
                watch_db->watch_info[gpu_id].sem_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].sem_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_MP0:
                watch_db->watch_info[gpu_id].mp0_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].mp0_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_MP1:
                watch_db->watch_info[gpu_id].mp1_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].mp1_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_FUSE:
                watch_db->watch_info[gpu_id].fuse_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].fuse_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_MCA:
                watch_db->watch_info[gpu_id].mca_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].mca_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_VCN:
                watch_db->watch_info[gpu_id].vcn_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].vcn_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_JPEG:
                watch_db->watch_info[gpu_id].jpeg_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].jpeg_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_IH:
                watch_db->watch_info[gpu_id].ih_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].ih_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            case AMDSMI_GPU_BLOCK_MPIO:
                watch_db->watch_info[gpu_id].mpio_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].mpio_uncorrectable_errors =
                    ec.uncorrectable_count;
                break;
            default:
                break;
            }
        }
    }

    // loop through all watch fields
    for (uint32_t i = 0; i < g_watch_field_list.size(); i++) {
        switch (g_watch_field_list[i]) {
        case AGA_GPU_WATCH_ATTR_ID_GPU_CLOCK:
            if (bulk_get_succeeded) {
                // GPU clock frequency in MHz
                watch_db->watch_info[gpu_id].gpu_clock = gpu_metrics.current_gfxclk;
            } else {
                clk_type = AMDSMI_CLK_TYPE_SYS;
                // get clock frequency
                amdsmi_ret = amdsmi_get_clk_freq(gpu_handle, clk_type, &freq_info);
                if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                    watch_db->watch_info[gpu_id].gpu_clock =
                        freq_info.frequency[freq_info.current] / 1000000;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_MEM_CLOCK:
            clk_type = AMDSMI_CLK_TYPE_MEM;
            // get clock frequency
            amdsmi_ret = amdsmi_get_clk_freq(gpu_handle, clk_type, &freq_info);
            if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                watch_db->watch_info[gpu_id].memory_clock =
                    freq_info.frequency[freq_info.current] / 1000000;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_MEMORY_TEMP:
            if (bulk_get_succeeded) {
                // GPU memory temperature in celsius
                watch_db->watch_info[gpu_id].memory_temperature =
                    gpu_metrics.temperature_mem;
            } else {
                sensor_type = AMDSMI_TEMPERATURE_TYPE_VRAM;
                // get GPU memory temperature
                amdsmi_ret = amdsmi_get_temp_metric(gpu_handle, sensor_type,
                                 AMDSMI_TEMP_CURRENT, &int64_val);
                if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                    watch_db->watch_info[gpu_id].memory_temperature = int64_val;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_GPU_TEMP:
            sensor_type = AMDSMI_TEMPERATURE_TYPE_EDGE;
            // get GPU temperature
            amdsmi_ret = amdsmi_get_temp_metric(gpu_handle, sensor_type,
                             AMDSMI_TEMP_CURRENT, &int64_val);
            if (amdsmi_ret == AMDSMI_STATUS_NOT_SUPPORTED) {
                // fallback to hotspot temperature as some card may not have
                // edge temperature.
                sensor_type = AMDSMI_TEMPERATURE_TYPE_JUNCTION;
                amdsmi_ret = amdsmi_get_temp_metric(gpu_handle, sensor_type,
                                 AMDSMI_TEMP_CURRENT, &int64_val);
            }
            if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                watch_db->watch_info[gpu_id].gpu_temperature = int64_val;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_POWER_USAGE:
            if (bulk_get_succeeded) {
                // GPU power usage
                if (gpu_metrics.average_socket_power == 65535) {
                    watch_db->watch_info[gpu_id].power_usage =
                        gpu_metrics.current_socket_power;
                } else {
                    watch_db->watch_info[gpu_id].power_usage =
                        gpu_metrics.average_socket_power;
                }
            }
            // power usage was not read from GPU metrics; use other API to read
            if (!watch_db->watch_info[gpu_id].power_usage) {
                amdsmi_ret = amdsmi_get_power_info(gpu_handle, &power_info);
                if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                    if (power_info.average_socket_power != 65535) {
                        watch_db->watch_info[gpu_id].power_usage =
                            power_info.average_socket_power;
                    } else if (power_info.current_socket_power != 65535) {
                        watch_db->watch_info[gpu_id].power_usage =
                            power_info.current_socket_power;
                    }
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_TX:
            watch_db->watch_info[gpu_id].pcie_tx_usage = pcie_tx;
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_RX:
            watch_db->watch_info[gpu_id].pcie_rx_usage = pcie_rx;
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_BANDWIDTH:
            // PCIe bandwidth
            amdsmi_ret = amdsmi_get_pcie_info(gpu_handle, &pcie_info);
            if (unlikely(amdsmi_ret == AMDSMI_STATUS_SUCCESS)) {
                watch_db->watch_info[gpu_id].pcie_bandwidth =
                    pcie_info.pcie_metric.pcie_bandwidth;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_GPU_UTIL:
            if (bulk_get_succeeded) {
                // GPU utilization
                watch_db->watch_info[gpu_id].gpu_util =
                    gpu_metrics.average_gfx_activity;
            } else {
                amdsmi_ret = amdsmi_get_gpu_activity(gpu_handle, &usage_info);
                if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                    watch_db->watch_info[gpu_id].gpu_util =
                        usage_info.gfx_activity;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE:
            amdsmi_ret = amdsmi_get_gpu_memory_usage(gpu_handle,
                                                     AMDSMI_MEM_TYPE_VRAM,
                                                     &uint64_val);
            if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                // convert GPU memory usage from bytes to MB
                watch_db->watch_info[gpu_id].gpu_memory_usage =
                    uint64_val/1024/1024;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_CORRECT_TOTAL:
            watch_db->watch_info[gpu_id].total_correctable_errors =
                total_correctable_count;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_UNCORRECT_TOTAL:
            watch_db->watch_info[gpu_id].total_uncorrectable_errors =
                total_uncorrectable_count;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_NOP_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_0_NOP_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor0_tx_nops =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_REQ_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_0_REQUEST_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor0_tx_requests =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_RESP_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_0_RESPONSE_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor0_tx_responses =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_BEATS_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_0_BEATS_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor0_tx_beats =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_NOP_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_1_NOP_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor1_tx_nops =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_REQ_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_1_REQUEST_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor1_tx_requests =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_RESP_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_1_RESPONSE_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor1_tx_responses =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_BEATS_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_1_BEATS_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor1_tx_beats =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_THRPUT:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_DATA_OUT_0;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    time_sec =
                        (double)(counter_value.time_running) / 1000000000.0;
                    watch_db->watch_info[gpu_id].xgmi_neighbor0_tx_throughput =
                        (counter_value.value * 32) / time_sec;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_THRPUT:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_DATA_OUT_1;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    time_sec =
                        (double)(counter_value.time_running) / 1000000000.0;
                    watch_db->watch_info[gpu_id].xgmi_neighbor1_tx_throughput =
                        (counter_value.value * 32) / time_sec;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_2_THRPUT:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_DATA_OUT_2;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    time_sec =
                        (double)(counter_value.time_running) / 1000000000.0;
                    watch_db->watch_info[gpu_id].xgmi_neighbor2_tx_throughput =
                        (counter_value.value * 32) / time_sec;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_3_THRPUT:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_DATA_OUT_3;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    time_sec =
                        (double)(counter_value.time_running) / 1000000000.0;
                    watch_db->watch_info[gpu_id].xgmi_neighbor3_tx_throughput =
                        (counter_value.value * 32) / time_sec;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_4_THRPUT:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_DATA_OUT_4;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    time_sec =
                        (double)(counter_value.time_running) / 1000000000.0;
                    watch_db->watch_info[gpu_id].xgmi_neighbor4_tx_throughput =
                        (counter_value.value * 32) / time_sec;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_5_THRPUT:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_DATA_OUT_5;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    time_sec =
                        (double)(counter_value.time_running) / 1000000000.0;
                    watch_db->watch_info[gpu_id].xgmi_neighbor5_tx_throughput =
                        (counter_value.value * 32) / time_sec;
                }
            }
            break;
        default:
            break;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::watcher_update_watch_db(aga_gpu_watch_db_t *watch_db) {
    // loop through all gpu devices
    for (uint32_t gpu = 0; gpu < num_gpu_; gpu++) {
        // update watch db
        smi_watcher_update_all_watch_fields_(gpu, gpu_handles_[gpu], watch_db);
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::cleanup_gpu_watch_inactive_subscribers_(
               vector<gpu_watch_subscriber_info_t>& subscribers) {
    sdk_ret_t ret;
    gpu_watch_subscriber_info_t subscriber;
    aga_gpu_watch_client_ctxt_t *client_ctxt;
    set<aga_gpu_watch_client_ctxt_t *> client_set;

    for (auto it = subscribers.begin(); it != subscribers.end(); it++) {
        aga_task_spec_t task_spec = {};

        subscriber = *it;
        auto& client_info =
            gpu_watch_subscriber_db_.gpu_watch_map[subscriber.gpu_watch_id];
        // erase the client
        client_info.client_set.erase(subscriber.client_ctxt);

        // post task to API thread to decrement subscriber refcount

        // NOTE: multiple subscribers can become inactive for a given GPU watch
        // object, post 1 task for each inactive subscriber instead of one
        // task for all inactive subscribers
        task_spec.task = AGA_TASK_GPU_WATCH_SUBSCRIBE_DEL;
        task_spec.subscriber_spec.num_gpu_watch_ids = 1;
        task_spec.subscriber_spec.gpu_watch_ids[0] = subscriber.gpu_watch_id;
        ret = aga_task_create(&task_spec);
        if (unlikely(ret != SDK_RET_OK)) {
            AGA_TRACE_ERR("Failed to create task to decrement subscriber count "
                          "for GPU watch {}, client {}, client ctxt {}",
                          subscriber.gpu_watch_id.str(),
                          subscriber.client_ctxt->client.c_str(),
                          (void *)subscriber.client_ctxt);
        }
        client_set.insert(subscriber.client_ctxt);
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
smi_state::gpu_watch_notify_subscribers(void) {
    sdk_ret_t ret;
    aga_obj_key_t key;
    aga_gpu_watch_info_t info;
    aga_gpu_watch_client_ctxt_t *client_ctxt;
    gpu_watch_subscriber_info_t inactive_subscriber;
    vector<gpu_watch_subscriber_info_t> inactive_subscribers;

    for (auto& it : gpu_watch_subscriber_db_.gpu_watch_map) {
        key = it.first;
        auto& client_info = it.second;

        AGA_TRACE_VERBOSE("GPU watch {} notify subscribers", key.str());
        memset(&info, 0, sizeof(aga_gpu_watch_info_t));
        aga_gpu_watch_read(&key, &info);
        for (auto client_set_it = client_info.client_set.begin();
             client_set_it != client_info.client_set.end();
             client_set_it++) {
             client_ctxt = *client_set_it;
            ret = client_ctxt->write_cb(&info, client_ctxt);
            if (unlikely(ret != SDK_RET_OK)) {
                // add to list of clients not reachable
                inactive_subscriber.gpu_watch_id = info.spec.key;
                inactive_subscriber.client_ctxt = client_ctxt;
                inactive_subscribers.push_back(inactive_subscriber);
            }
        }
    }
    cleanup_gpu_watch_inactive_subscribers_(inactive_subscribers);
    return SDK_RET_OK;
}

static void
watch_timer_cb_ (event::timer_t *timer)
{
    sdk_ret_t ret;
    aga_task_spec_t task_spec = {};
    static uint16_t timer_ticks = 0;

    // get latest values of all watch fields
    g_smi_state.watcher_update_watch_db(&task_spec.watch_db);

    // post task to api thread
    task_spec.task = AGA_TASK_GPU_WATCH_DB_UPDATE;
    ret = aga_task_create(&task_spec);
    if (unlikely(ret != SDK_RET_OK)) {
        AGA_TRACE_ERR("Failed to create GPU watch db update task, err {}",
                      ret());
    }
    // notify the gpu watch subscribers with latest stats once in every
    // <AGA_WATCHER_GPU_WATCH_UPDATE_FREQ> seconds
    timer_ticks += uint16_t(AGA_WATCHER_INTERVAL);
    if (timer_ticks >= AGA_WATCHER_GPU_WATCH_UPDATE_FREQ) {
        timer_ticks = 0;
        g_smi_state.gpu_watch_notify_subscribers();
    }
}

/// \brief process an gpu watch subscribe request from client
/// \param[in] args    pointer to incoming request
/// \return SDK_RET_OK if success or error code in case of failure
sdk_ret_t
smi_state::process_gpu_watch_subscribe_req(
               aga_gpu_watch_subscribe_args_t *args) {
    gpu_watch_client_info_t client_info = {};

    for (size_t i = 0; i < args->gpu_watch_ids.size(); i++) {
        AGA_TRACE_DEBUG("Rcvd GPU watch {} subscribe request, client {}, "
                        "client ctxt {}, stream {}",
                        args->gpu_watch_ids[i].str(),
                        args->client_ctxt->client.c_str(),
                        (void *)args->client_ctxt,
                        (void *)args->client_ctxt->stream);
        auto& gpu_watch_map = gpu_watch_subscriber_db_.gpu_watch_map;
        // check if this watch was of interest to any client
        // already
        auto it = gpu_watch_map.find(args->gpu_watch_ids[i]);
        if (it == gpu_watch_map.end()) {
            // 1st time anyone is subscribing to this gpu watch
            client_info.client_set.insert(args->client_ctxt);
            gpu_watch_map[args->gpu_watch_ids[i]] = client_info;
        } else {
            // atleast one client is already interested in this gpu watch ,
            // check if this particular client already subscribed to this
            // gpu watch group
            auto set_it = it->second.client_set.find(
                    args->client_ctxt);
            if (set_it == it->second.client_set.end()) {
                // this client is a new subscriber for this gpu watch group
                it->second.client_set.insert(args->client_ctxt);
            } else {
                // this client is already subscribed to this gpu watch group
            }
        }
    }
    return SDK_RET_OK;
}

static void
gpu_watch_subscribe_ipc_cb_ (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    sdk_ret_t ret;
    aga_gpu_watch_subscribe_args_t *args;

    args = *(aga_gpu_watch_subscribe_args_t **)msg->data();
    if (args == NULL) {
        AGA_TRACE_ERR("Ignoring NULL GPU watch subscribe request received");
        return;
    }
    ret = g_smi_state.process_gpu_watch_subscribe_req(args);
    sdk::ipc::respond(msg, &ret, sizeof(ret));
}

static sdk_ret_t
gpu_create_counter_ (amdsmi_processor_handle gpu_handle,
                     amdsmi_event_type_t event_type,
                     amdsmi_event_handle_t *counter_handle)
{
    amdsmi_status_t amdsmi_ret;

    amdsmi_ret = amdsmi_gpu_create_counter(gpu_handle, event_type,
                                           counter_handle);
    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
        AGA_TRACE_ERR("Failed to create counter {} on GPU {}, err {}",
                      event_type, gpu_handle, amdsmi_ret);
        return amdsmi_ret_to_sdk_ret(amdsmi_ret);
    } else {
        amdsmi_ret = amdsmi_gpu_control_counter(*counter_handle,
                                                AMDSMI_CNTR_CMD_START,
                                                NULL);
        if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
            AGA_TRACE_ERR("Failed to start counter {} on GPU {}, err {}",
                          event_type, gpu_handle, amdsmi_ret);
            return amdsmi_ret_to_sdk_ret(amdsmi_ret);
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::watcher_init(void) {
    uint32_t counters;
    uint64_t counter_key;
    sdk_ret_t ret = SDK_RET_OK;
    amdsmi_status_t amdsmi_ret;
    amdsmi_event_handle_t counter_handle;

    // initialize watch field list
    smi_watch_field_list_init();

    // create counters for xgmi stats
    for (uint32_t gpu = 0; gpu < num_gpu_; gpu++) {
        // check if xgmi counter groups are supported
        amdsmi_ret = amdsmi_gpu_counter_group_supported(gpu_handles_[gpu],
                                                        AMDSMI_EVNT_GRP_XGMI);
        if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
            AGA_TRACE_ERR("XGMI counter group not supported on GPU {}, ret {}",
                          gpu_handles_[gpu], amdsmi_ret);
            continue;
        }
        // check if atleast 8 counters are available for XGMI coutner group
        amdsmi_ret = amdsmi_get_gpu_available_counters(gpu_handles_[gpu],
                                                       AMDSMI_EVNT_GRP_XGMI,
                                                       &counters);
        if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
            AGA_TRACE_ERR("Counters unavailable for XGMI counter group on "
                          "GPU {}, ret {}", gpu_handles_[gpu], amdsmi_ret);
            continue;
        } else if (counters < 8) {
            AGA_TRACE_ERR("Only {} counters available for XGMI on GPU {}, "
                          "require {} counters", counters, gpu_handles_[gpu],
                          8);
            continue;
        }
        // create XGMI counters
        ret = gpu_create_counter_(gpu_handles_[gpu], AMDSMI_EVNT_XGMI_0_NOP_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_0_NOP_TX;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_0_REQUEST_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_0_REQUEST_TX;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_0_RESPONSE_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_0_RESPONSE_TX;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_0_BEATS_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_0_BEATS_TX;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu], AMDSMI_EVNT_XGMI_1_NOP_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_1_NOP_TX;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_1_REQUEST_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_1_REQUEST_TX;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_1_RESPONSE_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_1_RESPONSE_TX;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_1_BEATS_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_1_BEATS_TX;
            counter_handle_[counter_key] = counter_handle;
        }
    }
    // create counters for xgmi throughput stats
    for (uint32_t gpu = 0; gpu < num_gpu_; gpu++) {
        // check if xgmi counter groups are supported
        amdsmi_ret = amdsmi_gpu_counter_group_supported(gpu_handles_[gpu],
                         AMDSMI_EVNT_GRP_XGMI_DATA_OUT);
        if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
            AGA_TRACE_ERR("XGMI throughput counter group not supported on "
                          "GPU {}, ret {}", gpu_handles_[gpu], amdsmi_ret);
            continue;
        }
        // check if atleast 6 counters are available for XGMI coutner group
        amdsmi_ret = amdsmi_get_gpu_available_counters(gpu_handles_[gpu],
                         AMDSMI_EVNT_GRP_XGMI_DATA_OUT, &counters);
        if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
            AGA_TRACE_ERR("Counters unavailable for XGMI throughput counter "
                          "group on GPU {}, ret {}", gpu_handles_[gpu],
                          amdsmi_ret);
            continue;
        } else if (counters < 6) {
            AGA_TRACE_ERR("Only {} counters available for XGMI throughput on "
                          "GPU {}, require {} counters", counters,
                          gpu_handles_[gpu], 6);
            continue;
        }
        // create XGMI throughput counters
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_DATA_OUT_0,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_DATA_OUT_0;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_DATA_OUT_1,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_DATA_OUT_1;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_DATA_OUT_2,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_DATA_OUT_2;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_DATA_OUT_3,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_DATA_OUT_3;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_DATA_OUT_4,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_DATA_OUT_4;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_DATA_OUT_5,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_DATA_OUT_5;
            counter_handle_[counter_key] = counter_handle;
        }
    }
    return ret;
}

static void
watcher_thread_init_ (void *ctxt)
{
    static event::timer_t watch_timer;

    g_smi_state.watcher_init();
    // register for gpu watch subscribe messages
    sdk::ipc::reg_request_handler(AGA_IPC_MSG_ID_GPU_WATCH_SUBSCRIBE,
                                  gpu_watch_subscribe_ipc_cb_, NULL);
    // start watch timer
    event::timer_init(&watch_timer, watch_timer_cb_,
                      AGA_WATCHER_START_DELAY, AGA_WATCHER_INTERVAL);
    event::timer_start(&watch_timer);
}

static void
watcher_thread_exit_ (void *ctxt)
{
    // TODO: any timer related cleanup required?
}

sdk_ret_t
smi_state::spawn_watcher_thread_(void) {
    watcher_thread_ =
        sdk::event_thread::event_thread::factory(
            "smi-watcher", AGA_THREAD_ID_WATCHER,
            sdk::lib::THREAD_ROLE_CONTROL, 0x0, watcher_thread_init_,
            watcher_thread_exit_, NULL, // message
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            (THREAD_YIELD_ENABLE | THREAD_SYNC_IPC_ENABLE));
    SDK_ASSERT_TRACE_RETURN((watcher_thread_ != NULL), SDK_RET_ERR,
                            "GPU watcher thread create failure");
    watcher_thread_->start(NULL);
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::event_monitor_init(void) {
    amdsmi_status_t status;
    gpu_event_record_t null_event_record = {};

    // initialize the s/w state
    for (uint32_t d = 0; d < num_gpu_; d++) {
        SDK_SPINLOCK_INIT(&gpu_event_db_[gpu_handles_[d]].slock,
                          PTHREAD_PROCESS_SHARED);
    }
    // initialize event monitoring for all the devices
    for (uint32_t d = 0; d < num_gpu_; d++) {
        // initialize the event monitoring for the 1st time for all devices
        status = amdsmi_init_gpu_event_notification(gpu_handles_[d]);
        if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to do event notification initialization, "
                          "GPU {}, err {}", gpu_handles_[d], status);
            return amdsmi_ret_to_sdk_ret(status);
        }
        status = amdsmi_set_gpu_event_notification_mask(gpu_handles_[d],
                                                        AMDSMI_EVENT_MASK_ALL);
        if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to set event notification mask, "
                          "GPU {}, err {}", gpu_handles_[d], status);
            return amdsmi_ret_to_sdk_ret(status);
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::event_monitor_cleanup(void) {
    // stop monitoring
    for (uint32_t d = 0; d < num_gpu_; d++) {
        amdsmi_stop_gpu_event_notification(gpu_handles_[d]);
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
    gpu_entry *gpu;
    aga_event_t event = {};
    aga_event_id_t event_id;
    amdsmi_processor_handle gpu_handle;
    aga_event_client_ctxt_t *client_ctxt;
    aga_event_listener_info_t inactive_listener;
    amdsmi_evt_notification_data_t *event_buffer;
    vector<aga_event_listener_info_t> inactive_listeners;

    event_buffer = (amdsmi_evt_notification_data_t *)event_buffer_;
    // get current time
    clock_gettime(CLOCK_REALTIME, &ts);
    // start processing all the events
    for (uint32_t i = 0; i < num_events; i++) {
        gpu_handle = event_buffer[i].processor_handle;
        gpu = gpu_db()->find(gpu_handle);
        if (gpu == NULL) {
            continue;
        }
        event_id = aga_event_id_from_smi_event_id(event_buffer[i].event);
        auto& event_map = gpu_event_db_[gpu_handle].event_map;

        // lock the event state for this device
        SDK_SPINLOCK_LOCK(&gpu_event_db_[gpu_handle].slock);
        // update our event state
        auto& event_record = event_map[event_id];
        event_record.timestamp = ts;
        strncpy(event_record.message, event_buffer[i].message,
                AGA_MAX_EVENT_STR);
        event_record.message[AGA_MAX_EVENT_STR] = '\0';
        // fill the event record
        event.id = event_id;
        event.timestamp = ts;
        event.gpu = gpu->key();
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
                inactive_listener.gpu_id = gpu->id();
                inactive_listener.event = event_id;
                inactive_listener.client_ctxt = *client_set_it;
                inactive_listeners.push_back(inactive_listener);
            }
        }
        // unlock the event state maintained for this device
        SDK_SPINLOCK_UNLOCK(&gpu_event_db_[gpu_handle].slock);
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
        auto gpu = gpu_db()->find(gpu_handles_[d]);
        if (gpu == NULL) {
            continue;
        }
        auto& event_map = gpu_event_db_[gpu_handles_[d]].event_map;
        // lock the event map for this device
        SDK_SPINLOCK_LOCK(&gpu_event_db_[gpu_handles_[d]].slock);
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
    amdsmi_status_t status;
    uint32_t num_elem = AGA_MAX_GPU * AGA_EVENT_ID_MAX;
    amdsmi_evt_notification_data_t event_ntfn_data[num_elem];

    // get event information
    status = amdsmi_get_gpu_event_notification(AGA_SMI_EVENT_MONITOR_INTERVAL,
                                               &num_elem, event_ntfn_data);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
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
    case AGA_EVENT_ID_RING_HANG:
        return "Test event: GPU command ring hang";
    default:
        break;
    }
    return "Test event: unknown event";
}

sdk_ret_t
smi_state::process_event_gen_req(aga_event_gen_args_t *args) {
    sdk_ret_t ret;
    uint32_t num_elem = 0;
    amdsmi_evt_notification_type_t smi_event;
    amdsmi_evt_notification_data_t event_data[AGA_MAX_GPU * AGA_EVENT_ID_MAX];

    for (size_t i = 0; i < args->gpu_ids.size(); i++) {
        for (size_t e = 0; e < args->events.size(); e++) {
            ret = aga_event_id_to_smi_event_id(args->events[e], &smi_event);
            if (unlikely(ret != SDK_RET_OK)) {
                AGA_TRACE_ERR("Failed to generate event {}, smi event not "
                              "found", args->events[e]);
                return ret;
            }
            event_data[num_elem].event = smi_event;
            event_data[num_elem].processor_handle =
                gpu_handles_[args->gpu_ids[i]];
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
    amdsmi_status_t status;

    // initialize smi library
    status = amdsmi_init(AMDSMI_INIT_AMD_GPUS);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to initialize amd smi library, err {}", status);
        return amdsmi_ret_to_sdk_ret(status);
    }
    // discover gpus
    ret = aga::smi_discover_gpus(&num_gpu_, gpu_handles_, NULL);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    // spawn event monitor thread
    spawn_event_monitor_thread_();
    // spawn watcher thread
    spawn_watcher_thread_();
    return SDK_RET_OK;
}

/// \@}

}    // namespace aga
