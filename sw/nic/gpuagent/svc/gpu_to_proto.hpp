
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
/// This module defines protobuf conversion APIs for gpu object
///
//----------------------------------------------------------------------------

#ifndef __AGA_SVC_GPU_TO_PROTO_HPP__
#define __AGA_SVC_GPU_TO_PROTO_HPP__

#include <string>
#include "nic/gpuagent/svc/gpu.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"

static inline amdgpu::GPUAdminState
aga_gpu_admin_state_to_proto (aga_gpu_admin_state_t admin_state)
{
    switch(admin_state) {
    case AGA_GPU_ADMIN_STATE_UP:
        return amdgpu::GPU_ADMIN_STATE_UP;
    case AGA_GPU_ADMIN_STATE_DOWN:
        return amdgpu::GPU_ADMIN_STATE_DOWN;
    case AGA_GPU_ADMIN_STATE_NONE:
    default:
        break;
    }
    return amdgpu::GPU_ADMIN_STATE_NONE;
}

static inline amdgpu::GPUPerformanceLevel
aga_gpu_perf_level_to_proto (aga_gpu_perf_level_t perf_level)
{
    switch (perf_level) {
    case AGA_GPU_PERF_LEVEL_AUTO:
        return amdgpu::GPU_PERF_LEVEL_AUTO;
    case AGA_GPU_PERF_LEVEL_LOW:
        return amdgpu::GPU_PERF_LEVEL_LOW;
    case AGA_GPU_PERF_LEVEL_HIGH:
        return amdgpu::GPU_PERF_LEVEL_HIGH;
    case AGA_GPU_PERF_LEVEL_DETERMINISTIC:
        return amdgpu::GPU_PERF_LEVEL_DETERMINISTIC;
    case AGA_GPU_PERF_LEVEL_STABLE_WITH_MCLK:
        return amdgpu::GPU_PERF_LEVEL_STABLE_MIN_MCLK;
    case AGA_GPU_PERF_LEVEL_STABLE_WITH_SCLK:
        return amdgpu::GPU_PERF_LEVEL_STABLE_MIN_SCLK;
    case AGA_GPU_PERF_LEVEL_MANUAL:
        return amdgpu::GPU_PERF_LEVEL_MANUAL;
    case AGA_GPU_PERF_LEVEL_NONE:
    default:
        break;
    }
    return amdgpu::GPU_PERF_LEVEL_NONE;
}

static inline amdgpu::GPUClockType
aga_gpu_clock_type_to_proto (aga_gpu_clock_type_t type)
{
    switch (type) {
    case AGA_GPU_CLOCK_TYPE_FABRIC:
        return amdgpu::GPU_CLOCK_TYPE_FABRIC;
    case AGA_GPU_CLOCK_TYPE_MEMORY:
        return amdgpu::GPU_CLOCK_TYPE_MEMORY;
    case AGA_GPU_CLOCK_TYPE_SYSTEM:
        return amdgpu::GPU_CLOCK_TYPE_SYSTEM;
    case AGA_GPU_CLOCK_TYPE_SOC:
        return amdgpu::GPU_CLOCK_TYPE_SOC;
    case AGA_GPU_CLOCK_TYPE_DCE:
        return amdgpu::GPU_CLOCK_TYPE_DCE;
    case AGA_GPU_CLOCK_TYPE_PCIE:
        return amdgpu::GPU_CLOCK_TYPE_PCIE;
    case AGA_GPU_CLOCK_TYPE_VIDEO:
        return amdgpu::GPU_CLOCK_TYPE_VIDEO;
    case AGA_GPU_CLOCK_TYPE_DATA:
        return amdgpu::GPU_CLOCK_TYPE_DATA;
    case AGA_GPU_CLOCK_TYPE_NONE:
    default:
        break;
    }
    return amdgpu::GPU_CLOCK_TYPE_NONE;
}

static inline amdgpu::GPUThrottlingStatus
aga_gpu_throttling_status_to_proto (aga_gpu_throttling_status_t status)
{
    switch (status) {
    case AGA_GPU_THROTTLING_STATUS_OFF:
        return amdgpu::GPU_THROTTLING_STATUS_OFF;
    case AGA_GPU_THROTTLING_STATUS_ON:
        return amdgpu::GPU_THROTTLING_STATUS_ON;
    case AGA_GPU_THROTTLING_STATUS_NONE:
    default:
        break;
    }
    return amdgpu::GPU_THROTTLING_STATUS_NONE;
}

