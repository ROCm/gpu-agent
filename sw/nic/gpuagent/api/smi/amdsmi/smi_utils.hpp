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

#include "nic/third-party/rocm/amd_smi_lib/include/amd_smi/amdsmi.h"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/api/include/aga_event.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"

namespace aga {

/// \defgroup AGA_SMI - smi module APIs
/// \ingroup AGA
/// @{

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

/// \brief convert amdsmi VRAM vendor to aga VRAM vendor
/// \param[in] vendor    amdsmi vendor
/// \return    aga vendor
static inline aga_vram_vendor_t
smi_to_aga_vram_vendor (amdsmi_vram_vendor_type_t vendor)
{
    switch (vendor) {
    case AMDSMI_VRAM_VENDOR__SAMSUNG:
        return AGA_VRAM_VENDOR_SAMSUNG;
    case AMDSMI_VRAM_VENDOR__INFINEON:
        return AGA_VRAM_VENDOR_INFINEON;
    case AMDSMI_VRAM_VENDOR__ELPIDA:
        return AGA_VRAM_VENDOR_ELPIDA;
    case AMDSMI_VRAM_VENDOR__ETRON:
        return AGA_VRAM_VENDOR_ETRON;
    case AMDSMI_VRAM_VENDOR__NANYA:
        return AGA_VRAM_VENDOR_NANYA;
    case AMDSMI_VRAM_VENDOR__HYNIX:
        return AGA_VRAM_VENDOR_HYNIX;
    case AMDSMI_VRAM_VENDOR__MOSEL:
        return AGA_VRAM_VENDOR_MOSEL;
    case AMDSMI_VRAM_VENDOR__WINBOND:
        return AGA_VRAM_VENDOR_WINBOND;
    case AMDSMI_VRAM_VENDOR__ESMT:
        return AGA_VRAM_VENDOR_ESMT;
    case AMDSMI_VRAM_VENDOR__MICRON:
        return AGA_VRAM_VENDOR_MICRON;
    case AMDSMI_VRAM_VENDOR__PLACEHOLDER0:
    case AMDSMI_VRAM_VENDOR__PLACEHOLDER1:
    case AMDSMI_VRAM_VENDOR__PLACEHOLDER2:
    case AMDSMI_VRAM_VENDOR__PLACEHOLDER3:
    case AMDSMI_VRAM_VENDOR__PLACEHOLDER4:
    case AMDSMI_VRAM_VENDOR__PLACEHOLDER5:
        return AGA_VRAM_VENDOR_UNKNOWN;
    default:
        break;
    }
    return AGA_VRAM_VENDOR_NONE;
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
    switch (amdsmi_event) {
    case AMDSMI_EVT_NOTIF_VMFAULT:
        return AGA_EVENT_ID_VM_PAGE_FAULT;
    case AMDSMI_EVT_NOTIF_THERMAL_THROTTLE:
        return AGA_EVENT_ID_THERMAL_THROTTLE;
    case AMDSMI_EVT_NOTIF_GPU_PRE_RESET:
        return AGA_EVENT_ID_GPU_PRE_RESET;
    case AMDSMI_EVT_NOTIF_GPU_POST_RESET:
        return AGA_EVENT_ID_GPU_POST_RESET;
    case AMDSMI_EVT_NOTIF_RING_HANG:
        return AGA_EVENT_ID_RING_HANG;
    default:
        break;
    }
    return AGA_EVENT_ID_NONE;
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
    case AGA_EVENT_ID_RING_HANG:
        *amdsmi_event = AMDSMI_EVT_NOTIF_RING_HANG;
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

/// \brief convert amdsmi partition type string to aga comptue partition type
/// \param[in] partition_type amdsmi partition type string
/// \return    aga gpu compute partition type
static inline aga_gpu_compute_partition_type_t
smi_to_aga_gpu_compute_partition_type (std::string partition_type)
{
    if (partition_type == "SPX") {
        return AGA_GPU_COMPUTE_PARTITION_TYPE_SPX;
    } else if (partition_type == "DPX") {
        return AGA_GPU_COMPUTE_PARTITION_TYPE_DPX;
    } else if (partition_type == "TPX") {
        return AGA_GPU_COMPUTE_PARTITION_TYPE_TPX;
    } else if (partition_type == "QPX") {
        return AGA_GPU_COMPUTE_PARTITION_TYPE_QPX;
    } else if (partition_type == "CPX") {
        return AGA_GPU_COMPUTE_PARTITION_TYPE_CPX;
    }

    return AGA_GPU_COMPUTE_PARTITION_TYPE_NONE;
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
