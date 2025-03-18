


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


//
//----------------------------------------------------------------------------
///
/// \file
/// APIs for events object at the svc layer
///
//----------------------------------------------------------------------------

#ifndef __AGA_SVC_EVENTS_SVC_HPP__
#define __AGA_SVC_EVENTS_SVC_HPP__

#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/svc/events.hpp"
#include "nic/gpuagent/svc/utils.hpp"
#include "nic/gpuagent/svc/events_to_spec.hpp"
#include "nic/gpuagent/svc/events_to_proto.hpp"
#include "nic/gpuagent/api/include/aga_event.hpp"
#include "nic/gpuagent/api/internal/aga_event.hpp"


/// \brief    callback context passed back for each event
typedef struct aga_event_read_cb_ctxt_s {
    // incoming gRPC request
    const EventRequest *req;
    // response msg to populate event information
    EventResponse *rsp;
    // filtering criteria, if any
    aga_event_filter_ctxt_t filter;
} aga_event_read_cb_ctxt_t;

static inline aga_event_severity_t
event_id_to_severity (aga_event_id_t event_id)
{
    switch (event_id) {
    case AGA_EVENT_ID_VM_PAGE_FAULT:
        return AGA_EVENT_SEVERITY_DEBUG;
    case AGA_EVENT_ID_THERMAL_THROTTLE:
        return AGA_EVENT_SEVERITY_INFO;
    case AGA_EVENT_ID_GPU_PRE_RESET:
        return AGA_EVENT_SEVERITY_INFO;
    case AGA_EVENT_ID_GPU_POST_RESET:
        return AGA_EVENT_SEVERITY_INFO;
    case AGA_EVENT_ID_RING_HANG:
        return AGA_EVENT_SEVERITY_WARN;
    default:
        break;
    }
    return AGA_EVENT_SEVERITY_NONE;
}

static inline aga_event_category_t
event_id_to_category (aga_event_id_t event_id)
{
    switch (event_id) {
    case AGA_EVENT_ID_VM_PAGE_FAULT:
        return AGA_EVENT_CATEGORY_NONE;
    case AGA_EVENT_ID_THERMAL_THROTTLE:
        return AGA_EVENT_CATEGORY_NONE;
    case AGA_EVENT_ID_GPU_PRE_RESET:
        return AGA_EVENT_CATEGORY_NONE;
    case AGA_EVENT_ID_GPU_POST_RESET:
        return AGA_EVENT_CATEGORY_NONE;
    case AGA_EVENT_ID_RING_HANG:
        return AGA_EVENT_CATEGORY_NONE;
    default:
        break;
    }
    return AGA_EVENT_CATEGORY_NONE;
}

static inline void
aga_event_read_cb (const aga_event_t *event, void *ctxt)
{
    bool match;
    EventResponse *rsp;
    aga_event_filter_ctxt_t *filter;
    aga_event_read_cb_ctxt_t *cb_ctxt;

    cb_ctxt = (aga_event_read_cb_ctxt_t *)ctxt;
    rsp = cb_ctxt->rsp;
    filter = &cb_ctxt->filter;
    // apply the filter to the event, if applicable
    if (filter->match_type == AGA_EVENT_MATCH_TYPE_EVENT_ID) {
        match = false;
            for (auto i = 0; i < filter->num_event; i++) {
                if (filter->event[i] == event->id) {
                    match = true;
                    break;
                }
            }
        if (match == false) {
            // event id is not a match
            return;
        }
    } else if (filter->match_type == AGA_EVENT_MATCH_TYPE_EVENT_ATTRS) {
        if ((filter->severity != AGA_EVENT_SEVERITY_NONE) &&
                (filter->severity != event_id_to_severity(event->id))) {
            // severity is not a match
            return;
        }
        if ((filter->category != AGA_EVENT_CATEGORY_NONE) &&
                (filter->category != event_id_to_category(event->id))) {
            // category is not a match
            return;
        }
    }
    if (filter->num_gpu) {
        match = false;
        for (auto i = 0; i < filter->num_gpu; i++) {
            if (event->gpu == filter->gpu[i]) {
                match = true;
                break;
            }
        }
        if (match == false) {
            // gpu filter is not a match
            return;
        }
    }
    aga_event_api_spec_to_proto(rsp->add_event(), event);
}

static inline aga_event_match_type_t
aga_event_match_type (const amdgpu::EventFilter& proto_filter)
{
    switch (proto_filter.filter_case()) {
    case amdgpu::EventFilter::kEvents:
        return AGA_EVENT_MATCH_TYPE_EVENT_ID;
    case amdgpu::EventFilter::kMatchAttrs:
        return AGA_EVENT_MATCH_TYPE_EVENT_ATTRS;
    default:
        break;
    }
    return AGA_EVENT_MATCH_TYPE_NONE;
}

