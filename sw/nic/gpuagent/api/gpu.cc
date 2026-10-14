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
    spec_.key = spec->key;
    spec_.compute_partition_type = spec->compute_partition_type;
    spec_.memory_partition_type = spec->memory_partition_type;
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

    if (spec_.compute_partition_type != spec->compute_partition_type) {
        upd_mask |= AGA_GPU_UPD_COMPUTE_PARTITION_TYPE;
    }
    if (spec_.memory_partition_type != spec->memory_partition_type) {
        upd_mask |= AGA_GPU_UPD_MEMORY_PARTITION_TYPE;
    }
    if (spec_.admin_state != spec->admin_state) {
        upd_mask |= AGA_GPU_UPD_ADMIN_STATE;
    }
    if (spec_.overdrive_level != spec->overdrive_level) {
        upd_mask |= AGA_GPU_UPD_OVERDRIVE_LEVEL;
    }
    if ((spec_.num_gpu_power_cap != spec->num_gpu_power_cap) ||
        (memcmp(spec_.gpu_power_cap, spec->gpu_power_cap,
                sizeof(aga_gpu_power_cap_t) * spec->num_gpu_power_cap))) {
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
    ret = smi_gpu_update(handle_, &key_, spec, upd_mask);
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }
    // update the stashed spec if the gpu update goes through
    memcpy(&spec_, spec, sizeof(aga_gpu_spec_t));
    return ret;
}

void
gpu_entry::fill_stats_(aga_gpu_stats_t *stats,
                       const aga_gpu_get_filter_t *filter) {
    // fill stats only for non-parent GPUs
    if (child_gpus_.size()) {
        return;
    }
    // fetch stats from smi apis
    smi_gpu_fill_stats(handle_, &key_, is_partitioned_,
        (partition_id_ == AGA_GPU_INVALID_PARTITION_ID) ? 0 : partition_id_,
        first_partition_handle_, stats, filter);
}

void
gpu_entry::fill_status_(aga_gpu_spec_t *spec, aga_gpu_status_t *status,
                        const aga_gpu_get_filter_t *filter) {
    if (child_gpus_.size()) {
        status->num_gpu_partition = child_gpus_.size();
        // for parent GPUs get uuids of all children
        for (uint32_t i = 0; i < child_gpus_.size(); i++) {
            status->gpu_partition[i] = child_gpus_[i];
        }
    } else {
        // copy information that we got at init time
        memcpy(status, &status_, sizeof(aga_gpu_status_t));
        if (parent_gpu_.valid()) {
            status->physical_gpu = parent_gpu_;
        }
        smi_gpu_fill_status(handle_, &key_, id_, spec, status, filter);
    }
}

void
gpu_entry::init_attrs(void) {
    // stash information already available
    status_.index = id_;
    status_.handle = handle_;
    status_.partition_id = partition_id_;
    // fill initial attributes that we can get from API calls
    smi_gpu_init_attrs(handle_, &key_, &spec_, &status_);
}

void
gpu_entry::fill_spec_(aga_gpu_spec_t *spec) {
    spec->key = key_;
    // get additional details only for non-parent GPUs
    if (!child_gpus_.size()) {
        // copy information that we got at init time
        memcpy(spec, &spec_, sizeof(aga_gpu_spec_t));
        smi_gpu_fill_spec(handle_, &key_, spec);
    }
}

sdk_ret_t
gpu_entry::read(aga_gpu_info_t *info, const aga_gpu_get_filter_t *filter) {
    fill_spec_(&info->spec);
    fill_status_(&info->spec, &info->status, filter);
    fill_stats_(&info->stats, filter);
    return SDK_RET_OK;
}

sdk_ret_t
gpu_entry::read_topology(aga_device_topology_info_t *info) {
    std::string device_name = std::string("GPU") + std::to_string(id_);

    strcpy(info->device.name, device_name.c_str());
    info->device.type = AGA_DEVICE_TYPE_GPU;
    smi_gpu_fill_device_topology(handle_, &key_, info);
    return SDK_RET_OK;
}