static inline void
aga_gpu_clock_spec_to_proto (GPUClockFrequencyRange *proto_spec,
                             const aga_gpu_clock_freq_range_t *spec)
{
    proto_spec->set_clocktype(aga_gpu_clock_type_to_proto(spec->clock_type));
    proto_spec->set_lowfrequency(spec->lo);
    proto_spec->set_highfrequency(spec->hi);
}

static inline amdgpu::GPUComputePartitionType
aga_gpu_compute_partition_type_to_proto (aga_gpu_compute_partition_type_t type)
{
    switch (type) {
    case AGA_GPU_COMPUTE_PARTITION_TYPE_SPX:
        return amdgpu::GPU_COMPUTE_PARTITION_TYPE_SPX;
    case AGA_GPU_COMPUTE_PARTITION_TYPE_DPX:
        return amdgpu::GPU_COMPUTE_PARTITION_TYPE_DPX;
    case AGA_GPU_COMPUTE_PARTITION_TYPE_TPX:
        return amdgpu::GPU_COMPUTE_PARTITION_TYPE_TPX;
    case AGA_GPU_COMPUTE_PARTITION_TYPE_QPX:
        return amdgpu::GPU_COMPUTE_PARTITION_TYPE_QPX;
    case AGA_GPU_COMPUTE_PARTITION_TYPE_CPX:
        return amdgpu::GPU_COMPUTE_PARTITION_TYPE_CPX;
    default:
        return amdgpu::GPU_COMPUTE_PARTITION_TYPE_NONE;
    }
}

// populate proto buf spec from gpu API spec
static inline void
aga_gpu_api_spec_to_proto (GPUSpec *proto_spec,
                           const aga_gpu_spec_t *spec)
{
    proto_spec->set_id(spec->key.id, OBJ_MAX_KEY_LEN);
    proto_spec->set_adminstate(aga_gpu_admin_state_to_proto(spec->admin_state));
    proto_spec->set_overdrivelevel(spec->overdrive_level);
    proto_spec->set_gpupowercap(spec->gpu_power_cap);
    proto_spec->set_performancelevel(aga_gpu_perf_level_to_proto(
                                         spec->perf_level));
    for (uint32_t i = 0; i < spec->num_clock_freqs; i++) {
        aga_gpu_clock_spec_to_proto(proto_spec->add_clockfrequency(),
                                    &spec->clock_freq[i]);
    }
    proto_spec->set_fanspeed(spec->fan_speed);
    proto_spec->set_computepartitiontype(
                    aga_gpu_compute_partition_type_to_proto(
                        spec->compute_partition_type));
    // TODO: fill gpu RAS spec
}

static inline void
aga_gpu_fw_version_to_proto (GPUStatus *proto_status,
                             const aga_gpu_status_t *status)
{
    for (uint32_t i = 0; i < status->num_fw_versions; i++) {
        auto fw_ver = proto_status->add_firmwareversion();
        fw_ver->set_firmware(status->fw_version[i].firmware);
        fw_ver->set_version(status->fw_version[i].version);
    }
}

static inline void
aga_gpu_clock_status_to_proto (GPUStatus *proto_status,
                               const aga_gpu_status_t *status)
{
    for (uint32_t i = 0; i < status->num_clock_status; i++) {
        auto clk_status = proto_status->add_clockstatus();
        clk_status->set_type(aga_gpu_clock_type_to_proto(
                                 status->clock_status[i].clock_type));
        clk_status->set_frequency(status->clock_status[i].frequency);
        clk_status->set_lowfrequency(status->clock_status[i].low_frequency);
        clk_status->set_highfrequency(status->clock_status[i].high_frequency);
        clk_status->set_locked(status->clock_status[i].locked);
        clk_status->set_deepsleep(status->clock_status[i].deep_sleep);
    }
}
static inline void
aga_gpu_voltage_curve_point_to_proto (GPUStatus *proto_status,
                                      const aga_gpu_status_t *status)
{
    // voltage-curve-point proto message is currently not defined in status
}

