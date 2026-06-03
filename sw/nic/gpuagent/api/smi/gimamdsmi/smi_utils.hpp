//
// Copyright(C) Advanced Micro Devices, Inc. All rights reserved.
//
// You may not use this software and documentation (if any) (collectively,
// the "Materials") except in compliance with the terms and conditions of
// the Software License Agreement included with the Materials or otherwise as
// set forth in writing and signed by you and an authorized signatory of AMD.
// If you do not have a copy of the Software License Agreement, contact your
// AMD representative for a copy.
//
// You agree that you will not reverse engineer or decompile the Materials,
// in whole or in part, except as allowed by applicable law.
//
// THE MATERIALS ARE DISTRIBUTED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OR
// REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//----------------------------------------------------------------------------
///
/// \file
/// smi layer helper functions
///
//----------------------------------------------------------------------------

#ifndef __AGA_API_SMI_UTILS_HPP__
#define __AGA_API_SMI_UTILS_HPP__

extern "C" {
#include "nic/third-party/rocm/gim_amd_smi_lib/include/amd_smi/amdsmi.h"
}
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/api/include/aga_event.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"

namespace aga {

/// \defgroup AGA_SMI - smi module APIs
/// \ingroup AGA
/// @{
///
/// NOTE:
/// for UUID->string conversion needed by amdsmi APIs (e.g.
/// amdsmi_get_processor_handle_from_uuid), use the canonical
/// aga_obj_key_t::str() helper in nic/gpuagent/api/include/base.hpp

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

/// \brief copy amdsmi VRAM vendor string to output buffer
/// \param[in]  vendor_src  amdsmi vendor string (from amdsmi_vram_info_t)
/// \param[out] vendor_str  output vendor string
/// \param[in]  str_size    output buffer maximum size
static inline void
smi_vram_vendor_to_string (const char *vendor_src, char *vendor_str,
                           uint32_t str_size)
{
    strncpy(vendor_str, vendor_src, str_size);
    vendor_str[str_size - 1] = '\0';
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

/// \brief convert amdsmi partition type string to aga comptue partition type
/// \param[in] acceleerator partition type
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

/// \brief convert aga compute partition to amdsmi accelerator partition type
/// \param[in] partition_type aga compute partition type
/// \return    amdsmi gpu accelerator partition type
static inline amdsmi_accelerator_partition_type_t
aga_to_smi_gpu_compute_partition_type (
    aga_gpu_compute_partition_type_t partition_type)
{
    switch (partition_type) {
    case AGA_GPU_COMPUTE_PARTITION_TYPE_SPX:
        return AMDSMI_ACCELERATOR_PARTITION_SPX;
    case AGA_GPU_COMPUTE_PARTITION_TYPE_DPX:
        return AMDSMI_ACCELERATOR_PARTITION_DPX;
    case AGA_GPU_COMPUTE_PARTITION_TYPE_TPX:
        return AMDSMI_ACCELERATOR_PARTITION_TPX;
    case AGA_GPU_COMPUTE_PARTITION_TYPE_QPX:
        return AMDSMI_ACCELERATOR_PARTITION_QPX;
    case AGA_GPU_COMPUTE_PARTITION_TYPE_CPX:
        return AMDSMI_ACCELERATOR_PARTITION_CPX;
    default:
        break;
    }
    return AMDSMI_ACCELERATOR_PARTITION_INVALID;
}

/// \brief convert amdsmi memory partition type to aga memory partition type
/// \param[in] partition_type amdsmi memory partition type
/// \return    aga gpu memory partition type
static inline aga_gpu_memory_partition_type_t
smi_to_aga_gpu_memory_partition_type (
    amdsmi_memory_partition_type_t partition_type)
{
    switch (partition_type) {
    case AMDSMI_MEMORY_PARTITION_NPS1:
        return AGA_GPU_MEMORY_PARTITION_TYPE_NPS1;
    case AMDSMI_MEMORY_PARTITION_NPS2:
        return AGA_GPU_MEMORY_PARTITION_TYPE_NPS2;
    case AMDSMI_MEMORY_PARTITION_NPS4:
        return AGA_GPU_MEMORY_PARTITION_TYPE_NPS4;
    case AMDSMI_MEMORY_PARTITION_NPS8:
        return AGA_GPU_MEMORY_PARTITION_TYPE_NPS8;
    default:
        break;
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
