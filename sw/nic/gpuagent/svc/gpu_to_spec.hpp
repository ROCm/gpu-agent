
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
/// This module defines gpu protobuf to spec conversion APIs
///
//----------------------------------------------------------------------------

#ifndef __AGA_SVC_GPU_TO_SPEC_HPP__
#define __AGA_SVC_GPU_TO_SPEC_HPP__

#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/svc/events.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"
#include "nic/gpuagent/api/include/aga_task.hpp"

static inline aga_gpu_admin_state_t
aga_gpu_admin_state_to_spec (amdgpu::GPUAdminState admin_state)
{
    switch (admin_state) {
    case amdgpu::GPU_ADMIN_STATE_UP:
        return AGA_GPU_ADMIN_STATE_UP;
    case amdgpu::GPU_ADMIN_STATE_DOWN:
        return AGA_GPU_ADMIN_STATE_DOWN;
    case amdgpu::GPU_ADMIN_STATE_NONE:
    default:
        break;
    }
    return AGA_GPU_ADMIN_STATE_NONE;
}

static inline aga_gpu_clock_type_t
aga_gpu_clock_type_to_spec (amdgpu::GPUClockType clock_type)
{
    switch (clock_type) {
    case amdgpu::GPU_CLOCK_TYPE_FABRIC:
        return AGA_GPU_CLOCK_TYPE_FABRIC;
    case amdgpu::GPU_CLOCK_TYPE_MEMORY:
        return AGA_GPU_CLOCK_TYPE_MEMORY;
    case amdgpu::GPU_CLOCK_TYPE_SYSTEM:
        return AGA_GPU_CLOCK_TYPE_SYSTEM;
    case amdgpu::GPU_CLOCK_TYPE_SOC:
        return AGA_GPU_CLOCK_TYPE_SOC;
    case amdgpu::GPU_CLOCK_TYPE_DCE:
        return AGA_GPU_CLOCK_TYPE_DCE;
    case amdgpu::GPU_CLOCK_TYPE_PCIE:
        return AGA_GPU_CLOCK_TYPE_PCIE;
    case amdgpu::GPU_CLOCK_TYPE_VIDEO:
        return AGA_GPU_CLOCK_TYPE_VIDEO;
    case amdgpu::GPU_CLOCK_TYPE_DATA:
        return AGA_GPU_CLOCK_TYPE_DATA;
    default:
        return AGA_GPU_CLOCK_TYPE_NONE;
    }
}

