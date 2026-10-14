
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
/// This module defines protobuf conversion APIs for gpu watch object
///
//----------------------------------------------------------------------------

#ifndef __AGA_SVC_GPU_WATCH_TO_PROTO_HPP__
#define __AGA_SVC_GPU_WATCH_TO_PROTO_HPP__

#include <string>
#include "nic/gpuagent/svc/gpu_watch.hpp"
#include "nic/gpuagent/api/include/aga_gpu_watch.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"

// map a clock-array sub_type (aga_gpu_clock_type_t) to the clock_type label the
// exporter emits verbatim. These are the pull path's normalized values
// (NormalizeStringWithoutPrefix of GPU_CLOCK_TYPE_*: prefix stripped, lowered),
// so stream and pull clock_type labels match 1:1. "" means no qualifier.
static inline const char *
aga_gpu_watch_clock_type_proto_name (uint32_t sub_type)
{
    switch (sub_type) {
    case AGA_GPU_CLOCK_TYPE_FABRIC:
        return "fabric";
    case AGA_GPU_CLOCK_TYPE_MEMORY:
        return "memory";
    case AGA_GPU_CLOCK_TYPE_SYSTEM:
        return "system";
    case AGA_GPU_CLOCK_TYPE_SOC:
        return "soc";
    case AGA_GPU_CLOCK_TYPE_DCE:
        return "dce";
    case AGA_GPU_CLOCK_TYPE_PCIE:
        return "pcie";
    case AGA_GPU_CLOCK_TYPE_VIDEO:
        return "video";
    case AGA_GPU_CLOCK_TYPE_DATA:
        return "data";
    default:
        return "";
    }
}

// function to return units for GPU watch attribute
static inline std::string
aga_gpu_watch_attr_id_to_units (aga_gpu_watch_attr_id_t id)
{
    switch (id) {
    case AGA_GPU_WATCH_ATTR_ID_GPU_CLOCK:
        return std::string("MHz");
    case AGA_GPU_WATCH_ATTR_ID_MEM_CLOCK:
        return std::string("MHz");
    case AGA_GPU_WATCH_ATTR_ID_MEMORY_TEMP:
        return std::string("C");
    case AGA_GPU_WATCH_ATTR_ID_GPU_TEMP:
        return std::string("C");
    case AGA_GPU_WATCH_ATTR_ID_POWER_USAGE:
        return std::string("Watts");
    case AGA_GPU_WATCH_ATTR_ID_PCIE_BANDWIDTH:
        return std::string("Mb/s");
    case AGA_GPU_WATCH_ATTR_ID_GPU_UTIL:
    case AGA_GPU_WATCH_ATTR_ID_UMC_ACTIVITY:
    case AGA_GPU_WATCH_ATTR_ID_MM_ACTIVITY:
        return std::string("%");
    case AGA_GPU_WATCH_ATTR_ID_AVG_POWER:
        return std::string("Watts");
    case AGA_GPU_WATCH_ATTR_ID_GFX_VOLTAGE:
    case AGA_GPU_WATCH_ATTR_ID_SOC_VOLTAGE:
    case AGA_GPU_WATCH_ATTR_ID_MEM_VOLTAGE:
        return std::string("mV");
    case AGA_GPU_WATCH_ATTR_ID_ENERGY_CONSUMED:
        return std::string("uJ");
    case AGA_GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE:
    case AGA_GPU_WATCH_ATTR_ID_TOTAL_VRAM:
    case AGA_GPU_WATCH_ATTR_ID_TOTAL_VISIBLE_VRAM:
    case AGA_GPU_WATCH_ATTR_ID_TOTAL_GTT:
    case AGA_GPU_WATCH_ATTR_ID_USED_VISIBLE_VRAM:
    case AGA_GPU_WATCH_ATTR_ID_USED_GTT:
    case AGA_GPU_WATCH_ATTR_ID_FREE_VRAM:
    case AGA_GPU_WATCH_ATTR_ID_FREE_VISIBLE_VRAM:
    case AGA_GPU_WATCH_ATTR_ID_FREE_GTT:
        return std::string("MB");
    case AGA_GPU_WATCH_ATTR_ID_HBM_TEMP:
    case AGA_GPU_WATCH_ATTR_ID_EDGE_TEMP:
        return std::string("C");
    case AGA_GPU_WATCH_ATTR_ID_GPU_POWER_USAGE:
        return std::string("Watts");
    case AGA_GPU_WATCH_ATTR_ID_VCN_ACTIVITY:
    case AGA_GPU_WATCH_ATTR_ID_JPEG_ACTIVITY:
    case AGA_GPU_WATCH_ATTR_ID_GFX_BUSY_INST:
    case AGA_GPU_WATCH_ATTR_ID_VCN_BUSY_INST:
    case AGA_GPU_WATCH_ATTR_ID_JPEG_BUSY_INST:
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_PCT:
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_PCT:
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_PCT:
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_PCT:
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_PCT:
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_PCT:
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_PCT:
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_PCT:
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_PCT:
        return std::string("%");
    case AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_CURRENT:
    case AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_MIN:
    case AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_MAX:
        return std::string("MHz");
    case AGA_GPU_WATCH_ATTR_ID_XGMI_LINK_RX:
    case AGA_GPU_WATCH_ATTR_ID_XGMI_LINK_TX:
        return std::string("KB");
    case AGA_GPU_WATCH_ATTR_ID_VRAM_MAX_BANDWIDTH:
        return std::string("GB/s");
    case AGA_GPU_WATCH_ATTR_ID_PCIE_SPEED:
    case AGA_GPU_WATCH_ATTR_ID_PCIE_MAX_SPEED:
        return std::string("GT/s");
    default:
        // ECC/PCIe/violation accumulated counters are dimensionless counts
        return std::string("");
    }
}

