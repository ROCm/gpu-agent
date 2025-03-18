
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
/// GPU event internal definitions
///
//----------------------------------------------------------------------------

#ifndef __API_INTERNAL_AGA_EVENT_HPP__
#define __API_INTERNAL_AGA_EVENT_HPP__

#include <vector>
#include "nic/gpuagent/api/include/aga_event.hpp"

using std::vector;

/// \brief  event client context sent from frontend to backend
typedef struct aga_event_client_ctxt_s {
    /// client IP address and port
    std::string client;
    /// client_inactive is set to true by the backend thread when
    /// gRPC client is no longer active
    bool client_inactive;
    /// condition variable the front end gRPC thread is going to wait on
    pthread_cond_t cond;
    /// mutex lock to keep the gRPC thread alive
    pthread_mutex_t lock;
    /// opaque context sent from gRPC thread to backend
    /// NOTE:
    /// gRPC response stream to periodically publish events to
    void *stream;
    /// callback API to notify event to the client stream
    aga_event_cb_t notify_cb;
} aga_event_client_ctxt_t;

/// \brief event subscribe info
typedef struct aga_event_subscribe_args_s {
    /// gRPC client context
    aga_event_client_ctxt_t *client_ctxt;
    /// list of events of interest
    vector<aga_event_id_t> events;
    /// GPU id list;
    vector<uint8_t> gpu_ids;
} aga_event_subscribe_args_t;

typedef struct aga_event_gen_args_s {
    /// list of events of interest
    vector<aga_event_id_t> events;
    /// GPU list;
    vector<uint8_t> gpu_ids;
} aga_event_gen_args_t;

#endif    // __API_INTERNAL_AGA_EVENT_HPP__
