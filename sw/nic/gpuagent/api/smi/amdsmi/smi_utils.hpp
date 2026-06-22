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
/// smi layer helper functions
///
//----------------------------------------------------------------------------

#ifndef __AGA_API_SMI_UTILS_HPP__
#define __AGA_API_SMI_UTILS_HPP__

#include <vector>
#include <algorithm>
extern "C" {
#include "nic/third-party/rocm/amd_smi_lib/include/amd_smi/amdsmi.h"
}
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/api/include/aga_event.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"

namespace aga {

/// \defgroup AGA_SMI - smi module APIs
/// \ingroup AGA
/// @{

/// \brief function to get the low and high frequencies for a clock type
/// \param[in] freq    supported frequencies struct from amdsmi
/// \param[out] min    minimum supported frequency in MHz
/// \param[out] max    maximum supported frequency in MHz
static inline void
find_low_high_frequency (amdsmi_frequencies_t *freq,
                         uint32_t *min, uint32_t *max)
{
    // clamp num_supported to the array bound before building the vector
    uint32_t n = (freq->num_supported < AMDSMI_MAX_NUM_FREQUENCIES) ?
                      freq->num_supported : AMDSMI_MAX_NUM_FREQUENCIES;
    std::vector<uint64_t> f(freq->frequency, freq->frequency + n);

    // sort vector
    std::sort(f.begin(), f.end());

    // set default values
    *min = 0xffffffff;
    *max = 0xffffffff;

    if (freq->has_deep_sleep) {
        // lowest supported freq will be deep sleep value; set the second lowest
        // frequency to be the minimum
        if (f.size() > 1) {
            *min = (uint32_t)(f[1]/1000000);
        }
    } else {
        // lowest supported frequency
        if (f.size() >= 1) {
            *min = (uint32_t)(f[0]/1000000);
        }
    }
    if (f.size() >= 1) {
        // largest supported frequency
        *max = (uint32_t)(f[f.size() - 1]/1000000);
    }
    return;
}

/// \brief return current raw frequency in Hz, clamping an out-of-bounds index
/// \param[in] freq    frequencies struct from amdsmi
/// \return    current frequency in Hz; 0 when no supported frequencies,
///            else the first entry if current is out of range
static inline uint64_t
current_frequency_hz (amdsmi_frequencies_t *freq)
{
    uint32_t n, idx;

    if (freq->num_supported == 0) {
        return 0;
    }
    n = (freq->num_supported < AMDSMI_MAX_NUM_FREQUENCIES) ?
             freq->num_supported : AMDSMI_MAX_NUM_FREQUENCIES;
    idx = (freq->current < n) ? freq->current : 0;
    return freq->frequency[idx];
}

/// \brief convert amdsmi virtualization mode to aga vritualization mode
/// \param[in] virt_mode    amdsmi virtualization mode
/// \return    aga virtualization mode
static inline aga_gpu_virtualization_mode_t
smi_to_aga_virtualization_mode (amdsmi_virtualization_mode_t virt_mode)
{
    switch (virt_mode) {
    case AMDSMI_VIRTUALIZATION_MODE_HOST:
        return AGA_VIRTUALIZATION_MODE_HOST;
    case AMDSMI_VIRTUALIZATION_MODE_GUEST:
        return AGA_VIRTUALIZATION_MODE_GUEST;
    case AMDSMI_VIRTUALIZATION_MODE_PASSTHROUGH:
        return AGA_VIRTUALIZATION_MODE_PASSTHROUGH;
    default:
        break;
    }
    return AGA_VIRTUALIZATION_MODE_UNKNOWN;
}

/// \brief convert amdsmi VRAM type to aga VRAM type
/// \param[in] vram_type    amdsmi VRAM type
/// \return    aga VRAM type
static inline aga_vram_type_t
smi_to_aga_vram_type (amdsmi_vram_type_t vram_type)
{
    switch (vram_type) {
    case AMDSMI_VRAM_TYPE_HBM:
        return AGA_VRAM_TYPE_HBM;
    case AMDSMI_VRAM_TYPE_HBM2:
        return AGA_VRAM_TYPE_HBM2;
    case AMDSMI_VRAM_TYPE_HBM2E:
        return AGA_VRAM_TYPE_HBM2E;
    case AMDSMI_VRAM_TYPE_HBM3:
        return AGA_VRAM_TYPE_HBM3;
    case AMDSMI_VRAM_TYPE_DDR2:
        return AGA_VRAM_TYPE_DDR2;
    case AMDSMI_VRAM_TYPE_DDR3:
        return AGA_VRAM_TYPE_DDR3;
    case AMDSMI_VRAM_TYPE_DDR4:
        return AGA_VRAM_TYPE_DDR4;
    case AMDSMI_VRAM_TYPE_GDDR1:
        return AGA_VRAM_TYPE_GDDR1;
    case AMDSMI_VRAM_TYPE_GDDR2:
        return AGA_VRAM_TYPE_GDDR2;
    case AMDSMI_VRAM_TYPE_GDDR3:
        return AGA_VRAM_TYPE_GDDR3;
    case AMDSMI_VRAM_TYPE_GDDR4:
        return AGA_VRAM_TYPE_GDDR4;
    case AMDSMI_VRAM_TYPE_GDDR5:
        return AGA_VRAM_TYPE_GDDR5;
    case AMDSMI_VRAM_TYPE_GDDR6:
        return AGA_VRAM_TYPE_GDDR6;
    case AMDSMI_VRAM_TYPE_GDDR7:
        return AGA_VRAM_TYPE_GDDR7;
    case AMDSMI_VRAM_TYPE_UNKNOWN:
        return AGA_VRAM_TYPE_UNKNOWN;
    default:
        break;
    }
    return AGA_VRAM_TYPE_NONE;
}

/// \brief convert amdsmi clock type to aga clock type
/// \param[in] clock_type    amdsmi clock type
/// \return    aga clock type
static inline aga_gpu_clock_type_t
smi_to_aga_gpu_clock_type (amdsmi_clk_type_t clock_type)
{
    switch (clock_type) {
    case AMDSMI_CLK_TYPE_GFX:
        return AGA_GPU_CLOCK_TYPE_SYSTEM;
    case AMDSMI_CLK_TYPE_DF:
        return AGA_GPU_CLOCK_TYPE_FABRIC;
    case AMDSMI_CLK_TYPE_DCEF:
        return AGA_GPU_CLOCK_TYPE_DCE;
    case AMDSMI_CLK_TYPE_SOC:
        return AGA_GPU_CLOCK_TYPE_SOC;
    case AMDSMI_CLK_TYPE_MEM:
        return AGA_GPU_CLOCK_TYPE_MEMORY;
    case AMDSMI_CLK_TYPE_PCIE:
        return AGA_GPU_CLOCK_TYPE_PCIE;
    case AMDSMI_CLK_TYPE_VCLK0:
    case AMDSMI_CLK_TYPE_VCLK1:
        return AGA_GPU_CLOCK_TYPE_VIDEO;
    case AMDSMI_CLK_TYPE_DCLK0:
    case AMDSMI_CLK_TYPE_DCLK1:
        return AGA_GPU_CLOCK_TYPE_DATA;
    default:
        break;
    }
    return AGA_GPU_CLOCK_TYPE_NONE;
}

/// \brief convert aga clock type to amdsmi clock type
/// \param[in]  clock_type          aga clock type
/// \param[out] amdsmi_clock_type   amdsmi clock type
/// \return SDK_RET_OK or error status in case of failure
static inline sdk_ret_t
aga_to_smi_gpu_clock_type (aga_gpu_clock_type_t clock_type,
                           amdsmi_clk_type_t *amdsmi_clock_type)
{
    switch (clock_type) {
    case AGA_GPU_CLOCK_TYPE_SYSTEM:
        *amdsmi_clock_type =  AMDSMI_CLK_TYPE_GFX;
        break;
    case AGA_GPU_CLOCK_TYPE_FABRIC:
        *amdsmi_clock_type =  AMDSMI_CLK_TYPE_DF;
        break;
    case AGA_GPU_CLOCK_TYPE_DCE:
        *amdsmi_clock_type =  AMDSMI_CLK_TYPE_DCEF;
        break;
    case AGA_GPU_CLOCK_TYPE_SOC:
        *amdsmi_clock_type =  AMDSMI_CLK_TYPE_SOC;
        break;
    case AGA_GPU_CLOCK_TYPE_MEMORY:
        *amdsmi_clock_type =  AMDSMI_CLK_TYPE_MEM;
        break;
    case AGA_GPU_CLOCK_TYPE_PCIE:
        *amdsmi_clock_type =  AMDSMI_CLK_TYPE_PCIE;
        break;
    case AGA_GPU_CLOCK_TYPE_VIDEO:
        *amdsmi_clock_type =  AMDSMI_CLK_TYPE_VCLK0;
        break;
    case AGA_GPU_CLOCK_TYPE_DATA:
        *amdsmi_clock_type =  AMDSMI_CLK_TYPE_DCLK0;
        break;
    default:
        return SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

/// \brief convert amdsmi PCIe slot type to aga slot type
/// \param[in] slot type    amdsmi slot type
/// \return    aga slot type
static inline aga_pcie_slot_type_t
smi_to_aga_pcie_slot_type (amdsmi_card_form_factor_t slot_type)
{
    switch (slot_type) {
    case AMDSMI_CARD_FORM_FACTOR_PCIE:
        return AGA_PCIE_SLOT_TYPE_PCIE;
    case AMDSMI_CARD_FORM_FACTOR_OAM:
        return AGA_PCIE_SLOT_TYPE_OAM;
    case AMDSMI_CARD_FORM_FACTOR_CEM:
        return AGA_PCIE_SLOT_TYPE_CEM;
    case AMDSMI_CARD_FORM_FACTOR_UNKNOWN:
        return AGA_PCIE_SLOT_TYPE_UNKNOWN;
    default:
        break;
    }
    return AGA_PCIE_SLOT_TYPE_NONE;
}

/// \brief convert amdsmi perf_level to gpu perf_level
/// \param[in] perf_level    amdsmi performance level
/// \return    aga performance level
static inline aga_gpu_perf_level_t
smi_to_aga_gpu_perf_level (amdsmi_dev_perf_level_t perf_level)
{
    switch (perf_level) {
    case AMDSMI_DEV_PERF_LEVEL_AUTO:
        return AGA_GPU_PERF_LEVEL_AUTO;
    case AMDSMI_DEV_PERF_LEVEL_LOW:
        return AGA_GPU_PERF_LEVEL_LOW;
    case AMDSMI_DEV_PERF_LEVEL_HIGH:
        return AGA_GPU_PERF_LEVEL_HIGH;
    case AMDSMI_DEV_PERF_LEVEL_DETERMINISM:
        return AGA_GPU_PERF_LEVEL_DETERMINISTIC;
    case AMDSMI_DEV_PERF_LEVEL_STABLE_MIN_SCLK:
        return AGA_GPU_PERF_LEVEL_STABLE_WITH_SCLK;
    case AMDSMI_DEV_PERF_LEVEL_STABLE_MIN_MCLK:
        return AGA_GPU_PERF_LEVEL_STABLE_WITH_MCLK;
    case AMDSMI_DEV_PERF_LEVEL_MANUAL:
        return AGA_GPU_PERF_LEVEL_MANUAL;
    default:
        break;
    }
    return AGA_GPU_PERF_LEVEL_NONE;
}

/// \brief convert gpu perf_level to amdsmi perf_level
/// \param[in] perf_level    aga performance level
/// \return    rsmsi performance level
static inline amdsmi_dev_perf_level_t
aga_to_smi_gpu_perf_level (aga_gpu_perf_level_t perf_level)
{
    switch (perf_level) {
    case AGA_GPU_PERF_LEVEL_AUTO:
        return AMDSMI_DEV_PERF_LEVEL_AUTO;
    case AGA_GPU_PERF_LEVEL_LOW:
        return AMDSMI_DEV_PERF_LEVEL_LOW;
    case AGA_GPU_PERF_LEVEL_HIGH:
        return AMDSMI_DEV_PERF_LEVEL_HIGH;
    case AGA_GPU_PERF_LEVEL_DETERMINISTIC:
        return AMDSMI_DEV_PERF_LEVEL_DETERMINISM;
    case AGA_GPU_PERF_LEVEL_STABLE_WITH_SCLK:
        return AMDSMI_DEV_PERF_LEVEL_STABLE_MIN_SCLK;
    case AGA_GPU_PERF_LEVEL_STABLE_WITH_MCLK:
        return AMDSMI_DEV_PERF_LEVEL_STABLE_MIN_MCLK;
    case AGA_GPU_PERF_LEVEL_MANUAL:
        return AMDSMI_DEV_PERF_LEVEL_MANUAL;
    default:
        break;
    }
    return AMDSMI_DEV_PERF_LEVEL_UNKNOWN;
}

/// \brief    convert amdsmi event id to aga event id
/// \param[in] amdsmi_event rocm-smi event id
/// \return aga event id
static inline aga_event_id_t
aga_event_id_from_smi_event_id (amdsmi_evt_notification_type_t amdsmi_event)
{
    return (aga_event_id_t)amdsmi_event;
}

/// \brief      convert aga event id to amdsmi event id
/// \param[in]  event    aga event id
/// \param[out] amdsmi_event    rocm-smi event id
/// \return SDK_RET_OK or error status in case of failure
static inline sdk_ret_t
aga_event_id_to_smi_event_id (aga_event_id_t event,
                              amdsmi_evt_notification_type_t *amdsmi_event)
{
    switch (event) {
    case AGA_EVENT_ID_VM_PAGE_FAULT:
        *amdsmi_event = AMDSMI_EVT_NOTIF_VMFAULT;
        break;
    case AGA_EVENT_ID_THERMAL_THROTTLE:
        *amdsmi_event = AMDSMI_EVT_NOTIF_THERMAL_THROTTLE;
        break;
    case AGA_EVENT_ID_GPU_PRE_RESET:
        *amdsmi_event = AMDSMI_EVT_NOTIF_GPU_PRE_RESET;
        break;
    case AGA_EVENT_ID_GPU_POST_RESET:
        *amdsmi_event = AMDSMI_EVT_NOTIF_GPU_POST_RESET;
        break;
    default:
        return SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

/// \brief convert amdsmi xgmi error status to gpu xgmi error
/// \param[in] xe    smi xgmi error status
/// \return     aga xgmi performance level
static inline aga_gpu_xgmi_error_status_t
smi_to_aga_gpu_xgmi_error (amdsmi_xgmi_status_t xe)
{
    switch (xe) {
    case AMDSMI_XGMI_STATUS_NO_ERRORS:
        return AGA_GPU_XGMI_STATUS_NO_ERROR;
    case AMDSMI_XGMI_STATUS_ERROR:
        return AGA_GPU_XGMI_STATUS_ONE_ERROR;
    case AMDSMI_XGMI_STATUS_MULTIPLE_ERRORS:
        return AGA_GPU_XGMI_STATUS_MULTIPLE_ERROR;
    default:
        break;
    }
    return AGA_GPU_XGMI_STATUS_NONE;
}

/// \brief convert amdsmi page status to aga page status
/// \param[in] page_status    amdsmi page status
/// \return     aga xgmi performance level
static inline aga_gpu_page_status_t
smi_to_aga_gpu_page_status (amdsmi_memory_page_status_t page_status)
{
    switch (page_status) {
    case AMDSMI_MEM_PAGE_STATUS_RESERVED:
        return AGA_GPU_PAGE_STATUS_RESERVED;
    case AMDSMI_MEM_PAGE_STATUS_PENDING:
        return AGA_GPU_PAGE_STATUS_PENDING;
    case AMDSMI_MEM_PAGE_STATUS_UNRESERVABLE:
        return AGA_GPU_PAGE_STATUS_UNRESERVABLE;
    default:
        break;
    }
    return AGA_GPU_PAGE_STATUS_NONE;
}

/// \brief convert amdsmi accelerator partition to aga comptue partition type
/// \param[in] partition_type amdsmi partition type string
/// \return    aga gpu compute partition type
static inline aga_gpu_compute_partition_type_t
smi_to_aga_gpu_compute_partition_type (
    amdsmi_accelerator_partition_type_t partition_type)
{
    switch (partition_type) {
    case AMDSMI_ACCELERATOR_PARTITION_SPX:
        return AGA_GPU_COMPUTE_PARTITION_TYPE_SPX;
    case AMDSMI_ACCELERATOR_PARTITION_DPX:
        return AGA_GPU_COMPUTE_PARTITION_TYPE_DPX;
    case AMDSMI_ACCELERATOR_PARTITION_TPX:
        return AGA_GPU_COMPUTE_PARTITION_TYPE_TPX;
    case AMDSMI_ACCELERATOR_PARTITION_QPX:
        return AGA_GPU_COMPUTE_PARTITION_TYPE_QPX;
    case AMDSMI_ACCELERATOR_PARTITION_CPX:
        return AGA_GPU_COMPUTE_PARTITION_TYPE_CPX;
    default:
        break;
    }


    return AGA_GPU_COMPUTE_PARTITION_TYPE_NONE;
}

/// \brief convert aga compute partition type to amdsmi comptue partition type
/// \param[in] partition_type aga compute partition type
/// \return    amdsmi gpu compute partition type
static inline amdsmi_compute_partition_type_t
aga_to_smi_gpu_compute_partition_type (
    aga_gpu_compute_partition_type_t partition_type)
{
    switch (partition_type) {
    case AGA_GPU_COMPUTE_PARTITION_TYPE_SPX:
        return AMDSMI_COMPUTE_PARTITION_SPX;
    case AGA_GPU_COMPUTE_PARTITION_TYPE_DPX:
        return AMDSMI_COMPUTE_PARTITION_DPX;
    case AGA_GPU_COMPUTE_PARTITION_TYPE_TPX:
        return AMDSMI_COMPUTE_PARTITION_TPX;
    case AGA_GPU_COMPUTE_PARTITION_TYPE_QPX:
        return AMDSMI_COMPUTE_PARTITION_QPX;
    case AGA_GPU_COMPUTE_PARTITION_TYPE_CPX:
        return AMDSMI_COMPUTE_PARTITION_CPX;
    default:
        break;
    }
    return AMDSMI_COMPUTE_PARTITION_INVALID;
}

/// \brief convert amdsmi memory partition type string to aga memory partition
///        type
/// \param[in] partition_type amdsmi memory partition type string
/// \return    aga gpu memory partition type
static inline aga_gpu_memory_partition_type_t
smi_to_aga_gpu_memory_partition_type (std::string partition_type)
{
    if (partition_type == "NPS1") {
        return AGA_GPU_MEMORY_PARTITION_TYPE_NPS1;
    } else if (partition_type == "NPS2") {
        return AGA_GPU_MEMORY_PARTITION_TYPE_NPS2;
    } else if (partition_type == "NPS4") {
        return AGA_GPU_MEMORY_PARTITION_TYPE_NPS4;
    } else if (partition_type == "NPS8") {
        return AGA_GPU_MEMORY_PARTITION_TYPE_NPS8;
    }

    return AGA_GPU_MEMORY_PARTITION_TYPE_NONE;
}

/// \brief convert aga memory partition type to amdsmi memory partition type
/// \param[in] partition_type aga memory partition type
/// \return    amdsmi gpu memory partition type
static inline amdsmi_memory_partition_type_t
aga_to_smi_gpu_memory_partition_type (
    aga_gpu_memory_partition_type_t partition_type)
{
    switch (partition_type) {
    case AGA_GPU_MEMORY_PARTITION_TYPE_NPS1:
        return AMDSMI_MEMORY_PARTITION_NPS1;
    case AGA_GPU_MEMORY_PARTITION_TYPE_NPS2:
        return AMDSMI_MEMORY_PARTITION_NPS2;
    case AGA_GPU_MEMORY_PARTITION_TYPE_NPS4:
        return AMDSMI_MEMORY_PARTITION_NPS4;
    case AGA_GPU_MEMORY_PARTITION_TYPE_NPS8:
        return AMDSMI_MEMORY_PARTITION_NPS8;
    default:
        break;
    }
    return AMDSMI_MEMORY_PARTITION_UNKNOWN;
}

/// \brief convert amdsmi power cap type to aga power cap type
/// \param[in] power_cap_type    amdsmi power cap type
/// \return    aga power cap type
static inline aga_gpu_power_cap_type_t
smi_to_aga_power_cap_type (amdsmi_power_cap_type_t power_cap_type)
{
    switch (power_cap_type) {
    case AMDSMI_POWER_CAP_TYPE_PPT0:
        return AGA_GPU_POWER_CAP_TYPE_PPT0;
    case AMDSMI_POWER_CAP_TYPE_PPT1:
        return AGA_GPU_POWER_CAP_TYPE_PPT1;
    default:
        break;
    }

    return AGA_GPU_POWER_CAP_TYPE_NONE;
}

/// \brief convert amdsmi CPER severity to aga CPER severity
/// \param[in] amdsmi CPER severity
/// \return    aga CPER severity
static inline aga_cper_severity_t
smi_to_aga_cper_severity (amdsmi_cper_sev_t severity)
{
    switch (severity) {
    case AMDSMI_CPER_SEV_NON_FATAL_UNCORRECTED:
        return AGA_CPER_SEVERITY_NON_FATAL_UNCORRECTED;
    case AMDSMI_CPER_SEV_FATAL:
        return AGA_CPER_SEVERITY_FATAL;
    case AMDSMI_CPER_SEV_NON_FATAL_CORRECTED:
        return AGA_CPER_SEVERITY_NON_FATAL_CORRECTED;
    default:
        break;
    }

    return AGA_CPER_SEVERITY_NONE;
}

/// \brief convert amdsmi CPER notification type to aga CPER notification type
/// \param[in] amdsmi CPER notification type in amdsmi_cper_guid_t format
/// \return    aga CPER notification type
static inline aga_cper_notification_type_t
smi_to_aga_cper_notification_type (amdsmi_cper_guid_t ntfn_type)
{
    uint64_t amdsmi_ntfn_type;

    amdsmi_ntfn_type = (uint64_t)ntfn_type.b[0]         |
                       ((uint64_t)ntfn_type.b[1] << 8)  |
                       ((uint64_t)ntfn_type.b[2] << 16) |
                       ((uint64_t)ntfn_type.b[3] << 24) |
                       ((uint64_t)ntfn_type.b[4] << 32) |
                       ((uint64_t)ntfn_type.b[5] << 40) |
                       ((uint64_t)ntfn_type.b[6] << 48) |
                       ((uint64_t)ntfn_type.b[7] << 56);

    switch (amdsmi_ntfn_type) {
    case AMDSMI_CPER_NOTIFY_TYPE_CMC:
        return AGA_CPER_NOTIFICATION_TYPE_CMC;
    case AMDSMI_CPER_NOTIFY_TYPE_CPE:
        return AGA_CPER_NOTIFICATION_TYPE_CPE;
    case AMDSMI_CPER_NOTIFY_TYPE_MCE:
        return AGA_CPER_NOTIFICATION_TYPE_MCE;
    case AMDSMI_CPER_NOTIFY_TYPE_PCIE:
        return AGA_CPER_NOTIFICATION_TYPE_PCIE;
    case AMDSMI_CPER_NOTIFY_TYPE_INIT:
        return AGA_CPER_NOTIFICATION_TYPE_INIT;
    case AMDSMI_CPER_NOTIFY_TYPE_NMI:
        return AGA_CPER_NOTIFICATION_TYPE_NMI;
    case AMDSMI_CPER_NOTIFY_TYPE_BOOT:
        return AGA_CPER_NOTIFICATION_TYPE_BOOT;
    case AMDSMI_CPER_NOTIFY_TYPE_DMAR:
        return AGA_CPER_NOTIFICATION_TYPE_DMAR;
    case AMDSMI_CPER_NOTIFY_TYPE_SEA:
        return AGA_CPER_NOTIFICATION_TYPE_SEA;
    case AMDSMI_CPER_NOTIFY_TYPE_SEI:
        return AGA_CPER_NOTIFICATION_TYPE_SEI;
    case AMDSMI_CPER_NOTIFY_TYPE_PEI:
        return AGA_CPER_NOTIFICATION_TYPE_PEI;
    case AMDSMI_CPER_NOTIFY_TYPE_CXL_COMPONENT:
        return AGA_CPER_NOTIFICATION_TYPE_CXL_COMPONENT;
    default:
        break;
    }

    return AGA_CPER_NOTIFICATION_TYPE_NONE;
}

/// \brief     convert amdsmi return status to sdk return status
/// \param[in] amdsmi_ret amdsmi return status
/// \return    sdk return status
static inline sdk_ret_t
amdsmi_ret_to_sdk_ret (amdsmi_status_t amdsmi_ret)
{
    switch (amdsmi_ret) {
    case AMDSMI_STATUS_SUCCESS:
        return SDK_RET_OK;
    case AMDSMI_STATUS_INVAL:
        return SDK_RET_INVALID_ARG;
    case AMDSMI_STATUS_NOT_SUPPORTED:
        return SDK_RET_OP_NOT_SUPPORTED;
    case AMDSMI_STATUS_FILE_ERROR:
        return SDK_RET_FILE_ERR;
    case AMDSMI_STATUS_NO_PERM:
        return SDK_RET_PERMISSION_ERR;
    case AMDSMI_STATUS_OUT_OF_RESOURCES:
        return SDK_RET_OOM;
    case AMDSMI_STATUS_INTERNAL_EXCEPTION:
        return SDK_RET_INTERNAL_EXCEPTION_ERR;
    case AMDSMI_STATUS_INPUT_OUT_OF_BOUNDS:
        return SDK_RET_OOB;
    case AMDSMI_STATUS_INIT_ERROR:
        return SDK_RET_INIT_ERR;
    case AMDSMI_STATUS_NOT_YET_IMPLEMENTED:
        return SDK_RET_OP_NOT_SUPPORTED;
    case AMDSMI_STATUS_NOT_FOUND:
        return SDK_RET_ENTRY_NOT_FOUND;
    case AMDSMI_STATUS_INSUFFICIENT_SIZE:
        return SDK_RET_NO_RESOURCE;
    case AMDSMI_STATUS_INTERRUPT:
        return SDK_RET_INTERRUPT;
    case AMDSMI_STATUS_UNEXPECTED_SIZE:
        return SDK_RET_UNEXPECTED_DATA_SIZE_ERR;
    case AMDSMI_STATUS_NO_DATA:
        return SDK_RET_NO_DATA_ERR;
    case AMDSMI_STATUS_UNEXPECTED_DATA:
        return SDK_RET_UNEXPECTED_DATA_ERR;
    case AMDSMI_STATUS_BUSY:
        return SDK_RET_IN_USE;
    case AMDSMI_STATUS_REFCOUNT_OVERFLOW:
        return SDK_RET_REFCOUNT_OVERFLOW_ERR;
    case AMDSMI_STATUS_SETTING_UNAVAILABLE:
        return SDK_RET_SETTING_UNAVAILABLE_ERR;
    case AMDSMI_STATUS_AMDGPU_RESTART_ERR:
        return SDK_RET_RESTART_ERR;
    default:
        break;
    }
    return SDK_RET_ERR;
}

/// \@}

}    // namespace aga

#endif    // __AGA_API_SMI_UTILS_HPP__