static inline amdgpu::GPUOperStatus
aga_gpu_oper_status_to_proto (aga_gpu_oper_state_t oper_status)
{
    switch (oper_status) {
    case AGA_GPU_OPER_STATE_UP:
        return amdgpu::GPU_OPER_STATUS_UP;
    case AGA_GPU_OPER_STATE_DOWN:
        return amdgpu::GPU_OPER_STATUS_DOWN;
    case AGA_GPU_OPER_STATE_NONE:
    default:
        break;
    }
    return amdgpu::GPU_OPER_STATUS_NONE;
}

static inline amdgpu::GPUXGMIErrorStatus
aga_gpu_xgmi_error_status_to_proto (aga_gpu_xgmi_error_status_t xgmi_status)
{
    switch (xgmi_status) {
    case AGA_GPU_XGMI_STATUS_NO_ERROR:
        return amdgpu::GPU_XGMI_STATUS_NO_ERROR;
    case AGA_GPU_XGMI_STATUS_ONE_ERROR:
        return amdgpu::GPU_XGMI_STATUS_ONE_ERROR;
    case AGA_GPU_XGMI_STATUS_MULTIPLE_ERROR:
        return amdgpu::GPU_XGMI_STATUS_MULTIPLE_ERROR;
    default:
        break;
    }
    return amdgpu::GPU_XGMI_STATUS_NONE;
}

static inline void
aga_gpu_xgmi_status_to_proto (amdgpu::GPUXGMIStatus *proto_status,
                              const aga_gpu_xgmi_status_t *status)
{
    proto_status->set_errorstatus(
        aga_gpu_xgmi_error_status_to_proto(status->error_status));
    proto_status->set_width(status->width);
    proto_status->set_speed(status->speed);
}

static inline amdgpu::PCIeSlotType
aga_gpu_pcie_slot_type_to_proto (aga_pcie_slot_type_t slot_type)
{
    switch (slot_type) {
    case AGA_PCIE_SLOT_TYPE_PCIE:
        return amdgpu::PCIE_SLOT_TYPE_PCIE;
    case AGA_PCIE_SLOT_TYPE_OAM:
        return amdgpu::PCIE_SLOT_TYPE_OAM;
    case AGA_PCIE_SLOT_TYPE_CEM:
        return amdgpu::PCIE_SLOT_TYPE_CEM;
    case AGA_PCIE_SLOT_TYPE_UNKNOWN:
        return amdgpu::PCIE_SLOT_TYPE_UNKNOWN;
    default:
        return amdgpu::PCIE_SLOT_TYPE_NONE;
    }
}

// populte PCIe status proto
static inline void
aga_gpu_pcie_status_to_proto (GPUPCIeStatus *proto_status,
                              const aga_gpu_pcie_status_t *status)
{
    proto_status->set_slottype(
                      aga_gpu_pcie_slot_type_to_proto(status->slot_type));
    proto_status->set_pciebusid(status->pcie_bus_id);
    proto_status->set_maxwidth(status->max_width);
    proto_status->set_maxspeed(status->max_speed);
    proto_status->set_version(status->version);
    proto_status->set_width(status->width);
    proto_status->set_speed(status->speed);
    proto_status->set_bandwidth(status->bandwidth);
}

static inline amdgpu::VRAMType
aga_gpu_vram_type_to_proto (aga_vram_type_t type)
{
    switch (type) {
    case AGA_VRAM_TYPE_HBM:
        return amdgpu::VRAM_TYPE_HBM;
    case AGA_VRAM_TYPE_HBM2:
        return amdgpu::VRAM_TYPE_HBM2;
    case AGA_VRAM_TYPE_HBM2E:
        return amdgpu::VRAM_TYPE_HBM2E;
    case AGA_VRAM_TYPE_HBM3:
        return amdgpu::VRAM_TYPE_HBM3;
    case AGA_VRAM_TYPE_DDR2:
        return amdgpu::VRAM_TYPE_DDR2;
    case AGA_VRAM_TYPE_DDR3:
        return amdgpu::VRAM_TYPE_DDR3;
    case AGA_VRAM_TYPE_DDR4:
        return amdgpu::VRAM_TYPE_DDR4;
    case AGA_VRAM_TYPE_GDDR1:
        return amdgpu::VRAM_TYPE_GDDR1;
    case AGA_VRAM_TYPE_GDDR2:
        return amdgpu::VRAM_TYPE_GDDR2;
    case AGA_VRAM_TYPE_GDDR3:
        return amdgpu::VRAM_TYPE_GDDR3;
    case AGA_VRAM_TYPE_GDDR4:
        return amdgpu::VRAM_TYPE_GDDR4;
    case AGA_VRAM_TYPE_GDDR5:
        return amdgpu::VRAM_TYPE_GDDR5;
    case AGA_VRAM_TYPE_GDDR6:
        return amdgpu::VRAM_TYPE_GDDR6;
    case AGA_VRAM_TYPE_GDDR7:
        return amdgpu::VRAM_TYPE_GDDR7;
    case AGA_VRAM_TYPE_UNKNOWN:
        return amdgpu::VRAM_TYPE_UNKNOWN;
    default:
        return amdgpu::VRAM_TYPE_NONE;
    }
}

