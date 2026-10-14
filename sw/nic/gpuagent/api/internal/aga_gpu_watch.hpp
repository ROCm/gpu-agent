
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
/// this file contains gpu watch db definition
///
//----------------------------------------------------------------------------

#ifndef __INTERNAL_AGA_GPU_WATCH_HPP__
#define __INTERNAL_AGA_GPU_WATCH_HPP__

#include <vector>
#include "nic/gpuagent/api/include/base.hpp"
#include "nic/gpuagent/api/include/aga_gpu_watch.hpp"

using std::vector;

/// \brief watch fields
typedef struct aga_gpu_watch_fields_s {
    /// current GPU clock frequency (in MHz)
    uint64_t gpu_clock;
    /// current memory clock frequency (in MHz)
    uint64_t memory_clock;
    /// current GPU temperature (in C)
    uint64_t gpu_temperature;
    /// current memory temperature (in C)
    uint64_t memory_temperature;
    /// power usage (in Watts)
    uint64_t power_usage;
    /// PCIe Tx utilization (TODO: unit?)
    uint64_t pcie_tx_usage;
    /// PCIe Rx utilization (TODO: unit?)
    uint64_t  pcie_rx_usage;
    /// PCIe bandwidth (in GB/s)
    uint64_t  pcie_bandwidth;
    /// gpu utilization
    uint64_t gpu_util;
    /// gpu umc (memory controller) activity (in %)
    uint64_t umc_activity;
    /// gpu mm (multimedia) activity (in %)
    uint64_t mm_activity;
    /// average socket power (in Watts)
    uint64_t avg_power;
    /// gfx core voltage (in mV)
    uint64_t gfx_voltage;
    /// soc voltage (in mV)
    uint64_t soc_voltage;
    /// memory voltage (in mV)
    uint64_t mem_voltage;
    /// accumulated energy consumed (in uJ)
    uint64_t energy_consumed;
    /// gpu VRAM memory usage
    uint64_t gpu_memory_usage;
    /// gpu total VRM memory
    uint64_t gpu_total_memory;
    /// total correctable errors
    uint64_t total_correctable_errors;
    /// total uncorrectable errors
    uint64_t total_uncorrectable_errors;
    /// SDMA correctable errors
    uint64_t sdma_correctable_errors;
    /// SDMA uncorrectable errors
    uint64_t sdma_uncorrectable_errors;
    /// GFX correctable errors
    uint64_t gfx_correctable_errors;
    /// GFX uncorrectable errors
    uint64_t gfx_uncorrectable_errors;
    /// MMHUB correctable errors
    uint64_t mmhub_correctable_errors;
    /// MMHUB uncorrectable errors
    uint64_t mmhub_uncorrectable_errors;
    /// ATHUB correctable errors
    uint64_t athub_correctable_errors;
    /// ATHUB uncorrectable errors
    uint64_t athub_uncorrectable_errors;
    /// BIF correctable errors
    uint64_t bif_correctable_errors;
    /// BIF uncorrectable errors
    uint64_t bif_uncorrectable_errors;
    /// HDP correctable errors
    uint64_t hdp_correctable_errors;
    /// HDP uncorrectable errors
    uint64_t hdp_uncorrectable_errors;
    /// XGMI WAFL correctable errors
    uint64_t xgmi_wafl_correctable_errors;
    /// XGMI WAFL uncorrectable errors
    uint64_t xgmi_wafl_uncorrectable_errors;
    /// DF correctable errors
    uint64_t df_correctable_errors;
    /// DF uncorrectable errors
    uint64_t df_uncorrectable_errors;
    /// SMN correctable errors
    uint64_t smn_correctable_errors;
    /// SMN uncorrectable errors
    uint64_t smn_uncorrectable_errors;
    /// SEM correctable errors
    uint64_t sem_correctable_errors;
    /// SEM uncorrectable errors
    uint64_t sem_uncorrectable_errors;
    /// MP0 correctable errors
    uint64_t mp0_correctable_errors;
    /// MP0 uncorrectable errors
    uint64_t mp0_uncorrectable_errors;
    /// MP1 correctable errors
    uint64_t mp1_correctable_errors;
    /// MP1 uncorrectable errors
    uint64_t mp1_uncorrectable_errors;
    /// FUSE correctable errors
    uint64_t fuse_correctable_errors;
    /// FUSE uncorrectable errors
    uint64_t fuse_uncorrectable_errors;
    /// UMC correctable errors
    uint64_t umc_correctable_errors;
    /// UMC uncorrectable errors
    uint64_t umc_uncorrectable_errors;
    /// MCA correctable errors
    uint64_t mca_correctable_errors;
    /// MCA uncorrectable errors
    uint64_t mca_uncorrectable_errors;
    /// VCN correctable errors
    uint64_t vcn_correctable_errors;
    /// VCN uncorrectable errors
    uint64_t vcn_uncorrectable_errors;
    /// JPEG correctable errors
    uint64_t jpeg_correctable_errors;
    /// JPEG uncorrectable errors
    uint64_t jpeg_uncorrectable_errors;
    /// IH correctable errors
    uint64_t ih_correctable_errors;
    /// IH uncorrectable errors
    uint64_t ih_uncorrectable_errors;
    /// MPIO correctable errors
    uint64_t mpio_correctable_errors;
    /// MPIO uncorrectable errors
    uint64_t mpio_uncorrectable_errors;
    /// XGMI counters
    /// NOPs sent to neighbor0
    uint64_t xgmi_neighbor0_tx_nops;
    /// outgoing requests to neighbor0
    uint64_t xgmi_neighbor0_tx_requests;
    /// outgoing responses to neighbor0
    uint64_t xgmi_neighbor0_tx_responses;
    /// data beats sent to neighbor0 (each beat = 32 Bytes)
    uint64_t xgmi_neighbor0_tx_beats;
    /// NOPs sent to neighbor1
    uint64_t xgmi_neighbor1_tx_nops;
    /// outgoing requests to neighbor1
    uint64_t xgmi_neighbor1_tx_requests;
    /// outgoing responses to neighbor1
    uint64_t xgmi_neighbor1_tx_responses;
    /// data beats sent to neighbor1 (each beat = 32 Bytes)
    uint64_t xgmi_neighbor1_tx_beats;
    /// transmit throughput to XGMI neighbor 0 (in Bytes per second)
    uint64_t xgmi_neighbor0_tx_throughput;
    /// transmit throughput to XGMI neighbor 1 (in Bytes per second)
    uint64_t xgmi_neighbor1_tx_throughput;
    /// transmit throughput to XGMI neighbor 2 (in Bytes per second)
    uint64_t xgmi_neighbor2_tx_throughput;
    /// transmit throughput to XGMI neighbor 3 (in Bytes per second)
    uint64_t xgmi_neighbor3_tx_throughput;
    /// transmit throughput to XGMI neighbor 4 (in Bytes per second)
    uint64_t xgmi_neighbor4_tx_throughput;
    /// transmit throughput to XGMI neighbor 5 (in Bytes per second)
    uint64_t xgmi_neighbor5_tx_throughput;
    /// array-valued attrs: count + fixed array, element i -> *_index=i
    /// HBM stack temperatures (in C)
    uint32_t hbm_temperature_count;
    uint64_t hbm_temperature[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    /// VCN engine activity (in %)
    uint32_t vcn_activity_count;
    uint64_t vcn_activity[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    /// JPEG engine activity (in %)
    uint32_t jpeg_activity_count;
    uint64_t jpeg_activity[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    /// XGMI per-link accumulated read data (in KB)
    uint32_t xgmi_link_rx_count;
    uint64_t xgmi_link_rx[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    /// XGMI per-link accumulated write data (in KB)
    uint32_t xgmi_link_tx_count;
    uint64_t xgmi_link_tx[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    /// per-XCC gfx busy instantaneous (in %)
    uint32_t gfx_busy_inst_count;
    uint64_t gfx_busy_inst[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    /// per-XCC vcn busy instantaneous (in %)
    uint32_t vcn_busy_inst_count;
    uint64_t vcn_busy_inst[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    /// per-XCC jpeg busy instantaneous (in %)
    uint32_t jpeg_busy_inst_count;
    uint64_t jpeg_busy_inst[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    /// clock status, one entry per (clock_type, instance); slow tier.
    /// clock_index[] matches the pull-path global clk_cnt 1:1 (compacted, sparse)
    uint32_t clock_count;
    uint32_t clock_index[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    uint32_t clock_type[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    uint64_t clock_current[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    uint64_t clock_min[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    uint64_t clock_max[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    /// flat (non-indexed) violation counters, from amdsmi_get_violation_status
    uint64_t viol_current_acc_counter;
    uint64_t viol_proc_hot_residency_acc;
    uint64_t viol_ppt_residency_acc;
    uint64_t viol_socket_thermal_residency_acc;
    uint64_t viol_vr_thermal_residency_acc;
    uint64_t viol_hbm_thermal_residency_acc;
    uint64_t viol_proc_hot_residency_pct;
    uint64_t viol_ppt_residency_pct;
    uint64_t viol_socket_thermal_residency_pct;
    uint64_t viol_vr_thermal_residency_pct;
    uint64_t viol_hbm_thermal_residency_pct;
    /// per-XCC violation counters
    uint32_t viol_gfx_below_power_acc_count;
    uint64_t viol_gfx_below_power_acc[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    uint32_t viol_gfx_below_thermal_acc_count;
    uint64_t viol_gfx_below_thermal_acc[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    uint32_t viol_gfx_low_util_acc_count;
    uint64_t viol_gfx_low_util_acc[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    uint32_t viol_gfx_below_total_acc_count;
    uint64_t viol_gfx_below_total_acc[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    uint32_t viol_gfx_below_power_pct_count;
    uint64_t viol_gfx_below_power_pct[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    uint32_t viol_gfx_below_thermal_pct_count;
    uint64_t viol_gfx_below_thermal_pct[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    uint32_t viol_gfx_low_util_pct_count;
    uint64_t viol_gfx_low_util_pct[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    uint32_t viol_gfx_below_total_pct_count;
    uint64_t viol_gfx_below_total_pct[AGA_GPU_WATCH_MAX_INDEXED_VALS];
    /// ECC deferred error counters (third category alongside CE/UE)
    uint64_t total_deferred_errors;
    uint64_t sdma_deferred_errors;
    uint64_t gfx_deferred_errors;
    uint64_t mmhub_deferred_errors;
    uint64_t athub_deferred_errors;
    uint64_t bif_deferred_errors;
    uint64_t hdp_deferred_errors;
    uint64_t xgmi_wafl_deferred_errors;
    uint64_t df_deferred_errors;
    uint64_t smn_deferred_errors;
    uint64_t sem_deferred_errors;
    uint64_t mp0_deferred_errors;
    uint64_t mp1_deferred_errors;
    uint64_t fuse_deferred_errors;
    uint64_t umc_deferred_errors;
    uint64_t mca_deferred_errors;
    uint64_t vcn_deferred_errors;
    uint64_t jpeg_deferred_errors;
    uint64_t ih_deferred_errors;
    uint64_t mpio_deferred_errors;
    /// PCIe accumulated counters (from the bulk gpu metrics read)
    uint64_t pcie_replay_count;
    uint64_t pcie_recovery_count;
    uint64_t pcie_replay_rollover_count;
    uint64_t pcie_nack_sent_count;
    uint64_t pcie_nack_received_count;
    /// additional bulk scalars
    uint64_t gpu_power_usage;
    uint64_t vram_max_bandwidth;
    uint64_t edge_temperature;
    /// memory totals/used/free (in MB) from dedicated amdsmi calls
    uint64_t total_vram;
    uint64_t total_visible_vram;
    uint64_t total_gtt;
    uint64_t used_visible_vram;
    uint64_t used_gtt;
    uint64_t free_vram;
    uint64_t free_visible_vram;
    uint64_t free_gtt;
    /// PCIe link speed (in GT/s) from amdsmi_get_pcie_info
    uint64_t pcie_speed;
    uint64_t pcie_max_speed;
} aga_gpu_watch_fields_t;

/// \brief    reset the fast-tier fields of a watch record to the NA sentinel
/// fast-tier fields are re-read every watcher tick, so they must start at NA:
/// a failed read then reports not-applicable instead of a stale value carried
/// forward from the last slow refresh. slow-tier fields are untouched here so
/// counters keep carrying forward on ticks that skip the slow reads.
static inline void
aga_gpu_watch_reset_fast_fields (aga_gpu_watch_fields_t *wi)
{
    // scalars filled from the per-tick bulk gpu metrics read
    wi->gpu_clock = UINT64_MAX;
    wi->memory_clock = UINT64_MAX;
    wi->gpu_temperature = UINT64_MAX;
    wi->memory_temperature = UINT64_MAX;
    wi->power_usage = UINT64_MAX;
    wi->gpu_util = UINT64_MAX;
    wi->umc_activity = UINT64_MAX;
    wi->mm_activity = UINT64_MAX;
    wi->avg_power = UINT64_MAX;
    wi->gfx_voltage = UINT64_MAX;
    wi->soc_voltage = UINT64_MAX;
    wi->mem_voltage = UINT64_MAX;
    wi->energy_consumed = UINT64_MAX;
    wi->gpu_power_usage = UINT64_MAX;
    wi->vram_max_bandwidth = UINT64_MAX;
    wi->edge_temperature = UINT64_MAX;
    wi->pcie_replay_count = UINT64_MAX;
    wi->pcie_recovery_count = UINT64_MAX;
    wi->pcie_replay_rollover_count = UINT64_MAX;
    wi->pcie_nack_sent_count = UINT64_MAX;
    wi->pcie_nack_received_count = UINT64_MAX;
    // array-valued fast attrs: empty until (re)populated this tick
    wi->hbm_temperature_count = 0;
    wi->vcn_activity_count = 0;
    wi->jpeg_activity_count = 0;
    wi->xgmi_link_rx_count = 0;
    wi->xgmi_link_tx_count = 0;
    wi->gfx_busy_inst_count = 0;
    wi->vcn_busy_inst_count = 0;
    wi->jpeg_busy_inst_count = 0;
}

typedef struct aga_gpu_watch_db_s {
    aga_gpu_watch_fields_t watch_info[AGA_MAX_GPU];
} aga_gpu_watch_db_t;

/// \brief    GPU watch client context sent from frontend to backend
typedef struct aga_gpu_watch_client_ctxt_s {
    /// client IP address and port
    std::string client;
    /// client_inactive is set to true by the backend thread when
    /// gRPC client is no longer active
    bool client_inactive;
    /// condition variable the front end gRPC thread is going to wait on
    pthread_cond_t cond;
    /// mutex lock to keep the gRPC thread alive
    pthread_mutex_t lock;
    /// opaque context sent from gRPC thread to backend
    /// NOTE:
    /// gRPC response stream to periodically publish watch attributes to
    void *stream;
    /// callback API to write gpu watch info to the client stream
    aga_gpu_watch_cb_t write_cb;
} aga_gpu_watch_client_ctxt_t;

/// \brief    GPU watch subscribe request from a gRPC client
typedef struct aga_gpu_watch_subscribe_args_s {
    /// gRPC client context
    aga_gpu_watch_client_ctxt_t *client_ctxt;
    /// list of gpu watch uuids
    vector<aga_obj_key_t> gpu_watch_ids;
} aga_gpu_watch_subscribe_args_t;

#endif    // __INTERNAL_AGA_GPU_WATCH_HPP__