// convert gpu watch attrs id to proto
static inline GPUWatchAttrId
aga_gpu_watch_attr_id_to_proto (aga_gpu_watch_attr_id_t id)
{
    switch (id) {
    case AGA_GPU_WATCH_ATTR_ID_GPU_CLOCK:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_GPU_CLOCK;
    case AGA_GPU_WATCH_ATTR_ID_MEM_CLOCK:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_MEM_CLOCK;
    case AGA_GPU_WATCH_ATTR_ID_MEMORY_TEMP:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_MEMORY_TEMP;
    case AGA_GPU_WATCH_ATTR_ID_GPU_TEMP:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_GPU_TEMP;
    case AGA_GPU_WATCH_ATTR_ID_POWER_USAGE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_POWER_USAGE;
    case AGA_GPU_WATCH_ATTR_ID_PCIE_TX:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_TX;
    case AGA_GPU_WATCH_ATTR_ID_PCIE_RX:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_RX;
    case AGA_GPU_WATCH_ATTR_ID_PCIE_BANDWIDTH:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_BANDWIDTH;
    case AGA_GPU_WATCH_ATTR_ID_GPU_UTIL:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_GPU_UTIL;
    case AGA_GPU_WATCH_ATTR_ID_UMC_ACTIVITY:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_UMC_ACTIVITY;
    case AGA_GPU_WATCH_ATTR_ID_MM_ACTIVITY:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_MM_ACTIVITY;
    case AGA_GPU_WATCH_ATTR_ID_AVG_POWER:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_AVG_POWER;
    case AGA_GPU_WATCH_ATTR_ID_GFX_VOLTAGE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_GFX_VOLTAGE;
    case AGA_GPU_WATCH_ATTR_ID_SOC_VOLTAGE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_SOC_VOLTAGE;
    case AGA_GPU_WATCH_ATTR_ID_MEM_VOLTAGE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_MEM_VOLTAGE;
    case AGA_GPU_WATCH_ATTR_ID_ENERGY_CONSUMED:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ENERGY_CONSUMED;
    case AGA_GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_CORRECT_TOTAL:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_CORRECT_TOTAL;
    case AGA_GPU_WATCH_ATTR_ID_ECC_UNCORRECT_TOTAL:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_UNCORRECT_TOTAL;
    case AGA_GPU_WATCH_ATTR_ID_ECC_SDMA_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_SDMA_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_SDMA_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_SDMA_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_GFX_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_GFX_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_GFX_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_GFX_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_MMHUB_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MMHUB_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_MMHUB_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MMHUB_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_ATHUB_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_ATHUB_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_ATHUB_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_ATHUB_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_HDP_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_HDP_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_HDP_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_HDP_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DF_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DF_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DF_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DF_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_SMN_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_SMN_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_SMN_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_SMN_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_SEM_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_SEM_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_SEM_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_SEM_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_MP0_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MP0_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_MP0_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MP0_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_MP1_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MP1_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_MP1_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MP1_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_FUSE_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_FUSE_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_FUSE_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_FUSE_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_UMC_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_UMC_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_UMC_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_UMC_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_MCA_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MCA_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_MCA_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MCA_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_VCN_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_VCN_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_VCN_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_VCN_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_JPEG_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_JPEG_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_JPEG_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_JPEG_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_IH_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_IH_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_IH_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_IH_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_MPIO_CE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MPIO_CE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_MPIO_UE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MPIO_UE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_TOTAL:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_TOTAL;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SDMA:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_SDMA;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_GFX:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_GFX;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MMHUB:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_MMHUB;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_ATHUB:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_ATHUB;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_PCIE_BIF:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_PCIE_BIF;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_HDP:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_HDP;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_XGMI_WAFL:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_XGMI_WAFL;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_DF:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_DF;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SMN:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_SMN;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SEM:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_SEM;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MP0:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_MP0;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MP1:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_MP1;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_FUSE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_FUSE;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_UMC:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_UMC;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MCA:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_MCA;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_VCN:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_VCN;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_JPEG:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_JPEG;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_IH:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_IH;
    case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MPIO:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_MPIO;
    case AGA_GPU_WATCH_ATTR_ID_PCIE_REPLAY_COUNT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_REPLAY_COUNT;
    case AGA_GPU_WATCH_ATTR_ID_PCIE_RECOVERY_COUNT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_RECOVERY_COUNT;
    case AGA_GPU_WATCH_ATTR_ID_PCIE_REPLAY_ROLLOVER_COUNT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_REPLAY_ROLLOVER_COUNT;
    case AGA_GPU_WATCH_ATTR_ID_PCIE_NACK_SENT_COUNT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_NACK_SENT_COUNT;
    case AGA_GPU_WATCH_ATTR_ID_PCIE_NACK_RECEIVED_COUNT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_NACK_RECEIVED_COUNT;
    case AGA_GPU_WATCH_ATTR_ID_GPU_POWER_USAGE:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_GPU_POWER_USAGE;
    case AGA_GPU_WATCH_ATTR_ID_VRAM_MAX_BANDWIDTH:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VRAM_MAX_BANDWIDTH;
    case AGA_GPU_WATCH_ATTR_ID_EDGE_TEMP:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_EDGE_TEMP;
    case AGA_GPU_WATCH_ATTR_ID_TOTAL_VRAM:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_TOTAL_VRAM;
    case AGA_GPU_WATCH_ATTR_ID_TOTAL_VISIBLE_VRAM:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_TOTAL_VISIBLE_VRAM;
    case AGA_GPU_WATCH_ATTR_ID_TOTAL_GTT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_TOTAL_GTT;
    case AGA_GPU_WATCH_ATTR_ID_USED_VISIBLE_VRAM:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_USED_VISIBLE_VRAM;
    case AGA_GPU_WATCH_ATTR_ID_USED_GTT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_USED_GTT;
    case AGA_GPU_WATCH_ATTR_ID_FREE_VRAM:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_FREE_VRAM;
    case AGA_GPU_WATCH_ATTR_ID_FREE_VISIBLE_VRAM:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_FREE_VISIBLE_VRAM;
    case AGA_GPU_WATCH_ATTR_ID_FREE_GTT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_FREE_GTT;
    case AGA_GPU_WATCH_ATTR_ID_PCIE_SPEED:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_SPEED;
    case AGA_GPU_WATCH_ATTR_ID_PCIE_MAX_SPEED:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_MAX_SPEED;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_0_NOP_TX:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_0_NOP_TX;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_0_REQ_TX:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_0_REQ_TX;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_0_RESP_TX:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_0_RESP_TX;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_0_BEATS_TX:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_0_BEATS_TX;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_1_NOP_TX:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_1_NOP_TX;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_1_REQ_TX:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_1_REQ_TX;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_1_RESP_TX:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_1_RESP_TX;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_1_BEATS_TX:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_1_BEATS_TX;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_0_THRPUT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_0_THRPUT;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_1_THRPUT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_1_THRPUT;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_2_THRPUT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_2_THRPUT;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_3_THRPUT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_3_THRPUT;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_4_THRPUT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_4_THRPUT;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_5_THRPUT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_5_THRPUT;
    case AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_CURRENT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_CLOCK_FREQ_CURRENT;
    case AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_MIN:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_CLOCK_FREQ_MIN;
    case AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_MAX:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_CLOCK_FREQ_MAX;
    case AGA_GPU_WATCH_ATTR_ID_HBM_TEMP:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_HBM_TEMP;
    case AGA_GPU_WATCH_ATTR_ID_VCN_ACTIVITY:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VCN_ACTIVITY;
    case AGA_GPU_WATCH_ATTR_ID_JPEG_ACTIVITY:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_JPEG_ACTIVITY;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_LINK_RX:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_LINK_RX;
    case AGA_GPU_WATCH_ATTR_ID_XGMI_LINK_TX:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_LINK_TX;
    case AGA_GPU_WATCH_ATTR_ID_GFX_BUSY_INST:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_GFX_BUSY_INST;
    case AGA_GPU_WATCH_ATTR_ID_VCN_BUSY_INST:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VCN_BUSY_INST;
    case AGA_GPU_WATCH_ATTR_ID_JPEG_BUSY_INST:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_JPEG_BUSY_INST;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_CURRENT_ACC_COUNTER:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_CURRENT_ACC_COUNTER;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_ACC:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_ACC;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_ACC:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_ACC;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_ACC:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_ACC;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_ACC:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_ACC;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_ACC:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_ACC;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_PCT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_PCT;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_PCT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_PCT;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_PCT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_PCT;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_PCT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_PCT;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_PCT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_PCT;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_ACC:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_ACC;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_ACC:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_ACC;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_ACC:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_ACC;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_ACC:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_ACC;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_PCT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_PCT;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_PCT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_PCT;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_PCT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_PCT;
    case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_PCT:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_PCT;
    default:
        return amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_INVALID;
    }
}