static inline amdgpu::VRAMVendor
aga_gpu_vram_vendor_to_proto (aga_vram_vendor_t vendor)
{
    switch (vendor) {
    case AGA_VRAM_VENDOR_SAMSUNG:
        return amdgpu::VRAM_VENDOR_SAMSUNG;
    case AGA_VRAM_VENDOR_INFINEON:
        return amdgpu::VRAM_VENDOR_INFINEON;
    case AGA_VRAM_VENDOR_ELPIDA:
        return amdgpu::VRAM_VENDOR_ELPIDA;
    case AGA_VRAM_VENDOR_ETRON:
        return amdgpu::VRAM_VENDOR_ETRON;
    case AGA_VRAM_VENDOR_NANYA:
        return amdgpu::VRAM_VENDOR_NANYA;
    case AGA_VRAM_VENDOR_HYNIX:
        return amdgpu::VRAM_VENDOR_HYNIX;
    case AGA_VRAM_VENDOR_MOSEL:
        return amdgpu::VRAM_VENDOR_MOSEL;
    case AGA_VRAM_VENDOR_WINBOND:
        return amdgpu::VRAM_VENDOR_WINBOND;
    case AGA_VRAM_VENDOR_ESMT:
        return amdgpu::VRAM_VENDOR_ESMT;
    case AGA_VRAM_VENDOR_MICRON:
        return amdgpu::VRAM_VENDOR_MICRON;
    case AGA_VRAM_VENDOR_UNKNOWN:
        return amdgpu::VRAM_VENDOR_UNKNOWN;
    default:
        return amdgpu::VRAM_VENDOR_NONE;
    }
}

// populate VRAM status proto
static inline void
aga_gpu_vram_status_to_proto (GPUVRAMStatus *proto_status,
                              const aga_gpu_vram_status_t *status)
{
    proto_status->set_type(aga_gpu_vram_type_to_proto(status->type));
    proto_status->set_vendor(aga_gpu_vram_vendor_to_proto(status->vendor));
    proto_status->set_size(status->size);
}

static inline amdgpu::GPUPageStatus
aga_gpu_page_status_to_proto (aga_gpu_page_status_t page_status)
{
    switch (page_status) {
    case AGA_GPU_PAGE_STATUS_RESERVED:
        return amdgpu::GPU_PAGE_STATUS_RESERVED;
    case AGA_GPU_PAGE_STATUS_PENDING:
        return amdgpu::GPU_PAGE_STATUS_PENDING;
    case AGA_GPU_PAGE_STATUS_UNRESERVABLE:
        return amdgpu::GPU_PAGE_STATUS_UNRESERVABLE;
    case AGA_GPU_PAGE_STATUS_NONE:
    default:
        break;
    }
    return amdgpu::GPU_PAGE_STATUS_NONE;
}

