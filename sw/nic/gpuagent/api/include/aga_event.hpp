
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
/// GPU event definitions
///
//----------------------------------------------------------------------------

#ifndef __API_INCLUDE_AGA_EVENT_HPP__
#define __API_INCLUDE_AGA_EVENT_HPP__

#include <pthread.h>
#include <string>
#include "nic/sdk/include/sdk/timestamp.hpp"
#include "nic/gpuagent/api/include/base.hpp"

/// max. string length to describe an event
#define AGA_MAX_EVENT_STR        128
#define AGA_MAX_CLIENT_STR       128

/// \brief    event categories
typedef enum aga_event_category_e {
    AGA_EVENT_CATEGORY_NONE  = 0,
    /// XGMI related events
    AGA_EVENT_CATEGORY_XGMI  = 1,
    /// catch-all event category
    AGA_EVENT_CATEGORY_OTHER = 2,
} aga_event_category_t;

/// \brief    event severity
typedef enum aga_event_severity_e {
    AGA_EVENT_SEVERITY_NONE     = 0,
    AGA_EVENT_SEVERITY_DEBUG    = 1, // only for debugging purposes
    AGA_EVENT_SEVERITY_INFO     = 2, // no action needed
    AGA_EVENT_SEVERITY_WARN     = 3, // no immediate action needed
    AGA_EVENT_SEVERITY_CRITICAL = 4, // immediate attention required
} aga_event_severity_t;

// event match criteria
typedef enum aga_event_mach_type_e {
    AGA_EVENT_MATCH_TYPE_NONE        = 0,
    // event match criteria is event id
    AGA_EVENT_MATCH_TYPE_EVENT_ID    = 1,
    // event match criteria is event attributes
    AGA_EVENT_MATCH_TYPE_EVENT_ATTRS = 2,
} aga_event_match_type_t;

/// \brief    event identifiers
typedef enum aga_event_id_e {
    AGA_EVENT_ID_NONE             = 0,
    /// VM page fault event
    AGA_EVENT_ID_VM_PAGE_FAULT    = 1,
    /// clock frequency has decreased due to temperature rise
    AGA_EVENT_ID_THERMAL_THROTTLE = 2,
    /// GPU reset about to happen
    AGA_EVENT_ID_GPU_PRE_RESET    = 3,
    /// PU reset happened
    AGA_EVENT_ID_GPU_POST_RESET   = 4,
    /// GPU command ring hang
    AGA_EVENT_ID_RING_HANG        = 5,
    AGA_EVENT_ID_MAX              = AGA_EVENT_ID_RING_HANG,
} aga_event_id_t;

/// \brief    event get/subscribe filter
typedef struct aga_event_filter_ctxt_s {
    /// match type
    aga_event_match_type_t match_type;
    /// number of events
    uint8_t num_event;
    /// events list
    aga_event_id_t event[AGA_EVENT_ID_MAX];
    /// event severity
    aga_event_severity_t severity;
    /// event category
    aga_event_category_t category;
    /// number of GPUs
    uint8_t num_gpu;
    /// GPU list;
    aga_obj_key_t gpu[AGA_MAX_GPU];
} aga_event_filter_ctxt_t;

/// \brief    event record
typedef struct aga_event_s {
    /// unique event identifier
    aga_event_id_t id;
    /// event category
    aga_event_category_t category;
    /// event severity
    aga_event_severity_t severity;
    /// event timestamp indicating when the event happened
    timespec_t timestamp;
    /// uuid of the GPU device
    aga_obj_key_t gpu;
    /// description of the event
    char message[AGA_MAX_EVENT_STR + 1];
} aga_event_t;

/// \brief type of the callback function invoked to notify events
/// \param[in] event        event information
/// \param[in] client_ctxt  context cached during event subscription and
///                         passed back to the callback function
/// \return    SDK_RET_OK or status code in case of error
typedef sdk_ret_t (*aga_event_cb_t)(const aga_event_t *event,
                                    void *client_ctxt);

/// \brief event subscribe request from a gRPC client
typedef struct aga_event_subscribe_req_s {
    /// number of events
    uint8_t num_events;
    /// events list
    aga_event_id_t events[AGA_EVENT_ID_MAX];
    /// number of GPUs
    uint8_t num_gpu;
    /// GPU list;
    aga_obj_key_t gpu[AGA_MAX_GPU];
    /// gRPC client IP address and port
    char client[AGA_MAX_CLIENT_STR + 1];
    /// opaque context sent from gRPC thread to backend
    /// NOTE:
    /// gRPC response stream to periodically publish events to
    void *stream;
    /// callback API to notify event the client stream
    aga_event_cb_t notify_cb;
} aga_event_subscribe_req_t;

/// \brief event generation request
typedef struct aga_event_gen_req_s {
    /// number of events
    uint8_t num_event;
    /// events list
    aga_event_id_t event[AGA_EVENT_ID_MAX];
    /// number of GPUs
    uint8_t num_gpu;
    /// GPU list;
    aga_obj_key_t gpu[AGA_MAX_GPU];
} aga_event_gen_req_t;

typedef void (*aga_event_read_cb_t)(const aga_event_t *event, void *ctxt);

/// \brief    read all event information
/// \param[in]  cb      callback function
/// \param[in]  ctxt    opaque context passed to cb
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_event_read_all(_In_ aga_event_read_cb_t gpu_read_cb,
                             _In_ void *ctxt);

/// \brief    event subscribe
/// \param[in] req    pointer to event subscribe request
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_event_subscribe(_In_ aga_event_subscribe_req_t *req);

/// \brief    generate events
/// \param[in] req    pointer to event generate request
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_event_gen(_In_ aga_event_gen_req_t *req);

#endif    // __API_INCLUDE_AGA_EVENT_HPP__