// populate proto buf spec from gpu watch API spec
static inline void
aga_gpu_watch_spec_to_proto (amdgpu::GPUWatchSpec *proto_spec,
                             const aga_gpu_watch_spec_t *spec)
{
    proto_spec->set_id(spec->key.id, OBJ_MAX_KEY_LEN);
    for (uint32_t i = 0; i < spec->num_gpu; i++) {
        proto_spec->add_gpu(spec->gpu[i].id, OBJ_MAX_KEY_LEN);
    }
    for (uint32_t i = 0; i < spec->num_attrs; i++) {
        proto_spec->add_attribute(aga_gpu_watch_attr_id_to_proto(
                                      spec->attr_id[i]));
    }
}

// populate proto buf status from gpu watch API status
static inline void
aga_gpu_watch_status_to_proto (amdgpu::GPUWatchStatus *proto_status,
                               const aga_gpu_watch_status_t *status)
{
    proto_status->set_numsubscribers(status->num_subscribers);
}

// populate proto buf stats from gpu watch API stats
static inline void
aga_gpu_watch_stats_to_proto (amdgpu::GPUWatchStats *proto_stats,
                              const aga_gpu_watch_stats_t *stats)
{
    uint32_t num_attrs;

    for (uint32_t i = 0; i < stats->num_gpu; i++) {
        auto gpu_watch_attr = proto_stats->add_gpuwatchattr();
        gpu_watch_attr->set_gpu(stats->gpu_watch_attr[i].gpu.id,
                                OBJ_MAX_KEY_LEN);
        num_attrs = stats->gpu_watch_attr[i].num_attrs;
        for (uint32_t j = 0; j < num_attrs; j++) {
            auto proto_attr = gpu_watch_attr->add_attr();
            auto attr = &stats->gpu_watch_attr[i].attr[j];

            proto_attr->set_id(aga_gpu_watch_attr_id_to_proto(attr->id));
            proto_attr->mutable_value()->set_units(
                            aga_gpu_watch_attr_id_to_units(attr->id));
            switch (attr->value.type) {
            case AGA_GPU_WATCH_ATTR_VALUE_TYPE_FLOAT:
                proto_attr->mutable_value()->set_floatval(
                                                 attr->value.float_val);
                break;
            case AGA_GPU_WATCH_ATTR_VALUE_TYPE_LONG:
                proto_attr->mutable_value()->set_longval(attr->value.long_val);
                break;
            case AGA_GPU_WATCH_ATTR_VALUE_TYPE_STRING:
                proto_attr->mutable_value()->set_stringval(attr->value.str_val,
                                                 AGA_MAX_WATCH_ATTR_STR + 1);
                break;
            case AGA_GPU_WATCH_ATTR_VALUE_TYPE_LONG_ARRAY:
                for (uint32_t k = 0;
                     k < attr->value.long_array.num_vals; k++) {
                    auto indexed =
                        proto_attr->mutable_value()->add_indexedval();
                    indexed->set_index(attr->value.long_array.vals[k].index);
                    // sub_type qualifier (0 = none): clocks carry clock_type
                    if (attr->value.long_array.vals[k].sub_type != 0) {
                        const char *st = aga_gpu_watch_clock_type_proto_name(
                            attr->value.long_array.vals[k].sub_type);
                        if (st[0] != '\0') {
                            indexed->set_subtype(st);
                        }
                    }
                    indexed->set_longval(attr->value.long_array.vals[k].val);
                }
                break;
            default:
                break;
            }
        }
    }
}

static inline void
aga_gpu_watch_info_to_proto (void *ctxt, const aga_gpu_watch_info_t *info)
{
    amdgpu::GPUWatch *proto_info = (amdgpu::GPUWatch *)ctxt;

    aga_gpu_watch_spec_to_proto(proto_info->mutable_spec(), &info->spec);
    aga_gpu_watch_status_to_proto(proto_info->mutable_status(), &info->status);
    aga_gpu_watch_stats_to_proto(proto_info->mutable_stats(), &info->stats);
}

static inline void
aga_gpu_watch_info_to_get_rsp_proto (void *ctxt,
                                     const aga_gpu_watch_info_t *info)
{
    amdgpu::GPUWatchGetResponse *get_rsp_proto =
        (amdgpu::GPUWatchGetResponse *)ctxt;

    aga_gpu_watch_info_to_proto(get_rsp_proto->add_response(), info);
}

#endif    // __AGA_SVC_GPU_WATCH_TO_PROTO_HPP__