// populate proto buf status from gpu status
static inline void
aga_gpu_api_status_to_proto (GPUStatus *proto_status,
                             const aga_gpu_status_t *status)
{
    proto_status->set_index(status->index);
    proto_status->set_gpuhandle((uint64_t)status->handle);
    proto_status->set_serialnum(status->serial_num);
    proto_status->set_cardseries(status->card_series);
    proto_status->set_cardmodel(status->card_model);
    proto_status->set_cardvendor(status->card_vendor);
    proto_status->set_cardsku(status->card_sku);
    proto_status->set_driverversion(status->driver_version);
    proto_status->set_vbiosversion(status->vbios_version);
    proto_status->set_vbiospartnumber(status->vbios_part_number);
    aga_gpu_fw_version_to_proto(proto_status, status);
    proto_status->set_memoryvendor(status->memory_vendor);
    proto_status->set_operstatus(
                      aga_gpu_oper_status_to_proto(status->oper_status));
    aga_gpu_clock_status_to_proto(proto_status, status);
    for (uint32_t i = 0;  i < status->num_kfd_process_id; i++) {
        if (status->kfd_process_id[i]) {
            // copy only non-zero process ids only
            proto_status->add_kfdprocessid(status->kfd_process_id[i]);
        }
    }
    // TODO: fill RAS status
    aga_gpu_xgmi_status_to_proto(proto_status->mutable_xgmistatus(),
                                 &status->xgmi_status);
    aga_gpu_voltage_curve_point_to_proto(proto_status, status);
    aga_gpu_vram_status_to_proto(proto_status->mutable_vramstatus(),
                                 &status->vram_status);
    aga_gpu_pcie_status_to_proto(proto_status->mutable_pciestatus(),
                                 &status->pcie_status);
    proto_status->set_throttlingstatus(aga_gpu_throttling_status_to_proto(
                                          status->throttling_status));
    proto_status->set_fwtimestamp(status->fw_timestamp);
    proto_status->set_partitionid(status->partition_id);
    for (uint32_t i = 0; i < status->num_gpu_partition; i++) {
        if (status->gpu_partition[i].valid()) {
            proto_status->add_gpupartition(status->gpu_partition[i].id,
                                           OBJ_MAX_KEY_LEN);
        }
    }
    if (status->physical_gpu.valid()) {
        proto_status->set_physicalgpu(status->physical_gpu.id, OBJ_MAX_KEY_LEN);
    }
}

// populate gpu bad page records proto buf
static inline void
aga_gpu_bad_page_api_info_to_proto (uint32_t num_bad_pages,
                                    aga_gpu_bad_page_record_t *records,
                                    void *ctxt)
{
    streaming_get_ctxt_t *get_ctxt;
    GPUBadPageGetResponse *proto_rsp;
    grpc::ServerWriter<GPUBadPageGetResponse> *writer;

    get_ctxt = (streaming_get_ctxt_t *)ctxt;
    proto_rsp = (GPUBadPageGetResponse *)get_ctxt->msg_ctxt;
    writer = (grpc::ServerWriter<GPUBadPageGetResponse> *)get_ctxt->writer_ctxt;

    for (uint32_t i = 0; i < num_bad_pages; i++) {
        get_ctxt->count++;
        auto proto_record = proto_rsp->add_record();
        proto_record->set_gpu(records[i].key.id, OBJ_MAX_KEY_LEN);
        proto_record->set_pageaddress(records[i].page_address);
        proto_record->set_pagesize(records[i].page_size);
        proto_record->set_pagestatus(
                          aga_gpu_page_status_to_proto(records[i].page_status));
        if (proto_rsp->record_size() == AGA_MAX_STREAMING_RSP_SIZE) {
            proto_rsp->set_apistatus(sdk_ret_to_api_status(SDK_RET_OK));
            proto_rsp->set_errorcode(sdk_ret_to_error_code(SDK_RET_OK));
            if (!writer->Write(*proto_rsp)) {
                AGA_TRACE_ERR("Failed to write gpu bad page info to gRPC "
                              "stream");
            }
            proto_rsp->Clear();
        }
    }
}

// populate gpu compute partition get response proto buf
static inline void
aga_gpu_compute_partition_info_to_proto (
    aga_gpu_compute_partition_info_t *info, void *ctxt)
{
    GPUComputePartitionGetResponse *proto_rsp =
        (GPUComputePartitionGetResponse *)ctxt;

    auto resp = proto_rsp->add_response();
    resp->set_id(info->physical_gpu.id, OBJ_MAX_KEY_LEN);
    resp->set_computepartitiontype(aga_gpu_compute_partition_type_to_proto(
                                       info->compute_partition_type));
    for (uint32_t i = 0; i < info->num_gpu_partition; i++) {
        if (info->gpu_partition[i].valid()) {
            resp->add_gpupartition(info->gpu_partition[i].id,
                                   OBJ_MAX_KEY_LEN);
        }
    }
}

