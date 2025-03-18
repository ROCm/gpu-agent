
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
/// This module defines protobuf conversion APIs for events
///
//----------------------------------------------------------------------------

#ifndef __AGA_SVC_EVENTS_TO_PROTO_HPP__
#define __AGA_SVC_EVENTS_TO_PROTO_HPP__

#include "nic/gpuagent/svc/events.hpp"
#include "nic/gpuagent/api/include/base.hpp"
#include "nic/gpuagent/api/include/aga_event.hpp"

sdk_ret_t
aga_event_api_spec_to_proto (amdgpu::Event *proto_event,
                             const aga_event_t *event)
{
    switch (event->id) {
    case AGA_EVENT_ID_VM_PAGE_FAULT:
        proto_event->set_id(amdgpu::EVENT_ID_VM_PAGE_FAULT);
        proto_event->set_category(amdgpu::EVENT_CATEGORY_NONE);
        proto_event->set_severity(amdgpu::EVENT_SEVERITY_DEBUG);
        break;
    case AGA_EVENT_ID_THERMAL_THROTTLE:
        proto_event->set_id(amdgpu::EVENT_ID_THERMAL_THROTTLE);
        proto_event->set_category(amdgpu::EVENT_CATEGORY_NONE);
        proto_event->set_severity(amdgpu::EVENT_SEVERITY_INFO);
        break;
    case AGA_EVENT_ID_GPU_PRE_RESET:
        proto_event->set_id(amdgpu::EVENT_ID_GPU_PRE_RESET);
        proto_event->set_category(amdgpu::EVENT_CATEGORY_NONE);
        proto_event->set_severity(amdgpu::EVENT_SEVERITY_INFO);
        break;
    case AGA_EVENT_ID_GPU_POST_RESET:
        proto_event->set_id(amdgpu::EVENT_ID_GPU_POST_RESET);
        proto_event->set_category(amdgpu::EVENT_CATEGORY_NONE);
        proto_event->set_severity(amdgpu::EVENT_SEVERITY_INFO);
        break;
    case AGA_EVENT_ID_RING_HANG:
        proto_event->set_id(amdgpu::EVENT_ID_RING_HANG);
        proto_event->set_category(amdgpu::EVENT_CATEGORY_NONE);
        proto_event->set_severity(amdgpu::EVENT_SEVERITY_WARN);
        break;
    default:
        AGA_TRACE_ERR("Ignoring unknown GPU event {}", event->id);
        return SDK_RET_INVALID_ARG;
    }
    auto time_stamp = proto_event->mutable_time();
    time_stamp->set_seconds(event->timestamp.tv_sec);
    time_stamp->set_nanos(event->timestamp.tv_nsec);
    proto_event->set_gpu(event->gpu.id, OBJ_MAX_KEY_LEN);
    proto_event->set_description(event->message);
    return SDK_RET_OK;
}

#endif    // __AGA_SVC_EVENTS_TO_SPEC_HPP__