static inline sdk_ret_t
aga_gpu_clock_spec_to_spec (aga_gpu_spec_t *api_spec, const GPUSpec& proto_spec)
{
    bool gfx_cfg, mem_cfg, video_cfg, data_cfg;

    gfx_cfg = mem_cfg = video_cfg = data_cfg = false;

    // get number of clock frequencies
    api_spec->num_clock_freqs = proto_spec.clockfrequency_size();
    if (api_spec->num_clock_freqs > AGA_GPU_NUM_CFG_CLOCK_TYPES) {
        AGA_TRACE_ERR("GPU {} number of clock frequency ranges specified, {}, "
                      "is more than {} supported", api_spec->key.str(),
                      api_spec->num_clock_freqs, AGA_GPU_NUM_CFG_CLOCK_TYPES);
        return sdk_ret_t(SDK_RET_INVALID_ARG,
                         ERR_CODE_GPU_NUM_CLOCK_FREQ_RANGE_EXCEEDED);
    }
    for (uint32_t i = 0; i < api_spec->num_clock_freqs; i++) {
        auto clock_spec = &api_spec->clock_freq[i];
        auto proto_clock_spec = proto_spec.clockfrequency(i);

        clock_spec->clock_type =
            aga_gpu_clock_type_to_spec(proto_clock_spec.clocktype());
        switch (clock_spec->clock_type) {
        case AGA_GPU_CLOCK_TYPE_SYSTEM:
            if (gfx_cfg) {
                AGA_TRACE_ERR("GPU {} duplicate clock frequency range "
                              "specified for clock type {}",
                              api_spec->key.str(), AGA_GPU_CLOCK_TYPE_SYSTEM);
                return sdk_ret_t(SDK_RET_INVALID_ARG,
                                 ERR_CODE_GPU_DUPLICATE_CLOCK_FREQ_RANGE);
            }
            gfx_cfg = true;
            break;
        case AGA_GPU_CLOCK_TYPE_MEMORY:
            if (mem_cfg) {
                AGA_TRACE_ERR("GPU {} duplicate clock frequency range "
                              "specified for clock type {}",
                              api_spec->key.str(), AGA_GPU_CLOCK_TYPE_MEMORY);
                return sdk_ret_t(SDK_RET_INVALID_ARG,
                                 ERR_CODE_GPU_DUPLICATE_CLOCK_FREQ_RANGE);
            }
            mem_cfg = true;
            break;
        case AGA_GPU_CLOCK_TYPE_VIDEO:
            if (video_cfg) {
                AGA_TRACE_ERR("GPU {} duplicate clock frequency range "
                              "specified for clock type {}",
                              api_spec->key.str(), AGA_GPU_CLOCK_TYPE_VIDEO);
                return sdk_ret_t(SDK_RET_INVALID_ARG,
                                 ERR_CODE_GPU_DUPLICATE_CLOCK_FREQ_RANGE);
            }
            video_cfg = true;
            break;
        case AGA_GPU_CLOCK_TYPE_DATA:
            if (data_cfg) {
                AGA_TRACE_ERR("GPU {} duplicate clock frequency range "
                              "specified for clock type {}",
                              api_spec->key.str(), AGA_GPU_CLOCK_TYPE_DATA);
                return sdk_ret_t(SDK_RET_INVALID_ARG,
                                 ERR_CODE_GPU_DUPLICATE_CLOCK_FREQ_RANGE);
            }
            data_cfg = true;
            break;
        default:
            AGA_TRACE_ERR("GPU {} clock frequency range update not supported "
                          "for type {}", api_spec->key.str(),
                          clock_spec->clock_type);
            return sdk_ret_t(SDK_RET_INVALID_ARG,
                       ERR_CODE_GPU_CLOCK_TYPE_FREQ_RANGE_UPDATE_NOT_SUPPORTED);
            break;
        }
        clock_spec->lo = proto_clock_spec.lowfrequency();
        clock_spec->hi = proto_clock_spec.highfrequency();
        if (clock_spec->lo > clock_spec->hi) {
            AGA_TRACE_ERR("GPU {} invalid GPU clock frequency range specified "
                          "{}-{}", api_spec->key.str(), clock_spec->lo,
                          clock_spec->hi);
            return sdk_ret_t(SDK_RET_INVALID_ARG,
                             ERR_CODE_GPU_CLOCK_FREQ_RANGE_INVALID);
        }
    }
    return SDK_RET_OK;
}

static inline aga_gpu_perf_level_t
aga_gpu_power_overdrive_to_spec (amdgpu::GPUPerformanceLevel perf_level)
{
    switch (perf_level) {
    case amdgpu::GPU_PERF_LEVEL_AUTO:
        return AGA_GPU_PERF_LEVEL_AUTO;
    case amdgpu::GPU_PERF_LEVEL_LOW:
        return AGA_GPU_PERF_LEVEL_LOW;
    case amdgpu::GPU_PERF_LEVEL_HIGH:
        return AGA_GPU_PERF_LEVEL_HIGH;
    case amdgpu::GPU_PERF_LEVEL_DETERMINISTIC:
        return AGA_GPU_PERF_LEVEL_DETERMINISTIC;
    case amdgpu::GPU_PERF_LEVEL_STABLE_MIN_MCLK:
        return AGA_GPU_PERF_LEVEL_STABLE_WITH_MCLK;
    case amdgpu::GPU_PERF_LEVEL_STABLE_MIN_SCLK:
        return AGA_GPU_PERF_LEVEL_STABLE_WITH_SCLK;
    case amdgpu::GPU_PERF_LEVEL_MANUAL:
        return AGA_GPU_PERF_LEVEL_MANUAL;
    case amdgpu::GPU_PERF_LEVEL_NONE:
    default:
        break;
    }
    return AGA_GPU_PERF_LEVEL_NONE;
}