// populate temperature proto buf stats from gpu stats
static inline void
aga_gpu_temp_stats_to_proto (amdgpu::GPUTemperatureStats *proto_stats,
                             const aga_gpu_temperature_stats_t *stats)
{
    proto_stats->add_hbmtemperature(stats->hbm_temperature[0]);
    proto_stats->add_hbmtemperature(stats->hbm_temperature[1]);
    proto_stats->add_hbmtemperature(stats->hbm_temperature[2]);
    proto_stats->add_hbmtemperature(stats->hbm_temperature[3]);
    proto_stats->set_edgetemperature(stats->edge_temperature);
    proto_stats->set_junctiontemperature(stats->junction_temperature);
    proto_stats->set_memorytemperature(stats->memory_temperature);
}

// populate proto gpu usage stats from gpu stats
static inline void
aga_gpu_usage_stats_to_proto (GPUUsage *proto_stats,
                              const aga_gpu_usage_t *stats)
{
    proto_stats->set_gfxactivity(stats->gfx_activity);
    proto_stats->set_umcactivity(stats->umc_activity);
    proto_stats->set_mmactivity(stats->mm_activity);
    for (uint16_t i = 0; i < stats->num_vcn; i++) {
        proto_stats->add_vcnactivity(stats->vcn_activity[i]);
    }
    for (uint16_t i = 0; i < stats->num_jpeg; i++) {
        proto_stats->add_jpegactivity(stats->jpeg_activity[i]);
    }
}

// populate proto memory usage stats from gpu stats
static inline void
aga_gpu_memory_usage_stats_to_proto (GPUMemoryUsage *proto_stats,
                                     const aga_gpu_memory_usage_t *stats)
{
    proto_stats->set_memoryusage(stats->memory_usage);
    proto_stats->set_activity(stats->activity);
}

// populte PCIe stats proto
static inline void
aga_gpu_pcie_stats_to_proto (GPUPCIeStats *proto_stats,
                             const aga_gpu_pcie_stats_t *stats)
{
    proto_stats->set_replaycount(stats->replay_count);
    proto_stats->set_recoverycount(stats->recovery_count);
    proto_stats->set_replayrollovercount(stats->replay_rollover_count);
    proto_stats->set_nacksentcount(stats->nack_sent_count);
    proto_stats->set_nackreceivedcount(stats->nack_received_count);
}

// populte VRAM usage stats proto
static inline void
aga_gpu_vram_usage_stats_to_proto (GPUVRAMUsage *proto_stats,
                                   const aga_gpu_vram_usage_t *stats)
{
    proto_stats->set_totalvram(stats->total_vram);
    proto_stats->set_usedvram(stats->used_vram);
    proto_stats->set_freevram(stats->free_vram);
    proto_stats->set_totalvisiblevram(stats->total_visible_vram);
    proto_stats->set_usedvisiblevram(stats->used_visible_vram);
    proto_stats->set_freevisiblevram(stats->free_visible_vram);
    proto_stats->set_totalgtt(stats->total_gtt);
    proto_stats->set_usedgtt(stats->used_gtt);
    proto_stats->set_freegtt(stats->free_gtt);
}

// populte GPU voltage proto
static inline void
aga_gpu_voltage_to_proto (GPUVoltage *proto_stats,
                          const aga_gpu_voltage_t *stats)
{
    proto_stats->set_voltage(stats->voltage);
    proto_stats->set_gfxvoltage(stats->gfx_voltage);
    proto_stats->set_memoryvoltage(stats->memory_voltage);
}

// populate GPU XGMI link statistics
static inline void
aga_gpu_xgmi_link_stats_to_proto (amdgpu::GPUXGMILinkStats *proto_stats,
                                  const aga_gpu_xgmi_link_stats_t *stats)
{
    proto_stats->set_dataread(stats->data_read);
    proto_stats->set_datawrite(stats->data_write);
}

