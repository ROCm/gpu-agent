
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
        return std::string("%");
    case AGA_GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE:
        return std::string("MB");
    default:
        // TODO add units for other ids
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