static inline sdk_ret_t
aga_gpu_proto_to_api_spec (aga_gpu_spec_t *api_spec, const GPUSpec& proto_spec)
{
    sdk_ret_t ret;

    aga_obj_key_proto_to_api_spec(&api_spec->key, proto_spec.id());
    api_spec->admin_state =
        aga_gpu_admin_state_to_spec(proto_spec.adminstate());
    api_spec->overdrive_level = proto_spec.overdrivelevel();
    if ((api_spec->overdrive_level < AGA_GPU_MIN_OVERDRIVE_LEVEL) ||
        (api_spec->overdrive_level > AGA_GPU_MAX_OVERDRIVE_LEVEL)) {
        AGA_TRACE_ERR("GPU {} overdrive level {} out of allowed range {}-{}",
                      api_spec->key.str(), api_spec->overdrive_level,
                      AGA_GPU_MIN_OVERDRIVE_LEVEL, AGA_GPU_MAX_OVERDRIVE_LEVEL);
        return sdk_ret_t(SDK_RET_INVALID_ARG,
                         ERR_CODE_GPU_OVERDRIVE_OUT_OF_RANGE);
    }
    api_spec->gpu_power_cap = proto_spec.gpupowercap();
    api_spec->fan_speed = proto_spec.fanspeed();
    api_spec->perf_level =
        aga_gpu_power_overdrive_to_spec(proto_spec.performancelevel());
    ret = aga_gpu_clock_spec_to_spec(api_spec, proto_spec);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    // TODO: fill RAS spec
    return SDK_RET_OK;
}

static inline sdk_ret_t
aga_gpu_reset_request_proto_to_api_spec (aga_gpu_reset_task_spec_t *api_spec,
                                         const GPUResetRequest *proto_req)
{
    if (proto_req->id_size() == 0) {
        AGA_TRACE_ERR("Atleast one GPU must be specified in the reset request");
        return SDK_RET_INVALID_ARG;
    }
    memset(api_spec, 0, sizeof(*api_spec));
    api_spec->num_gpu = proto_req->id_size();
    for (int i = 0; i < api_spec->num_gpu; i++) {
        aga_obj_key_proto_to_api_spec(&api_spec->gpu[i], proto_req->id(i));
    }
    switch (proto_req->reset_case()) {
    case amdgpu::GPUResetRequest::kResetClocks:
        api_spec->reset_type = AGA_GPU_RESET_TYPE_CLOCK;
        break;
    case amdgpu::GPUResetRequest::kResetFans:
        api_spec->reset_type = AGA_GPU_RESET_TYPE_FAN;
        break;
    case amdgpu::GPUResetRequest::kResetPowerProfile:
        api_spec->reset_type = AGA_GPU_RESET_TYPE_POWER_PROFILE;
        break;
    case amdgpu::GPUResetRequest::kResetPowerOverDrive:
        api_spec->reset_type = AGA_GPU_RESET_TYPE_POWER_OVERDRIVE;
        break;
    case amdgpu::GPUResetRequest::kResetXGMIError:
        api_spec->reset_type = AGA_GPU_RESET_TYPE_XGMI_ERROR;
        break;
    case amdgpu::GPUResetRequest::kResetPerfDeterminism:
        api_spec->reset_type = AGA_GPU_RESET_TYPE_PERF_DETERMINISM;
        break;
    case amdgpu::GPUResetRequest::kResetComputePartition:
        api_spec->reset_type = AGA_GPU_RESET_TYPE_COMPUTE_PARTITION;
        break;
    case amdgpu::GPUResetRequest::kResetNPSMode:
        api_spec->reset_type = AGA_GPU_RESET_TYPE_NPS_MODE;
        break;
    default:
        return SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

#endif    // __AGA_SVC_GPU_TO_SPEC_HPP__