sdk_ret_t
gpu_entry::update_stats(aga_gpu_watch_fields_t *stats) {
    memcpy(&stats_, stats, sizeof(aga_gpu_watch_fields_t));
    return SDK_RET_OK;
}

// pack a flat uint64 array from the watch fields into an array-valued attribute
// value; element i is emitted at index i with no sub-type qualifier
static inline void
aga_fill_watch_long_array (aga_gpu_watch_attr_value_t *attr_val,
                           const uint64_t *src, uint32_t count) {
    attr_val->type = AGA_GPU_WATCH_ATTR_VALUE_TYPE_LONG_ARRAY;
    if (count > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
        count = AGA_GPU_WATCH_MAX_INDEXED_VALS;
    }
    attr_val->long_array.num_vals = count;
    for (uint32_t k = 0; k < count; k++) {
        attr_val->long_array.vals[k].index = k;
        attr_val->long_array.vals[k].sub_type = 0;
        attr_val->long_array.vals[k].val = src[k];
    }
}

// pack an array-valued attribute carrying an explicit per-element index and
// sub_type qualifier (used by clocks: index is the pull-path global clock slot,
// sub_type is the aga_gpu_clock_type_t; 0 means none and the exporter drops it)
static inline void
aga_fill_watch_indexed_array (aga_gpu_watch_attr_value_t *attr_val,
                              const uint32_t *index_arr,
                              const uint32_t *sub_type_arr,
                              const uint64_t *val_arr, uint32_t count) {
    attr_val->type = AGA_GPU_WATCH_ATTR_VALUE_TYPE_LONG_ARRAY;
    if (count > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
        count = AGA_GPU_WATCH_MAX_INDEXED_VALS;
    }
    attr_val->long_array.num_vals = count;
    for (uint32_t k = 0; k < count; k++) {
        attr_val->long_array.vals[k].index = index_arr[k];
        attr_val->long_array.vals[k].sub_type = sub_type_arr[k];
        attr_val->long_array.vals[k].val = val_arr[k];
    }
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
        case AGA_GPU_WATCH_ATTR_ID_UMC_ACTIVITY:
            attr_val->long_val = stats_.umc_activity;
            break;
        case AGA_GPU_WATCH_ATTR_ID_MM_ACTIVITY:
            attr_val->long_val = stats_.mm_activity;
            break;
        case AGA_GPU_WATCH_ATTR_ID_AVG_POWER:
            attr_val->long_val = stats_.avg_power;
            break;
        case AGA_GPU_WATCH_ATTR_ID_GFX_VOLTAGE:
            attr_val->long_val = stats_.gfx_voltage;
            break;
        case AGA_GPU_WATCH_ATTR_ID_SOC_VOLTAGE:
            attr_val->long_val = stats_.soc_voltage;
            break;
        case AGA_GPU_WATCH_ATTR_ID_MEM_VOLTAGE:
            attr_val->long_val = stats_.mem_voltage;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ENERGY_CONSUMED:
            attr_val->long_val = stats_.energy_consumed;
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
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_TOTAL:
            attr_val->long_val = stats_.total_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SDMA:
            attr_val->long_val = stats_.sdma_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_GFX:
            attr_val->long_val = stats_.gfx_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MMHUB:
            attr_val->long_val = stats_.mmhub_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_ATHUB:
            attr_val->long_val = stats_.athub_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_PCIE_BIF:
            attr_val->long_val = stats_.bif_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_HDP:
            attr_val->long_val = stats_.hdp_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_XGMI_WAFL:
            attr_val->long_val = stats_.xgmi_wafl_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_DF:
            attr_val->long_val = stats_.df_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SMN:
            attr_val->long_val = stats_.smn_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SEM:
            attr_val->long_val = stats_.sem_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MP0:
            attr_val->long_val = stats_.mp0_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MP1:
            attr_val->long_val = stats_.mp1_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_FUSE:
            attr_val->long_val = stats_.fuse_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_UMC:
            attr_val->long_val = stats_.umc_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MCA:
            attr_val->long_val = stats_.mca_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_VCN:
            attr_val->long_val = stats_.vcn_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_JPEG:
            attr_val->long_val = stats_.jpeg_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_IH:
            attr_val->long_val = stats_.ih_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MPIO:
            attr_val->long_val = stats_.mpio_deferred_errors;
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_REPLAY_COUNT:
            attr_val->long_val = stats_.pcie_replay_count;
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_RECOVERY_COUNT:
            attr_val->long_val = stats_.pcie_recovery_count;
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_REPLAY_ROLLOVER_COUNT:
            attr_val->long_val = stats_.pcie_replay_rollover_count;
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_NACK_SENT_COUNT:
            attr_val->long_val = stats_.pcie_nack_sent_count;
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_NACK_RECEIVED_COUNT:
            attr_val->long_val = stats_.pcie_nack_received_count;
            break;
        case AGA_GPU_WATCH_ATTR_ID_GPU_POWER_USAGE:
            attr_val->long_val = stats_.gpu_power_usage;
            break;
        case AGA_GPU_WATCH_ATTR_ID_VRAM_MAX_BANDWIDTH:
            attr_val->long_val = stats_.vram_max_bandwidth;
            break;
        case AGA_GPU_WATCH_ATTR_ID_EDGE_TEMP:
            attr_val->long_val = stats_.edge_temperature;
            break;
        case AGA_GPU_WATCH_ATTR_ID_TOTAL_VRAM:
            attr_val->long_val = stats_.total_vram;
            break;
        case AGA_GPU_WATCH_ATTR_ID_TOTAL_VISIBLE_VRAM:
            attr_val->long_val = stats_.total_visible_vram;
            break;
        case AGA_GPU_WATCH_ATTR_ID_TOTAL_GTT:
            attr_val->long_val = stats_.total_gtt;
            break;
        case AGA_GPU_WATCH_ATTR_ID_USED_VISIBLE_VRAM:
            attr_val->long_val = stats_.used_visible_vram;
            break;
        case AGA_GPU_WATCH_ATTR_ID_USED_GTT:
            attr_val->long_val = stats_.used_gtt;
            break;
        case AGA_GPU_WATCH_ATTR_ID_FREE_VRAM:
            attr_val->long_val = stats_.free_vram;
            break;
        case AGA_GPU_WATCH_ATTR_ID_FREE_VISIBLE_VRAM:
            attr_val->long_val = stats_.free_visible_vram;
            break;
        case AGA_GPU_WATCH_ATTR_ID_FREE_GTT:
            attr_val->long_val = stats_.free_gtt;
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_SPEED:
            attr_val->long_val = stats_.pcie_speed;
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_MAX_SPEED:
            attr_val->long_val = stats_.pcie_max_speed;
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
        case AGA_GPU_WATCH_ATTR_ID_HBM_TEMP:
            aga_fill_watch_long_array(attr_val, stats_.hbm_temperature,
                                      stats_.hbm_temperature_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_VCN_ACTIVITY:
            aga_fill_watch_long_array(attr_val, stats_.vcn_activity,
                                      stats_.vcn_activity_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_JPEG_ACTIVITY:
            aga_fill_watch_long_array(attr_val, stats_.jpeg_activity,
                                      stats_.jpeg_activity_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_LINK_RX:
            aga_fill_watch_long_array(attr_val, stats_.xgmi_link_rx,
                                      stats_.xgmi_link_rx_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_LINK_TX:
            aga_fill_watch_long_array(attr_val, stats_.xgmi_link_tx,
                                      stats_.xgmi_link_tx_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_GFX_BUSY_INST:
            aga_fill_watch_long_array(attr_val, stats_.gfx_busy_inst,
                                      stats_.gfx_busy_inst_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_VCN_BUSY_INST:
            aga_fill_watch_long_array(attr_val, stats_.vcn_busy_inst,
                                      stats_.vcn_busy_inst_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_JPEG_BUSY_INST:
            aga_fill_watch_long_array(attr_val, stats_.jpeg_busy_inst,
                                      stats_.jpeg_busy_inst_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_CURRENT:
            aga_fill_watch_indexed_array(attr_val, stats_.clock_index,
                                         stats_.clock_type, stats_.clock_current,
                                         stats_.clock_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_MIN:
            aga_fill_watch_indexed_array(attr_val, stats_.clock_index,
                                         stats_.clock_type, stats_.clock_min,
                                         stats_.clock_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_MAX:
            aga_fill_watch_indexed_array(attr_val, stats_.clock_index,
                                         stats_.clock_type, stats_.clock_max,
                                         stats_.clock_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_CURRENT_ACC_COUNTER:
            attr_val->long_val = stats_.viol_current_acc_counter;
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_ACC:
            attr_val->long_val = stats_.viol_proc_hot_residency_acc;
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_ACC:
            attr_val->long_val = stats_.viol_ppt_residency_acc;
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_ACC:
            attr_val->long_val = stats_.viol_socket_thermal_residency_acc;
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_ACC:
            attr_val->long_val = stats_.viol_vr_thermal_residency_acc;
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_ACC:
            attr_val->long_val = stats_.viol_hbm_thermal_residency_acc;
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_PCT:
            attr_val->long_val = stats_.viol_proc_hot_residency_pct;
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_PCT:
            attr_val->long_val = stats_.viol_ppt_residency_pct;
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_PCT:
            attr_val->long_val = stats_.viol_socket_thermal_residency_pct;
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_PCT:
            attr_val->long_val = stats_.viol_vr_thermal_residency_pct;
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_PCT:
            attr_val->long_val = stats_.viol_hbm_thermal_residency_pct;
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_ACC:
            aga_fill_watch_long_array(attr_val, stats_.viol_gfx_below_power_acc,
                                      stats_.viol_gfx_below_power_acc_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_ACC:
            aga_fill_watch_long_array(attr_val, stats_.viol_gfx_below_thermal_acc,
                                      stats_.viol_gfx_below_thermal_acc_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_ACC:
            aga_fill_watch_long_array(attr_val, stats_.viol_gfx_low_util_acc,
                                      stats_.viol_gfx_low_util_acc_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_ACC:
            aga_fill_watch_long_array(attr_val, stats_.viol_gfx_below_total_acc,
                                      stats_.viol_gfx_below_total_acc_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_PCT:
            aga_fill_watch_long_array(attr_val, stats_.viol_gfx_below_power_pct,
                                      stats_.viol_gfx_below_power_pct_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_PCT:
            aga_fill_watch_long_array(attr_val, stats_.viol_gfx_below_thermal_pct,
                                      stats_.viol_gfx_below_thermal_pct_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_PCT:
            aga_fill_watch_long_array(attr_val, stats_.viol_gfx_low_util_pct,
                                      stats_.viol_gfx_low_util_pct_count);
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_PCT:
            aga_fill_watch_long_array(attr_val, stats_.viol_gfx_below_total_pct,
                                      stats_.viol_gfx_below_total_pct_count);
            break;
        default:
            // skip rather than abort: an attribute subscribed but not yet
            // fillable (e.g. a deferred one) must not drop the whole frame
            AGA_TRACE_ERR("unknown watch attribute {}, GPU {}",
                          stats->attr[i].id, key_.str());
            attr_val->type = AGA_GPU_WATCH_ATTR_VALUE_TYPE_NONE;
            break;
        }
    }
    return SDK_RET_OK;
}

}    // namespace aga
