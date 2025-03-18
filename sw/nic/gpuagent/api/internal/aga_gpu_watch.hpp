
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
} aga_gpu_watch_fields_t;

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
