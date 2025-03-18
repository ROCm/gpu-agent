
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

#ifndef __AGA_SVC_EVENTS_TO_SPEC_HPP__
#define __AGA_SVC_EVENTS_TO_SPEC_HPP__

#include "nic/gpuagent/svc/events.hpp"
#include "nic/gpuagent/api/include/aga_event.hpp"

static inline aga_event_id_t
aga_event_id_proto_to_api_spec (amdgpu::EventId id)
{
    switch (id) {
    case amdgpu::EVENT_ID_VM_PAGE_FAULT:
        return AGA_EVENT_ID_VM_PAGE_FAULT;
    case amdgpu::EVENT_ID_THERMAL_THROTTLE:
        return AGA_EVENT_ID_THERMAL_THROTTLE;
    case amdgpu::EVENT_ID_GPU_PRE_RESET:
        return AGA_EVENT_ID_GPU_PRE_RESET;
    case amdgpu::EVENT_ID_GPU_POST_RESET:
        return AGA_EVENT_ID_GPU_POST_RESET;
    case amdgpu::EVENT_ID_RING_HANG:
        return AGA_EVENT_ID_RING_HANG;
    default:
        break;
    }
    return AGA_EVENT_ID_NONE;
}

static inline aga_event_severity_t
aga_event_severity_proto_to_api_spec (amdgpu::EventSeverity severity)
{
    switch (severity) {
    case amdgpu::EVENT_SEVERITY_DEBUG:
        return AGA_EVENT_SEVERITY_DEBUG;
    case amdgpu::EVENT_SEVERITY_INFO:
        return AGA_EVENT_SEVERITY_INFO;
    case amdgpu::EVENT_SEVERITY_WARN:
        return AGA_EVENT_SEVERITY_WARN;
    case amdgpu::EVENT_SEVERITY_CRITICAL:
        return AGA_EVENT_SEVERITY_CRITICAL;
    default:
        break;
    }
    return AGA_EVENT_SEVERITY_NONE;
}

static inline aga_event_category_t
aga_event_category_proto_to_api_spec (amdgpu::EventCategory category)
{
    switch (category) {
    case amdgpu::EVENT_CATEGORY_OTHER:
        return AGA_EVENT_CATEGORY_OTHER;
    default:
        break;
    }
    return AGA_EVENT_CATEGORY_NONE;
}

static inline sdk_ret_t
aga_event_filter_proto_to_api_spec (aga_event_filter_ctxt_t *api_spec,
                                    const amdgpu::EventFilter& proto_spec)
{
    sdk_ret_t ret;
    aga_event_id_t event_id;

    switch (proto_spec.filter_case()) {
    case amdgpu::EventFilter::kEvents:
    {
        api_spec->match_type = AGA_EVENT_MATCH_TYPE_EVENT_ID;
        api_spec->num_event = proto_spec.events().id_size();
        for (auto i = 0; i < api_spec->num_event; i++) {
            event_id =
                aga_event_id_proto_to_api_spec(proto_spec.events().id(i));
            if (unlikely(event_id == AGA_EVENT_ID_NONE)) {
                AGA_TRACE_ERR("Failed to get events, unknown event id {}",
                              proto_spec.events().id(i));
                return SDK_RET_INVALID_ARG;
            }
            api_spec->event[i] = event_id;
        }
    }
        break;
    case amdgpu::EventFilter::kMatchAttrs:
    {
        auto match_attrs = proto_spec.matchattrs();

        api_spec->match_type = AGA_EVENT_MATCH_TYPE_EVENT_ATTRS;
        api_spec->severity =
            aga_event_severity_proto_to_api_spec(match_attrs.severity());
        api_spec->category =
            aga_event_category_proto_to_api_spec(match_attrs.category());
    }
        break;
    default:
        api_spec->match_type = AGA_EVENT_MATCH_TYPE_NONE;
        break;
    }
    api_spec->num_gpu = proto_spec.gpu_size();
    for (auto i = 0; i < api_spec->num_gpu; i++) {
        ret = aga_obj_key_proto_to_api_spec(&api_spec->gpu[i],
                                            proto_spec.gpu(i));
        if (unlikely(ret != SDK_RET_OK)) {
            AGA_TRACE_ERR("Failed to get events, invalid GPU id");
            return SDK_RET_INVALID_ARG;
        }
    }
    return SDK_RET_OK;
}

#endif    // __AGA_SVC_EVENTS_TO_SPEC_HPP__
