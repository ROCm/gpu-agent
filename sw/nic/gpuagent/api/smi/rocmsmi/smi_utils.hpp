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

#include "nic/third-party/rocm/rocm_smi_lib/include/rocm_smi/rocm_smi.h"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/api/include/aga_event.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"

namespace aga {

/// \defgroup AGA_SMI - smi module APIs
/// \ingroup AGA
/// @{

/// \brief convert rsmi clock type to aga clock type
/// \param[in] clock_type    rsmi clock type
/// \return    aga clock type
static inline aga_gpu_clock_type_t
smi_to_aga_gpu_clock_type (rsmi_clk_type_t clock_type)
{
    switch (clock_type) {
    case RSMI_CLK_TYPE_SYS:
        return AGA_GPU_CLOCK_TYPE_SYSTEM;
    case RSMI_CLK_TYPE_DF:
        return AGA_GPU_CLOCK_TYPE_FABRIC;
    case RSMI_CLK_TYPE_DCEF:
        return AGA_GPU_CLOCK_TYPE_DCE;
    case RSMI_CLK_TYPE_SOC:
        return AGA_GPU_CLOCK_TYPE_SOC;
    case RSMI_CLK_TYPE_MEM:
        return AGA_GPU_CLOCK_TYPE_MEMORY;
    case RSMI_CLK_TYPE_PCIE:
        return AGA_GPU_CLOCK_TYPE_PCIE;
    default:
        break;
    }
    return AGA_GPU_CLOCK_TYPE_NONE;
}

/// \brief convert aga clock type to rsmi clock type
/// \param[in]  clock_type          aga clock type
/// \param[out] rsmi_clock_type     rsmi clock type
/// \return SDK_RET_OK or error status in case of failure
static inline sdk_ret_t
aga_to_smi_gpu_clock_type (aga_gpu_clock_type_t clock_type,
                           rsmi_clk_type_t *rsmi_clock_type)
{
    switch (clock_type) {
    case AGA_GPU_CLOCK_TYPE_SYSTEM:
        *rsmi_clock_type =  RSMI_CLK_TYPE_SYS;
        break;
    case AGA_GPU_CLOCK_TYPE_FABRIC:
        *rsmi_clock_type =  RSMI_CLK_TYPE_DF;
        break;
    case AGA_GPU_CLOCK_TYPE_DCE:
        *rsmi_clock_type =  RSMI_CLK_TYPE_DCEF;
        break;
    case AGA_GPU_CLOCK_TYPE_SOC:
        *rsmi_clock_type =  RSMI_CLK_TYPE_SOC;
        break;
    case AGA_GPU_CLOCK_TYPE_MEMORY:
        *rsmi_clock_type =  RSMI_CLK_TYPE_MEM;
        break;
    case AGA_GPU_CLOCK_TYPE_PCIE:
        *rsmi_clock_type =  RSMI_CLK_TYPE_PCIE;
        break;
    default:
        return SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

/// \brief convert rsmi perf_level to gpu perf_level
/// \param[in] perf_level    rsmi performance level
/// \return    aga performance level
static inline aga_gpu_perf_level_t
smi_to_aga_gpu_perf_level (rsmi_dev_perf_level_t perf_level)
{
    switch (perf_level) {
    case RSMI_DEV_PERF_LEVEL_AUTO:
        return AGA_GPU_PERF_LEVEL_AUTO;
    case RSMI_DEV_PERF_LEVEL_LOW:
        return AGA_GPU_PERF_LEVEL_LOW;
    case RSMI_DEV_PERF_LEVEL_HIGH:
        return AGA_GPU_PERF_LEVEL_HIGH;
    case RSMI_DEV_PERF_LEVEL_DETERMINISM:
        return AGA_GPU_PERF_LEVEL_DETERMINISTIC;
    case RSMI_DEV_PERF_LEVEL_STABLE_MIN_SCLK:
        return AGA_GPU_PERF_LEVEL_STABLE_WITH_SCLK;
    case RSMI_DEV_PERF_LEVEL_STABLE_MIN_MCLK:
        return AGA_GPU_PERF_LEVEL_STABLE_WITH_MCLK;
    case RSMI_DEV_PERF_LEVEL_MANUAL:
        return AGA_GPU_PERF_LEVEL_MANUAL;
    default:
        break;
    }
    return AGA_GPU_PERF_LEVEL_NONE;
}

/// \brief convert rsmi xgmi error status to gpu xgmi error
/// \param[in] xe    smi xgmi error status
/// \return     aga xgmi performance level
static inline aga_gpu_xgmi_status_t
smi_to_aga_gpu_xgmi_error (rsmi_xgmi_status_t xe)
{
    switch (xe) {
    case RSMI_XGMI_STATUS_NO_ERRORS:
        return AGA_GPU_XGMI_STATUS_NO_ERROR;
    case RSMI_XGMI_STATUS_ERROR:
        return AGA_GPU_XGMI_STATUS_ONE_ERROR;
    case RSMI_XGMI_STATUS_MULTIPLE_ERRORS:
        return AGA_GPU_XGMI_STATUS_MULTIPLE_ERROR;
    default:
        break;
    }
    return AGA_GPU_XGMI_STATUS_NONE;
}

/// \brief convert gpu perf_level to rsmi perf_level
/// \param[in] perf_level    aga performance level
/// \return    rsmsi performance level
static inline rsmi_dev_perf_level_t
aga_to_smi_gpu_perf_level (aga_gpu_perf_level_t perf_level)
{
    switch (perf_level) {
    case AGA_GPU_PERF_LEVEL_AUTO:
        return RSMI_DEV_PERF_LEVEL_AUTO;
    case AGA_GPU_PERF_LEVEL_LOW:
        return RSMI_DEV_PERF_LEVEL_LOW;
    case AGA_GPU_PERF_LEVEL_HIGH:
        return RSMI_DEV_PERF_LEVEL_HIGH;
    case AGA_GPU_PERF_LEVEL_DETERMINISTIC:
        return RSMI_DEV_PERF_LEVEL_DETERMINISM;
    case AGA_GPU_PERF_LEVEL_STABLE_WITH_SCLK:
        return RSMI_DEV_PERF_LEVEL_STABLE_MIN_SCLK;
    case AGA_GPU_PERF_LEVEL_STABLE_WITH_MCLK:
        return RSMI_DEV_PERF_LEVEL_STABLE_MIN_MCLK;
    case AGA_GPU_PERF_LEVEL_MANUAL:
        return RSMI_DEV_PERF_LEVEL_MANUAL;
    default:
        break;
    }
    return RSMI_DEV_PERF_LEVEL_UNKNOWN;
}

/// \brief    convert rsmi event id to aga event id
/// \param[in] rsmi_event rocm-smi event id
/// \return aga event id
static inline aga_event_id_t
aga_event_id_from_smi_event_id (rsmi_evt_notification_type_t rsmi_event)
{
    switch (rsmi_event) {
    case RSMI_EVT_NOTIF_VMFAULT:
        return AGA_EVENT_ID_VM_PAGE_FAULT;
    case RSMI_EVT_NOTIF_THERMAL_THROTTLE:
        return AGA_EVENT_ID_THERMAL_THROTTLE;
    case RSMI_EVT_NOTIF_GPU_PRE_RESET:
        return AGA_EVENT_ID_GPU_PRE_RESET;
    case RSMI_EVT_NOTIF_GPU_POST_RESET:
        return AGA_EVENT_ID_GPU_POST_RESET;
    default:
        break;
    }
    return AGA_EVENT_ID_NONE;
}

/// \brief      convert aga event id to rsmi event id
/// \param[in]  event    aga event id
/// \param[out] rsmi_event    rocm-smi event id
/// \return SDK_RET_OK or error status in case of failure
static inline sdk_ret_t
aga_event_id_to_smi_event_id (aga_event_id_t event,
                              rsmi_evt_notification_type_t *rsmi_event)
{
    switch (event) {
    case AGA_EVENT_ID_VM_PAGE_FAULT:
        *rsmi_event = RSMI_EVT_NOTIF_VMFAULT;
        break;
    case AGA_EVENT_ID_THERMAL_THROTTLE:
        *rsmi_event = RSMI_EVT_NOTIF_THERMAL_THROTTLE;
        break;
    case AGA_EVENT_ID_GPU_PRE_RESET:
        *rsmi_event = RSMI_EVT_NOTIF_GPU_PRE_RESET;
        break;
    case AGA_EVENT_ID_GPU_POST_RESET:
        *rsmi_event = RSMI_EVT_NOTIF_GPU_POST_RESET;
        break;
    default:
        return SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

/// \brief     convert rsmi return status to sdk return status
/// \param[in] rsmi_ret rsmi return status
/// \return    sdk return status
static inline sdk_ret_t
rsmi_ret_to_sdk_ret (rsmi_status_t rsmi_ret)
{
    switch (rsmi_ret) {
    case RSMI_STATUS_SUCCESS:
        return SDK_RET_OK;
    case RSMI_STATUS_INVALID_ARGS:
        return SDK_RET_INVALID_ARG;
    case RSMI_STATUS_NOT_SUPPORTED:
        return SDK_RET_OP_NOT_SUPPORTED;
    case RSMI_STATUS_FILE_ERROR:
        return SDK_RET_FILE_ERR;
    case RSMI_STATUS_PERMISSION:
        return SDK_RET_PERMISSION_ERR;
    case RSMI_STATUS_OUT_OF_RESOURCES:
        return SDK_RET_OOM;
    case RSMI_STATUS_INTERNAL_EXCEPTION:
        return SDK_RET_INTERNAL_EXCEPTION_ERR;
    case RSMI_STATUS_INPUT_OUT_OF_BOUNDS:
        return SDK_RET_OOB;
    case RSMI_STATUS_INIT_ERROR:
        return SDK_RET_INIT_ERR;
    case RSMI_STATUS_NOT_YET_IMPLEMENTED:
        return SDK_RET_OP_NOT_SUPPORTED;
    case RSMI_STATUS_NOT_FOUND:
        return SDK_RET_ENTRY_NOT_FOUND;
    case RSMI_STATUS_INSUFFICIENT_SIZE:
        return SDK_RET_NO_RESOURCE;
    case RSMI_STATUS_INTERRUPT:
        return SDK_RET_INTERRUPT;
    case RSMI_STATUS_UNEXPECTED_SIZE:
        return SDK_RET_UNEXPECTED_DATA_SIZE_ERR;
    case RSMI_STATUS_NO_DATA:
        return SDK_RET_NO_DATA_ERR;
    case RSMI_STATUS_UNEXPECTED_DATA:
        return SDK_RET_UNEXPECTED_DATA_ERR;
    case RSMI_STATUS_BUSY:
        return SDK_RET_IN_USE;
    case RSMI_STATUS_REFCOUNT_OVERFLOW:
        return SDK_RET_REFCOUNT_OVERFLOW_ERR;
    case RSMI_STATUS_SETTING_UNAVAILABLE:
        return SDK_RET_SETTING_UNAVAILABLE_ERR;
    case RSMI_STATUS_AMDGPU_RESTART_ERR:
        return SDK_RET_RESTART_ERR;
    default:
        break;
    }
    return SDK_RET_ERR;
}

/// \@}

}    // namespace aga

#endif    // __AGA_API_SMI_UTILS_HPP__
