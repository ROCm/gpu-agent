//
// Copyright(C) Advanced Micro Devices, Inc. All rights reserved.
//
// You may not use this software and documentation (if any) (collectively,
// the "Materials") except in compliance with the terms and conditions of
// the Software License Agreement included with the Materials or otherwise as
// set forth in writing and signed by you and an authorized signatory of AMD.
// If you do not have a copy of the Software License Agreement, contact your
// AMD representative for a copy.
//
// You agree that you will not reverse engineer or decompile the Materials,
// in whole or in part, except as allowed by applicable law.
//
// THE MATERIALS ARE DISTRIBUTED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OR
// REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//----------------------------------------------------------------------------
///
/// \file
/// smi library state implementation
///
//----------------------------------------------------------------------------

#include <vector>
extern "C" {
#include "nic/third-party/rocm/gim_amd_smi_lib/include/amd_smi/amdsmi.h"
}
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/core/aga_core.hpp"
#include "nic/gpuagent/core/ipc_msg.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/smi/smi_state.hpp"
#include "nic/gpuagent/api/smi/smi_watch.hpp"
#include "nic/gpuagent/api/smi/gimamdsmi/smi_utils.hpp"

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
    int64_t int64_val = 0;
    amdsmi_error_count_t ec;
    amdsmi_clk_type_t clk_type;
    amdsmi_status_t amdsmi_ret;
    uint64_t pcie_tx = 0, pcie_rx = 0;
    amdsmi_clk_info_t clk_info = { 0 };
    amdsmi_pcie_info_t pcie_info = { 0 };
    uint64_t total_correctable_count = 0;
    amdsmi_temperature_type_t sensor_type;
    uint64_t total_uncorrectable_count = 0;
    amdsmi_power_info_t power_info = { 0 };
    amdsmi_engine_usage_t usage_info = { 0 };

    watch_db->watch_info[gpu_id] = { 0 };

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
            clk_type = AMDSMI_CLK_TYPE_SYS;
            // get clock frequency
            amdsmi_ret = amdsmi_get_clock_info(gpu_handle, clk_type, &clk_info);
            if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                watch_db->watch_info[gpu_id].gpu_clock = clk_info.clk;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_MEM_CLOCK:
            clk_type = AMDSMI_CLK_TYPE_MEM;
            // get clock frequency
            amdsmi_ret = amdsmi_get_clock_info(gpu_handle, clk_type, &clk_info);
            if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                watch_db->watch_info[gpu_id].memory_clock = clk_info.clk;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_MEMORY_TEMP:
            sensor_type = AMDSMI_TEMPERATURE_TYPE_VRAM;
            // get GPU memory temperature
            amdsmi_ret = amdsmi_get_temp_metric(gpu_handle, sensor_type,
                             AMDSMI_TEMP_CURRENT, &int64_val);
            if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                watch_db->watch_info[gpu_id].memory_temperature = int64_val;
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
            amdsmi_ret = amdsmi_get_power_info(gpu_handle, 0, &power_info);
            if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                if (power_info.socket_power != 65535) {
                    watch_db->watch_info[gpu_id].power_usage =
                        power_info.socket_power;
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
            amdsmi_ret = amdsmi_get_gpu_activity(gpu_handle, &usage_info);
            if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                watch_db->watch_info[gpu_id].gpu_util =
                    usage_info.gfx_activity;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE:
            // not supported
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
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_REQ_TX:
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_RESP_TX:
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_BEATS_TX:
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_NOP_TX:
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_REQ_TX:
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_RESP_TX:
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_BEATS_TX:
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_THRPUT:
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_THRPUT:
        case AGA_GPU_WATCH_ATTR_ID_XGMI_2_THRPUT:
        case AGA_GPU_WATCH_ATTR_ID_XGMI_3_THRPUT:
        case AGA_GPU_WATCH_ATTR_ID_XGMI_4_THRPUT:
        case AGA_GPU_WATCH_ATTR_ID_XGMI_5_THRPUT:
            // not supported
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

sdk_ret_t
smi_state::watcher_init(void) {
    // initialize watch field list
    smi_watch_field_list_init();

    return SDK_RET_OK;
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

/// \brief callback function to process IPC msg from gRPC thread
///        to handle event subscription requests
/// \param[in] msg    received IPC message
/// \param[in] ctxt   opaque context (used when callback was registered)
static void
event_subscribe_ipc_cb_ (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    sdk_ret_t ret = SDK_RET_INVALID_OP;

    sdk::ipc::respond(msg, &ret, sizeof(ret));
}

sdk_ret_t
smi_state::process_event_gen_req(aga_event_gen_args_t *args) {
    return SDK_RET_INVALID_OP;
}

/// \brief callback function to process IPC msg from gRPC thread
///        to handle event generate requests
/// \param[in] msg    received IPC message
/// \param[in] ctxt   opaque context (used when callback was registered)
static void
event_gen_ipc_cb_ (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    sdk_ret_t ret = SDK_RET_INVALID_OP;

    sdk::ipc::respond(msg, &ret, sizeof(ret));
}

static void
event_monitor_thread_init_ (void *ctxt)
{
    // subscribe to all IPC msgs of interest
    sdk::ipc::reg_request_handler(AGA_IPC_MSG_ID_EVENT_SUBSCRIBE,
                                  event_subscribe_ipc_cb_, NULL);
    sdk::ipc::reg_request_handler(AGA_IPC_MSG_ID_EVENT_GEN,
                                  event_gen_ipc_cb_, NULL);
}

void
event_monitor_thread_exit_ (void *ctxt)
{
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
    aga_gpu_profile_t gpu[AGA_MAX_GPU];

    // initialize smi library
    status = amdsmi_init(AMDSMI_INIT_AMD_GPUS);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to initialize amd smi library, err {}", status);
        return amdsmi_ret_to_sdk_ret(status);
    }
    // discover gpus
    ret = aga::smi_discover_gpus(&num_gpu_, gpu);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    for (uint32_t i = 0; i < num_gpu_; i++) {
        gpu_handles_[i] = gpu[i].handle;
    }
    // spawn event monitor thread
    spawn_event_monitor_thread_();
    // spawn watcher thread
    spawn_watcher_thread_();
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::read_counter (aga_gpu_handle_t gpu_handle, uint64_t counter,
                         uint64_t *value) {
    *value = 0;
    return SDK_RET_OK;
}

/// \@}

}    // namespace aga