static inline sdk_ret_t
aga_svc_event_get (const EventRequest *req, EventResponse *rsp) {
    sdk_ret_t ret;
    aga_event_read_cb_ctxt_t cb_ctxt = {};

    aga_api_trace_verbose("Event", "Get", req);
    // populate the request and response in the callback context
    cb_ctxt.req = req;
    cb_ctxt.rsp = rsp;
    // decode the filter type
    if (req->has_filter()) {
        ret = aga_event_filter_proto_to_api_spec(&cb_ctxt.filter,
                                                 req->filter());
        if (unlikely(ret != SDK_RET_OK)) {
            return ret;
        }
    }
    ret = aga_event_read_all(aga_event_read_cb, &cb_ctxt);
    return ret;
}

sdk_ret_t
aga_event_ntfn_cb (const aga_event_t *event,
                   void *ctxt)
{
    bool rv;
    sdk_ret_t ret;
    amdgpu::Event proto_event;
    aga_event_client_ctxt_t *client_ctxt = (aga_event_client_ctxt_t *)ctxt;

    // convert the event to protobuf format
    ret = aga_event_api_spec_to_proto(&proto_event, event);
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }
    // try to write to the client stream
    rv = ((ServerWriter<Event> *)client_ctxt->stream)->Write(proto_event);
    if (unlikely(rv == false)) {
        AGA_TRACE_ERR("Failed to notify event {} to client {}",
                      event->id, client_ctxt->client.c_str());
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

static inline sdk_ret_t
aga_svc_event_subscribe (ServerContext* context,
                         const EventSubscribeRequest *proto_req,
                         ServerWriter<Event> *stream) {
    sdk_ret_t ret;
    aga_event_id_t event_id;
    aga_event_subscribe_req_t req;

    if (proto_req->has_filter() == false) {
        // empty event subscribe request is not supported
        AGA_TRACE_ERR("Empty event subscribe request is not supported");
        return SDK_RET_INVALID_ARG;
    }
    aga_api_trace_verbose("Event", "Subscribe", proto_req);
    AGA_TRACE_VERBOSE("Rcvd event subscribe request from client {}, "
                      "stream {}", context->peer().c_str(), (void *)stream);

    if (proto_req->filter().has_events()) {
        req.num_events = proto_req->filter().events().id_size();
        for (auto i = 0; i < proto_req->filter().events().id_size(); i++) {
            // convert the event id
            event_id =
                aga_event_id_proto_to_api_spec(
                    proto_req->filter().events().id(i));
            if (unlikely(event_id == AGA_EVENT_ID_NONE)) {
                AGA_TRACE_ERR("Failed to subscribe client {} to unknown "
                              "event {}", context->peer().c_str(),
                              proto_req->filter().events().id(i));
                goto end;
            }
            req.events[i] = event_id;
            AGA_TRACE_VERBOSE("Client {}, stream {} subscribed for event {}",
                              context->peer().c_str(), (void *)stream,
                              event_id);
        }
    }
    req.num_gpu = proto_req->filter().gpu_size();
    for (auto i = 0; i < req.num_gpu; i++) {
        ret = aga_obj_key_proto_to_api_spec(&req.gpu[i],
                                            proto_req->filter().gpu(i));
        if (unlikely(ret != SDK_RET_OK)) {
            AGA_TRACE_ERR("Failed to subscribe to events, invalid GPU id");
            return SDK_RET_INVALID_ARG;
        }
    }
    strncpy(req.client, context->peer().c_str(), AGA_MAX_CLIENT_STR);
    req.stream = stream;
    req.notify_cb = aga_event_ntfn_cb;
    aga_event_subscribe(&req);

end:
    return SDK_RET_OK;
}

static inline sdk_ret_t
aga_svc_event_gen (const EventGenRequest *proto_req) {
    sdk_ret_t ret;
    aga_event_id_t event_id;
    aga_event_gen_req_t req = {};

    req.num_event = proto_req->id_size();
    for (auto i = 0; i < req.num_event; i++) {
        event_id = aga_event_id_proto_to_api_spec(proto_req->id(i));
        if (unlikely(event_id == AGA_EVENT_ID_NONE)) {
            AGA_TRACE_ERR("Failed to generate events, unknown event id {}",
                          proto_req->id(i));
            return SDK_RET_INVALID_ARG;
        }
        req.event[i] = event_id;
    }
    req.num_gpu = proto_req->gpu_size();
    for (auto i = 0; i < req.num_gpu; i++) {
        ret = aga_obj_key_proto_to_api_spec(&req.gpu[i], proto_req->gpu(i));
        if (unlikely(ret != SDK_RET_OK)) {
            AGA_TRACE_ERR("Failed to generate events, invalid GPU id");
            return SDK_RET_INVALID_ARG;
        }
    }
    ret = aga_event_gen(&req);
    return ret;
}

#endif    // __AGA_SVC_EVENTS_SVC_HPP__

