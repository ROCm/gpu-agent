
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
    default:
        return AGA_GPU_WATCH_ATTR_ID_INVALID;
    }
}

static inline sdk_ret_t
aga_gpu_watch_proto_to_api_spec (aga_gpu_watch_spec_t *api_spec,
                                 const GPUWatchSpec& proto_spec)
{
    sdk_ret_t ret;

    aga_obj_key_proto_to_api_spec(&api_spec->key, proto_spec.id());
    for (int i = 0; i < proto_spec.gpu_size(); i++) {
        aga_obj_key_proto_to_api_spec(&api_spec->gpu[i], proto_spec.gpu(i));
    }
    api_spec->num_gpu = proto_spec.gpu_size();
    for (int i = 0; i < proto_spec.attribute_size(); i++) {
        api_spec->attr_id[i] =
            aga_gpu_watch_attr_id_to_api_spec(proto_spec.attribute(i));
    }
    api_spec->num_attrs = proto_spec.attribute_size();
    return SDK_RET_OK;
}

#endif    // __AGA_SVC_GPU_WATCH_TO_SPEC_HPP__