// populate GPU violation statistics
static inline void
aga_gpu_violation_stats_to_proto (amdgpu::GPUViolationStats *proto_stats,
                                  const aga_gpu_violation_stats_t *stats)
{
    proto_stats->set_currentaccumulatedcounter(
                     stats->current_accumulated_counter);
    proto_stats->set_processorhotresidencyaccumulated(
                     stats->processor_hot_residency_accumulated);
    proto_stats->set_pptresidencyaccumulated(
                     stats->ppt_residency_accumulated);
    proto_stats->set_socketthermalresidencyaccumulated(
                     stats->socket_thermal_residency_accumulated);
    proto_stats->set_vrthermalresidencyaccumulated(
                     stats->vr_thermal_residency_accumulated);
    proto_stats->set_hbmthermalresidencyaccumulated(
                     stats->hbm_thermal_residency_accumulated);
}

// populate proto buf stats from gpu stats
static inline void
aga_gpu_api_stats_to_proto (GPUStats *proto_stats,
                            const aga_gpu_stats_t *stats)
{
    proto_stats->set_packagepower(stats->package_power);
    proto_stats->set_avgpackagepower(stats->avg_package_power);
    aga_gpu_temp_stats_to_proto(proto_stats->mutable_temperature(),
                                &stats->temperature);
    aga_gpu_usage_stats_to_proto(proto_stats->mutable_usage(),
                                 &stats->usage);
    aga_gpu_voltage_to_proto(proto_stats->mutable_voltage(),
                             &stats->voltage);
    aga_gpu_pcie_stats_to_proto(proto_stats->mutable_pciestats(),
                                &stats->pcie_stats);
    aga_gpu_vram_usage_stats_to_proto(proto_stats->mutable_vramusage(),
                                      &stats->vram_usage);
    proto_stats->set_energyconsumed(stats->energy_consumed);
    proto_stats->set_powerusage(stats->power_usage);
    proto_stats->set_totalcorrectableerrors(stats->total_correctable_errors);
    proto_stats->set_totaluncorrectableerrors(
                     stats->total_uncorrectable_errors);
    proto_stats->set_sdmacorrectableerrors(stats->sdma_correctable_errors);
    proto_stats->set_sdmauncorrectableerrors(stats->sdma_uncorrectable_errors);
    proto_stats->set_gfxcorrectableerrors(stats->gfx_correctable_errors);
    proto_stats->set_gfxuncorrectableerrors(stats->gfx_uncorrectable_errors);
    proto_stats->set_mmhubcorrectableerrors(stats->mmhub_correctable_errors);
    proto_stats->set_mmhubuncorrectableerrors(
                     stats->mmhub_uncorrectable_errors);
    proto_stats->set_athubcorrectableerrors(stats->athub_correctable_errors);
    proto_stats->set_athubuncorrectableerrors(
                     stats->athub_uncorrectable_errors);
    proto_stats->set_bifcorrectableerrors(stats->bif_correctable_errors);
    proto_stats->set_bifuncorrectableerrors(stats->bif_uncorrectable_errors);
    proto_stats->set_hdpcorrectableerrors(stats->hdp_correctable_errors);
    proto_stats->set_hdpuncorrectableerrors(stats->hdp_uncorrectable_errors);
    proto_stats->set_xgmiwaflcorrectableerrors(
                     stats->xgmi_wafl_correctable_errors);
    proto_stats->set_xgmiwafluncorrectableerrors(
                     stats->xgmi_wafl_uncorrectable_errors);
    proto_stats->set_dfcorrectableerrors(stats->df_correctable_errors);
    proto_stats->set_dfuncorrectableerrors(stats->df_uncorrectable_errors);
    proto_stats->set_smncorrectableerrors(stats->smn_correctable_errors);
    proto_stats->set_smnuncorrectableerrors(stats->smn_uncorrectable_errors);
    proto_stats->set_semcorrectableerrors(stats->sem_correctable_errors);
    proto_stats->set_semuncorrectableerrors(stats->sem_uncorrectable_errors);
    proto_stats->set_mp0correctableerrors(stats->mp0_correctable_errors);
    proto_stats->set_mp0uncorrectableerrors(stats->mp0_uncorrectable_errors);
    proto_stats->set_mp1correctableerrors(stats->mp1_correctable_errors);
    proto_stats->set_mp1uncorrectableerrors(stats->mp1_uncorrectable_errors);
    proto_stats->set_fusecorrectableerrors(stats->fuse_correctable_errors);
    proto_stats->set_fuseuncorrectableerrors(stats->fuse_uncorrectable_errors);
    proto_stats->set_umccorrectableerrors(stats->umc_correctable_errors);
    proto_stats->set_umcuncorrectableerrors(stats->umc_uncorrectable_errors);
    proto_stats->set_mcacorrectableerrors(stats->mca_correctable_errors);
    proto_stats->set_mcauncorrectableerrors(stats->mca_uncorrectable_errors);
    proto_stats->set_vcncorrectableerrors(stats->vcn_correctable_errors);
    proto_stats->set_vcnuncorrectableerrors(stats->vcn_uncorrectable_errors);
    proto_stats->set_jpegcorrectableerrors(stats->jpeg_correctable_errors);
    proto_stats->set_jpeguncorrectableerrors(stats->jpeg_uncorrectable_errors);
    proto_stats->set_ihcorrectableerrors(stats->ih_correctable_errors);
    proto_stats->set_ihuncorrectableerrors(stats->ih_uncorrectable_errors);
    proto_stats->set_mpiocorrectableerrors(stats->mpio_correctable_errors);
    proto_stats->set_mpiouncorrectableerrors(stats->mpio_uncorrectable_errors);
    proto_stats->set_xgmineighbor0txnops(stats->xgmi_neighbor0_tx_nops);
    proto_stats->set_xgmineighbor0txrequests(stats->xgmi_neighbor0_tx_requests);
    proto_stats->set_xgmineighbor0txresponses
                     (stats->xgmi_neighbor0_tx_responses);
    proto_stats->set_xgmineighbor0txbeats(stats->xgmi_neighbor0_tx_beats);
    proto_stats->set_xgmineighbor1txnops(stats->xgmi_neighbor1_tx_nops);
    proto_stats->set_xgmineighbor1txrequests(stats->xgmi_neighbor1_tx_requests);
    proto_stats->set_xgmineighbor1txresponses
                     (stats->xgmi_neighbor1_tx_responses);
    proto_stats->set_xgmineighbor1txbeats(stats->xgmi_neighbor1_tx_beats);
    proto_stats->set_xgmineighbor0txthroughput(
                     stats->xgmi_neighbor0_tx_throughput);
    proto_stats->set_xgmineighbor1txthroughput(
                     stats->xgmi_neighbor1_tx_throughput);
    proto_stats->set_xgmineighbor2txthroughput(
                     stats->xgmi_neighbor2_tx_throughput);
    proto_stats->set_xgmineighbor3txthroughput(
                     stats->xgmi_neighbor3_tx_throughput);
    proto_stats->set_xgmineighbor4txthroughput(
                     stats->xgmi_neighbor4_tx_throughput);
    proto_stats->set_xgmineighbor5txthroughput(
                     stats->xgmi_neighbor5_tx_throughput);
    proto_stats->set_fanspeed(stats->fan_speed);
    proto_stats->set_gfxactivityaccumulated(stats->gfx_activity_accumulated);
    proto_stats->set_memoryactivityaccumulated(stats->mem_activity_accumulated);
    for (uint32_t i = 0; i < AGA_GPU_MAX_XGMI_LINKS; i++) {
        aga_gpu_xgmi_link_stats_to_proto(proto_stats->add_xgmilinkstats(),
                                         &stats->xgmi_link_stats[i]);
    }
    aga_gpu_violation_stats_to_proto(proto_stats->mutable_violationstats(),
                                     &stats->violation_stats);
}

// populate proto buf from gpu info
static inline void
aga_gpu_api_info_to_proto (aga_gpu_info_t *info, void *ctxt)
{
    GPUGetResponse *proto_rsp = (GPUGetResponse *)ctxt;
    auto gpu = proto_rsp->add_response();
    GPUSpec *proto_spec = gpu->mutable_spec();
    GPUStatus *proto_status = gpu->mutable_status();
    GPUStats *proto_stats = gpu->mutable_stats();

    aga_gpu_api_spec_to_proto(proto_spec, &info->spec);
    aga_gpu_api_status_to_proto(proto_status, &info->status);
    aga_gpu_api_stats_to_proto(proto_stats, &info->stats);
}

#endif    // __AGA_SVC_GPU_TO_PROTO_HPP__
