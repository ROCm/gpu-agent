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
/// gpu entry handling
///
//----------------------------------------------------------------------------

#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/mem.hpp"
#include "nic/gpuagent/api/gpu.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/internal/aga_api_params.hpp"
#include "nic/gpuagent/api/smi/smi_api.hpp"

namespace aga {

gpu_entry::gpu_entry() {
    // set partition id as invalid
    partition_id_ = AGA_GPU_INVALID_PARTITION_ID;
    // reset parent GPU uuid
    parent_gpu_.reset();
}

gpu_entry *
gpu_entry::factory(aga_gpu_spec_t *spec) {
    gpu_entry *gpu;

    gpu = gpu_db()->alloc();
    if (gpu) {
        new (gpu) gpu_entry();
    }
    return gpu;
}

gpu_entry::~gpu_entry() {
}

void
gpu_entry::destroy(gpu_entry *gpu) {
    gpu->~gpu_entry();
    gpu_db()->free(gpu);
}

sdk_ret_t
gpu_entry::free(gpu_entry *gpu) {
    gpu->~gpu_entry();
    gpu_db()->free(gpu);
    return SDK_RET_OK;
}

sdk_ret_t
gpu_entry::add_to_db(void) {
    return gpu_db()->insert(this);
}

sdk_ret_t
gpu_entry::del_from_db(void) {
    if (gpu_db()->remove(this)) {
        return SDK_RET_OK;
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

sdk_ret_t
gpu_entry::delay_delete(void) {
    return aga::delay_delete(AGA_OBJ_ID_GPU, this);
}

sdk_ret_t
gpu_entry::create_handler(api_params_base *api_params) {
    aga_gpu_spec_t *spec = AGA_GPU_SPEC(api_params);

    key_ = spec->key;
    parent_gpu_ = spec->parent_gpu;
    return SDK_RET_OK;
}

sdk_ret_t
gpu_entry::delete_handler(api_params_base *api_params) {
    if (num_gpu_watch_) {
        AGA_TRACE_ERR("Failed to delete GPU {}, there are currently {} "
                      "GPU watch objects watching this GPU", key_.str(),
                      num_gpu_watch_);
        return SDK_RET_IN_USE;
    }
    return SDK_RET_OK;
}

sdk_ret_t
gpu_entry::update_handler(api_params_base *api_params) {
    sdk_ret_t ret;
    uint64_t upd_mask = 0;
    aga_gpu_spec_t *spec = AGA_GPU_SPEC(api_params);

    if (spec_.admin_state != spec->admin_state) {
        upd_mask |= AGA_GPU_UPD_ADMIN_STATE;
    }
    if (spec_.overdrive_level != spec->overdrive_level) {
        upd_mask |= AGA_GPU_UPD_OVERDRIVE_LEVEL;
    }
    if (spec_.gpu_power_cap != spec->gpu_power_cap) {
        upd_mask |= AGA_GPU_UPD_POWER_CAP;
    }
    if (spec_.perf_level != spec->perf_level) {
        upd_mask |= AGA_GPU_UPD_PERF_LEVEL;
    }
    if (memcmp(spec_.clock_freq, spec->clock_freq,
               sizeof(aga_gpu_clock_freq_range_t) * AGA_GPU_CLOCK_TYPE_MAX)) {
        upd_mask |= AGA_GPU_UPD_CLOCK_FREQ_RANGE;
    }
    if (spec_.fan_speed != spec->fan_speed) {
        upd_mask |= AGA_GPU_UPD_FAN_SPEED;
    }
    if (memcmp(&spec_.ras_spec, &spec->ras_spec, sizeof(aga_gpu_ras_spec_t))) {
        upd_mask |= AGA_GPU_UPD_RAS_SPEC;
    }
    ret = smi_gpu_update(handle_, spec, upd_mask);
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }
    // update the stashed spec if the gpu update goes through
    memcpy(spec, &spec_, sizeof(aga_gpu_spec_t));
    return ret;
}

void
gpu_entry::fill_stats_(aga_gpu_stats_t *stats) {
    // fill stats only for non-parent GPUs
    if (child_gpus_.size()) {
        return;
    }
    // fill stats stased from watch infra
    stats->power_usage = stats_.power_usage;
    stats->total_correctable_errors = stats_.total_correctable_errors;
    stats->total_uncorrectable_errors = stats_.total_uncorrectable_errors;
    stats->sdma_correctable_errors = stats_.sdma_correctable_errors;
    stats->sdma_uncorrectable_errors = stats_.sdma_uncorrectable_errors;
    stats->gfx_correctable_errors = stats_.gfx_correctable_errors;
    stats->gfx_uncorrectable_errors = stats_.gfx_uncorrectable_errors;
    stats->mmhub_correctable_errors = stats_.mmhub_correctable_errors;
    stats->mmhub_uncorrectable_errors = stats_.mmhub_uncorrectable_errors;
    stats->athub_correctable_errors = stats_.athub_correctable_errors;
    stats->athub_uncorrectable_errors = stats_.athub_uncorrectable_errors;
    stats->bif_correctable_errors = stats_.bif_correctable_errors;
    stats->bif_uncorrectable_errors = stats_.bif_uncorrectable_errors;
    stats->hdp_correctable_errors = stats_.hdp_correctable_errors;
    stats->hdp_uncorrectable_errors = stats_.hdp_uncorrectable_errors;
    stats->xgmi_wafl_correctable_errors = stats_.xgmi_wafl_correctable_errors;
    stats->xgmi_wafl_uncorrectable_errors =
        stats_.xgmi_wafl_uncorrectable_errors;
    stats->df_correctable_errors = stats_.df_correctable_errors;
    stats->df_uncorrectable_errors = stats_.df_uncorrectable_errors;
    stats->smn_correctable_errors = stats_.smn_correctable_errors;
    stats->smn_uncorrectable_errors = stats_.smn_uncorrectable_errors;
    stats->sem_correctable_errors = stats_.sem_correctable_errors;
    stats->sem_uncorrectable_errors = stats_.sem_uncorrectable_errors;
    stats->mp0_correctable_errors = stats_.mp0_correctable_errors;
    stats->mp0_uncorrectable_errors = stats_.mp0_uncorrectable_errors;
    stats->mp1_correctable_errors = stats_.mp1_correctable_errors;
    stats->mp1_uncorrectable_errors = stats_.mp1_uncorrectable_errors;
    stats->fuse_correctable_errors = stats_.fuse_correctable_errors;
    stats->fuse_uncorrectable_errors = stats_.fuse_uncorrectable_errors;
    stats->umc_correctable_errors = stats_.umc_correctable_errors;
    stats->umc_uncorrectable_errors = stats_.umc_uncorrectable_errors;
    stats->mca_correctable_errors = stats_.mca_correctable_errors;
    stats->mca_uncorrectable_errors = stats_.mca_uncorrectable_errors;
    stats->vcn_correctable_errors = stats_.vcn_correctable_errors;
    stats->vcn_uncorrectable_errors = stats_.vcn_uncorrectable_errors;
    stats->jpeg_correctable_errors = stats_.jpeg_correctable_errors;
    stats->jpeg_uncorrectable_errors = stats_.jpeg_uncorrectable_errors;
    stats->ih_correctable_errors = stats_.ih_correctable_errors;
    stats->ih_uncorrectable_errors = stats_.ih_uncorrectable_errors;
    stats->mpio_correctable_errors = stats_.mpio_correctable_errors;
    stats->mpio_uncorrectable_errors = stats_.mpio_uncorrectable_errors;
    stats->xgmi_neighbor0_tx_nops = stats_.xgmi_neighbor0_tx_nops;
    stats->xgmi_neighbor0_tx_requests = stats_.xgmi_neighbor0_tx_requests;
    stats->xgmi_neighbor0_tx_responses = stats_.xgmi_neighbor0_tx_responses;
    stats->xgmi_neighbor0_tx_beats = stats_.xgmi_neighbor0_tx_beats;
    stats->xgmi_neighbor1_tx_nops = stats_.xgmi_neighbor1_tx_nops;
    stats->xgmi_neighbor1_tx_requests = stats_.xgmi_neighbor1_tx_requests;
    stats->xgmi_neighbor1_tx_responses = stats_.xgmi_neighbor1_tx_responses;
    stats->xgmi_neighbor1_tx_beats = stats_.xgmi_neighbor1_tx_beats;
    stats->xgmi_neighbor0_tx_throughput = stats_.xgmi_neighbor0_tx_throughput;
    stats->xgmi_neighbor1_tx_throughput = stats_.xgmi_neighbor1_tx_throughput;
    stats->xgmi_neighbor2_tx_throughput = stats_.xgmi_neighbor2_tx_throughput;
    stats->xgmi_neighbor3_tx_throughput = stats_.xgmi_neighbor3_tx_throughput;
    stats->xgmi_neighbor4_tx_throughput = stats_.xgmi_neighbor4_tx_throughput;
    stats->xgmi_neighbor5_tx_throughput = stats_.xgmi_neighbor5_tx_throughput;

    // fetch stats from smi apis
    smi_gpu_fill_stats(handle_, stats);
}

void
gpu_entry::fill_status_(aga_gpu_status_t *status) {
    if (child_gpus_.size()) {
        status->num_gpu_partition = child_gpus_.size();
        // for parent GPUs get uuids of all children
        for (uint32_t i = 0; i < child_gpus_.size(); i++) {
            status->gpu_partition[i] = child_gpus_[i];
        }
    } else {
        // get additional details only for non-parent GPUs
        status->handle = handle_;
        if (parent_gpu_.valid()) {
            status->physical_gpu = parent_gpu_;
        }
        smi_gpu_fill_status(handle_, id_, status);
    }
}

void
gpu_entry::fill_spec_(aga_gpu_spec_t *spec) {
    spec->key = key_;
    // get additional details only for non-parent GPUs
    if (!child_gpus_.size()) {
        smi_gpu_fill_spec(handle_, spec);
    }
}

sdk_ret_t
gpu_entry::read(aga_gpu_info_t *info) {
    fill_spec_(&info->spec);
    fill_status_(&info->status);
    fill_stats_(&info->stats);
    return SDK_RET_OK;
}

sdk_ret_t
gpu_entry::read_topology(aga_device_topology_info_t *info) {
    std::string device_name = std::string("GPU") + std::to_string(id_);

    strcpy(info->device.name, device_name.c_str());
    info->device.type = AGA_DEVICE_TYPE_GPU;
    smi_gpu_fill_device_topology(handle_, info);
    return SDK_RET_OK;
}

sdk_ret_t
gpu_entry::update_stats(aga_gpu_watch_fields_t *stats) {
    memcpy(&stats_, stats, sizeof(aga_gpu_watch_fields_t));
    return SDK_RET_OK;
}

sdk_ret_t
gpu_entry::fill_gpu_watch_stats(aga_gpu_watch_attrs_t *stats) {
    for (auto i = 0; i < stats->num_attrs; i++) {
        auto attr_val = &stats->attr[i].value;

        // set all attributes types to long upfront, if any attribute type
        // is different, can be overwritten in switch case
        attr_val->type = AGA_GPU_WATCH_ATTR_VALUE_TYPE_LONG;

        switch (stats->attr[i].id) {
        case AGA_GPU_WATCH_ATTR_ID_GPU_CLOCK:
            attr_val->long_val = stats_.gpu_clock;
            break;
        case AGA_GPU_WATCH_ATTR_ID_MEM_CLOCK:
            attr_val->long_val = stats_.memory_clock;
            break;
        case AGA_GPU_WATCH_ATTR_ID_GPU_TEMP:
            attr_val->long_val = stats_.gpu_temperature;
            break;
        case AGA_GPU_WATCH_ATTR_ID_MEMORY_TEMP:
            attr_val->long_val = stats_.memory_temperature;
            break;
        case AGA_GPU_WATCH_ATTR_ID_POWER_USAGE:
            attr_val->long_val = stats_.power_usage;
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_TX:
            attr_val->long_val = stats_.pcie_tx_usage;
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_RX:
            attr_val->long_val = stats_.pcie_rx_usage;
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_BANDWIDTH:
            attr_val->long_val = stats_.pcie_bandwidth;
            break;
        case AGA_GPU_WATCH_ATTR_ID_GPU_UTIL:
            attr_val->long_val = stats_.gpu_util;
            break;
        case AGA_GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE:
            attr_val->long_val = stats_.gpu_memory_usage;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_CORRECT_TOTAL:
            attr_val->long_val = stats_.total_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_UNCORRECT_TOTAL:
            attr_val->long_val = stats_.total_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_SDMA_CE:
            attr_val->long_val = stats_.sdma_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_SDMA_UE:
            attr_val->long_val = stats_.sdma_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_GFX_CE:
            attr_val->long_val = stats_.gfx_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_GFX_UE:
            attr_val->long_val = stats_.gfx_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_MMHUB_CE:
            attr_val->long_val = stats_.mmhub_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_MMHUB_UE:
            attr_val->long_val = stats_.mmhub_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_ATHUB_CE:
            attr_val->long_val = stats_.athub_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_ATHUB_UE:
            attr_val->long_val = stats_.athub_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_CE:
            attr_val->long_val = stats_.bif_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_UE:
            attr_val->long_val = stats_.bif_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_HDP_CE:
            attr_val->long_val = stats_.hdp_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_HDP_UE:
            attr_val->long_val = stats_.hdp_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_CE:
            attr_val->long_val = stats_.xgmi_wafl_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_UE:
            attr_val->long_val = stats_.xgmi_wafl_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DF_CE:
            attr_val->long_val = stats_.df_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DF_UE:
            attr_val->long_val = stats_.df_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_SMN_CE:
            attr_val->long_val = stats_.smn_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_SMN_UE:
            attr_val->long_val = stats_.smn_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_SEM_CE:
            attr_val->long_val = stats_.sem_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_SEM_UE:
            attr_val->long_val = stats_.sem_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_MP0_CE:
            attr_val->long_val = stats_.mp0_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_MP0_UE:
            attr_val->long_val = stats_.mp0_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_MP1_CE:
            attr_val->long_val = stats_.mp1_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_MP1_UE:
            attr_val->long_val = stats_.mp1_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_FUSE_CE:
            attr_val->long_val = stats_.fuse_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_FUSE_UE:
            attr_val->long_val = stats_.fuse_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_UMC_CE:
            attr_val->long_val = stats_.umc_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_UMC_UE:
            attr_val->long_val = stats_.umc_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_MCA_CE:
            attr_val->long_val = stats_.mca_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_MCA_UE:
            attr_val->long_val = stats_.mca_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_VCN_CE:
            attr_val->long_val = stats_.vcn_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_VCN_UE:
            attr_val->long_val = stats_.vcn_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_JPEG_CE:
            attr_val->long_val = stats_.jpeg_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_JPEG_UE:
            attr_val->long_val = stats_.jpeg_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_IH_CE:
            attr_val->long_val = stats_.ih_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_IH_UE:
            attr_val->long_val = stats_.ih_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_MPIO_CE:
            attr_val->long_val = stats_.mpio_correctable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_MPIO_UE:
            attr_val->long_val = stats_.mpio_uncorrectable_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_NOP_TX:
            attr_val->long_val = stats_.xgmi_neighbor0_tx_nops;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_REQ_TX:
            attr_val->long_val = stats_.xgmi_neighbor0_tx_requests;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_RESP_TX:
            attr_val->long_val = stats_.xgmi_neighbor0_tx_responses;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_BEATS_TX:
            attr_val->long_val = stats_.xgmi_neighbor0_tx_beats;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_NOP_TX:
            attr_val->long_val = stats_.xgmi_neighbor1_tx_nops;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_REQ_TX:
            attr_val->long_val = stats_.xgmi_neighbor1_tx_requests;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_RESP_TX:
            attr_val->long_val = stats_.xgmi_neighbor1_tx_responses;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_BEATS_TX:
            attr_val->long_val = stats_.xgmi_neighbor1_tx_beats;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_THRPUT:
            attr_val->long_val = stats_.xgmi_neighbor0_tx_throughput;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_THRPUT:
            attr_val->long_val = stats_.xgmi_neighbor1_tx_throughput;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_2_THRPUT:
            attr_val->long_val = stats_.xgmi_neighbor2_tx_throughput;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_3_THRPUT:
            attr_val->long_val = stats_.xgmi_neighbor3_tx_throughput;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_4_THRPUT:
            attr_val->long_val = stats_.xgmi_neighbor4_tx_throughput;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_5_THRPUT:
            attr_val->long_val = stats_.xgmi_neighbor5_tx_throughput;
            break;
        default:
            AGA_TRACE_ERR("unknown watch attribute {}, GPU {}",
                          stats->attr[i].id, key_.str());
            return SDK_RET_ERR;
        }
    }
    return SDK_RET_OK;
}

}    // namespace aga
