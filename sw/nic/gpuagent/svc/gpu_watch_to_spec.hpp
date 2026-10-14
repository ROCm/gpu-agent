
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
/// This module defines gpu watch protobuf to spec conversion APIs
///
//----------------------------------------------------------------------------

#ifndef __AGA_SVC_GPU_WATCH_TO_SPEC_HPP__
#define __AGA_SVC_GPU_WATCH_TO_SPEC_HPP__

#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/svc/gpu_watch.hpp"
#include "nic/gpuagent/api/include/aga_gpu_watch.hpp"

// convert gpu watch attrs id to spec
static inline aga_gpu_watch_attr_id_t
aga_gpu_watch_attr_id_to_api_spec (GPUWatchAttrId id)
{
    switch (id) {
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_GPU_CLOCK:
        return AGA_GPU_WATCH_ATTR_ID_GPU_CLOCK;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_MEM_CLOCK:
        return AGA_GPU_WATCH_ATTR_ID_MEM_CLOCK;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_MEMORY_TEMP:
        return AGA_GPU_WATCH_ATTR_ID_MEMORY_TEMP;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_GPU_TEMP:
        return AGA_GPU_WATCH_ATTR_ID_GPU_TEMP;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_POWER_USAGE:
        return AGA_GPU_WATCH_ATTR_ID_POWER_USAGE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_TX:
        return AGA_GPU_WATCH_ATTR_ID_PCIE_TX;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_RX:
        return AGA_GPU_WATCH_ATTR_ID_PCIE_RX;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_BANDWIDTH:
        return AGA_GPU_WATCH_ATTR_ID_PCIE_BANDWIDTH;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_GPU_UTIL:
        return AGA_GPU_WATCH_ATTR_ID_GPU_UTIL;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_UMC_ACTIVITY:
        return AGA_GPU_WATCH_ATTR_ID_UMC_ACTIVITY;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_MM_ACTIVITY:
        return AGA_GPU_WATCH_ATTR_ID_MM_ACTIVITY;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_AVG_POWER:
        return AGA_GPU_WATCH_ATTR_ID_AVG_POWER;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_GFX_VOLTAGE:
        return AGA_GPU_WATCH_ATTR_ID_GFX_VOLTAGE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_SOC_VOLTAGE:
        return AGA_GPU_WATCH_ATTR_ID_SOC_VOLTAGE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_MEM_VOLTAGE:
        return AGA_GPU_WATCH_ATTR_ID_MEM_VOLTAGE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ENERGY_CONSUMED:
        return AGA_GPU_WATCH_ATTR_ID_ENERGY_CONSUMED;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE:
        return AGA_GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_CORRECT_TOTAL:
        return AGA_GPU_WATCH_ATTR_ID_ECC_CORRECT_TOTAL;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_UNCORRECT_TOTAL:
        return AGA_GPU_WATCH_ATTR_ID_ECC_UNCORRECT_TOTAL;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_SDMA_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_SDMA_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_SDMA_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_SDMA_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_GFX_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_GFX_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_GFX_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_GFX_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MMHUB_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_MMHUB_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MMHUB_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_MMHUB_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_ATHUB_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_ATHUB_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_ATHUB_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_ATHUB_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_HDP_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_HDP_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_HDP_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_HDP_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DF_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DF_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DF_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DF_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_SMN_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_SMN_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_SMN_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_SMN_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_SEM_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_SEM_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_SEM_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_SEM_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MP0_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_MP0_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MP0_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_MP0_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MP1_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_MP1_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MP1_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_MP1_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_FUSE_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_FUSE_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_FUSE_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_FUSE_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_UMC_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_UMC_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_UMC_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_UMC_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MCA_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_MCA_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MCA_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_MCA_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_VCN_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_VCN_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_VCN_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_VCN_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_JPEG_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_JPEG_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_JPEG_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_JPEG_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_IH_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_IH_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_IH_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_IH_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MPIO_CE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_MPIO_CE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_MPIO_UE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_MPIO_UE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_TOTAL:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_TOTAL;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_SDMA:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SDMA;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_GFX:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_GFX;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_MMHUB:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MMHUB;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_ATHUB:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_ATHUB;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_PCIE_BIF:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_PCIE_BIF;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_HDP:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_HDP;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_XGMI_WAFL:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_XGMI_WAFL;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_DF:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_DF;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_SMN:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SMN;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_SEM:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SEM;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_MP0:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MP0;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_MP1:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MP1;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_FUSE:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_FUSE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_UMC:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_UMC;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_MCA:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MCA;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_VCN:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_VCN;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_JPEG:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_JPEG;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_IH:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_IH;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_ECC_DEF_MPIO:
        return AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MPIO;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_REPLAY_COUNT:
        return AGA_GPU_WATCH_ATTR_ID_PCIE_REPLAY_COUNT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_RECOVERY_COUNT:
        return AGA_GPU_WATCH_ATTR_ID_PCIE_RECOVERY_COUNT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_REPLAY_ROLLOVER_COUNT:
        return AGA_GPU_WATCH_ATTR_ID_PCIE_REPLAY_ROLLOVER_COUNT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_NACK_SENT_COUNT:
        return AGA_GPU_WATCH_ATTR_ID_PCIE_NACK_SENT_COUNT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_NACK_RECEIVED_COUNT:
        return AGA_GPU_WATCH_ATTR_ID_PCIE_NACK_RECEIVED_COUNT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_GPU_POWER_USAGE:
        return AGA_GPU_WATCH_ATTR_ID_GPU_POWER_USAGE;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VRAM_MAX_BANDWIDTH:
        return AGA_GPU_WATCH_ATTR_ID_VRAM_MAX_BANDWIDTH;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_EDGE_TEMP:
        return AGA_GPU_WATCH_ATTR_ID_EDGE_TEMP;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_TOTAL_VRAM:
        return AGA_GPU_WATCH_ATTR_ID_TOTAL_VRAM;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_TOTAL_VISIBLE_VRAM:
        return AGA_GPU_WATCH_ATTR_ID_TOTAL_VISIBLE_VRAM;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_TOTAL_GTT:
        return AGA_GPU_WATCH_ATTR_ID_TOTAL_GTT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_USED_VISIBLE_VRAM:
        return AGA_GPU_WATCH_ATTR_ID_USED_VISIBLE_VRAM;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_USED_GTT:
        return AGA_GPU_WATCH_ATTR_ID_USED_GTT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_FREE_VRAM:
        return AGA_GPU_WATCH_ATTR_ID_FREE_VRAM;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_FREE_VISIBLE_VRAM:
        return AGA_GPU_WATCH_ATTR_ID_FREE_VISIBLE_VRAM;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_FREE_GTT:
        return AGA_GPU_WATCH_ATTR_ID_FREE_GTT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_SPEED:
        return AGA_GPU_WATCH_ATTR_ID_PCIE_SPEED;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_PCIE_MAX_SPEED:
        return AGA_GPU_WATCH_ATTR_ID_PCIE_MAX_SPEED;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_0_NOP_TX:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_0_NOP_TX;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_0_REQ_TX:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_0_REQ_TX;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_0_RESP_TX:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_0_RESP_TX;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_0_BEATS_TX:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_0_BEATS_TX;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_1_NOP_TX:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_1_NOP_TX;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_1_REQ_TX:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_1_REQ_TX;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_1_RESP_TX:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_1_RESP_TX;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_1_BEATS_TX:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_1_BEATS_TX;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_0_THRPUT:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_0_THRPUT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_1_THRPUT:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_1_THRPUT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_2_THRPUT:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_2_THRPUT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_3_THRPUT:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_3_THRPUT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_4_THRPUT:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_4_THRPUT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_5_THRPUT:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_5_THRPUT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_HBM_TEMP:
        return AGA_GPU_WATCH_ATTR_ID_HBM_TEMP;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VCN_ACTIVITY:
        return AGA_GPU_WATCH_ATTR_ID_VCN_ACTIVITY;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_JPEG_ACTIVITY:
        return AGA_GPU_WATCH_ATTR_ID_JPEG_ACTIVITY;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_CLOCK_FREQ_CURRENT:
        return AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_CURRENT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_CLOCK_FREQ_MIN:
        return AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_MIN;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_CLOCK_FREQ_MAX:
        return AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_MAX;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_LINK_RX:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_LINK_RX;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_XGMI_LINK_TX:
        return AGA_GPU_WATCH_ATTR_ID_XGMI_LINK_TX;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_ACC:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_ACC;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_ACC:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_ACC;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_ACC:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_ACC;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_ACC:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_ACC;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_PCT:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_PCT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_PCT:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_PCT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_PCT:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_PCT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_PCT:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_PCT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_GFX_BUSY_INST:
        return AGA_GPU_WATCH_ATTR_ID_GFX_BUSY_INST;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VCN_BUSY_INST:
        return AGA_GPU_WATCH_ATTR_ID_VCN_BUSY_INST;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_JPEG_BUSY_INST:
        return AGA_GPU_WATCH_ATTR_ID_JPEG_BUSY_INST;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_CURRENT_ACC_COUNTER:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_CURRENT_ACC_COUNTER;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_ACC:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_ACC;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_ACC:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_ACC;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_ACC:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_ACC;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_ACC:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_ACC;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_ACC:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_ACC;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_PCT:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_PCT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_PCT:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_PCT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_PCT:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_PCT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_PCT:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_PCT;
    case amdgpu::GPUWatchAttrId::GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_PCT:
        return AGA_GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_PCT;
    default:
        return AGA_GPU_WATCH_ATTR_ID_INVALID;
    }
}

static inline sdk_ret_t
aga_gpu_watch_proto_to_api_spec (aga_gpu_watch_spec_t *api_spec,
                                 const GPUWatchSpec& proto_spec)
{
    int num_gpu;
    int num_attrs;
    uint32_t fast_secs, slow_secs;

    if (unlikely(aga_obj_key_proto_to_api_spec(&api_spec->key,
                     proto_spec.id()) != SDK_RET_OK)) {
        AGA_TRACE_ERR("Invalid GPU watch object id");
        return SDK_RET_INVALID_ARG;
    }
    num_gpu = proto_spec.gpu_size();
    if (num_gpu == 0) {
        AGA_TRACE_ERR("At least one GPU must be specified in GPU watch object");
        return SDK_RET_INVALID_ARG;
    }
    if (num_gpu > AGA_MAX_GPU) {
        AGA_TRACE_ERR("Number of GPUs {} in GPU watch object exceeds maximum "
                      "allowed {}", num_gpu, AGA_MAX_GPU);
        return SDK_RET_INVALID_ARG;
    }
    for (int i = 0; i < num_gpu; i++) {
        if (unlikely(aga_obj_key_proto_to_api_spec(&api_spec->gpu[i],
                         proto_spec.gpu(i)) != SDK_RET_OK)) {
            AGA_TRACE_ERR("Invalid GPU id at index {} in GPU watch object", i);
            return SDK_RET_INVALID_ARG;
        }
    }
    api_spec->num_gpu = num_gpu;
    num_attrs = proto_spec.attribute_size();
    if (num_attrs == 0) {
        AGA_TRACE_ERR("At least one attribute must be specified in GPU watch "
                      "spec");
        return SDK_RET_INVALID_ARG;
    }
    if (num_attrs > AGA_GPU_WATCH_ATTRS_MAX) {
        AGA_TRACE_ERR("Number of attributes {} in GPU watch object exceeds "
                      "maximum allowed {}", num_attrs, AGA_GPU_WATCH_ATTRS_MAX);
        return SDK_RET_INVALID_ARG;
    }
    for (int i = 0; i < num_attrs; i++) {
        api_spec->attr_id[i] =
            aga_gpu_watch_attr_id_to_api_spec(proto_spec.attribute(i));
    }
    api_spec->num_attrs = num_attrs;
    api_spec->fast_period_secs = proto_spec.fastperiodsecs();
    api_spec->slow_period_secs = proto_spec.slowperiodsecs();
    if (api_spec->fast_period_secs &&
        ((api_spec->fast_period_secs < AGA_GPU_WATCH_FAST_PERIOD_MIN) ||
         (api_spec->fast_period_secs > AGA_GPU_WATCH_FAST_PERIOD_MAX))) {
        AGA_TRACE_ERR("GPU watch {} fast period {} out of allowed range {}-{}",
                      api_spec->key.str(), api_spec->fast_period_secs,
                      AGA_GPU_WATCH_FAST_PERIOD_MIN,
                      AGA_GPU_WATCH_FAST_PERIOD_MAX);
        return SDK_RET_INVALID_ARG;
    }
    if (api_spec->slow_period_secs &&
        ((api_spec->slow_period_secs < AGA_GPU_WATCH_SLOW_PERIOD_MIN) ||
         (api_spec->slow_period_secs > AGA_GPU_WATCH_SLOW_PERIOD_MAX))) {
        AGA_TRACE_ERR("GPU watch {} slow period {} out of allowed range {}-{}",
                      api_spec->key.str(), api_spec->slow_period_secs,
                      AGA_GPU_WATCH_SLOW_PERIOD_MIN,
                      AGA_GPU_WATCH_SLOW_PERIOD_MAX);
        return SDK_RET_INVALID_ARG;
    }
    // 0 selects the default, so compare the periods that end up in effect
    fast_secs = api_spec->fast_period_secs ? api_spec->fast_period_secs :
                    AGA_GPU_WATCH_FAST_PERIOD_DFLT;
    slow_secs = api_spec->slow_period_secs ? api_spec->slow_period_secs :
                    AGA_GPU_WATCH_SLOW_PERIOD_DFLT;
    if (slow_secs < fast_secs) {
        AGA_TRACE_ERR("GPU watch {} slow period {} is less than fast period {}",
                      api_spec->key.str(), slow_secs, fast_secs);
        return SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

#endif    // __AGA_SVC_GPU_WATCH_TO_SPEC_HPP__
