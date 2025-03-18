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
/// this file implements event related APIs
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/core/aga_core.hpp"
#include "nic/gpuagent/core/ipc_msg.hpp"
#include "nic/gpuagent/api/include/aga_event.hpp"
#include "nic/gpuagent/api/internal/aga_event.hpp"
#include "nic/gpuagent/api/gpu.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/smi/smi_api.hpp"

sdk_ret_t
aga_event_read_all (aga_event_read_cb_t cb, void *ctxt)
{
    return aga::smi_event_read_all(cb, ctxt);
}

static void
aga_event_subscribe_rsp_cb (sdk::ipc::ipc_msg_ptr msg, const void *status)
{
    if (msg) {
        *(sdk_ret_t *)status = *(sdk_ret_t *)msg->data();
    } else {
        *(sdk_ret_t *)status = SDK_RET_ERR;
    }
}

sdk_ret_t
aga_event_subscribe (aga_event_subscribe_req_t *req)
{
    sdk_ret_t ret;
    gpu_entry *entry;
    aga_event_subscribe_args_t *args;

    args = new aga_event_subscribe_args_t();
    // allocate the client context
    args->client_ctxt = new aga_event_client_ctxt_t();
    AGA_TRACE_VERBOSE("Event subscribe request client_ctxt {}",
                      (void *)args->client_ctxt);
    // initialize the client context
    args->client_ctxt->client = req->client;
    args->client_ctxt->client_inactive = false;
    args->client_ctxt->cond = PTHREAD_COND_INITIALIZER;
    args->client_ctxt->lock = PTHREAD_MUTEX_INITIALIZER;
    args->client_ctxt->stream = req->stream;
    args->client_ctxt->notify_cb = req->notify_cb;

    for (auto i = 0; i < req->num_gpu; i++) {
        entry = gpu_db()->find(&req->gpu[i]);
        if (unlikely(entry == NULL)) {
            AGA_TRACE_ERR("Failed to subscribe events, GPU {} not found",
                          req->gpu[i].str());
            return SDK_RET_INVALID_ARG;
        }
        args->gpu_ids.push_back(entry->id());
    }
    for (auto i = 0; i < req->num_events; i++) {
        // convert the event id
        args->events.push_back(req->events[i]);
    }
    // send this request to backend service thread
    // NOTE: intention is to send pointer to the backend thread
    sdk::ipc::FIXME_request(aga::AGA_THREAD_ID_EVENT_MONITOR,
                            aga::AGA_IPC_MSG_ID_EVENT_SUBSCRIBE, &args,
                            sizeof(args),
                            aga_event_subscribe_rsp_cb, &ret);
    /// wait until gRPC client closes the connection and becomes inactive
    pthread_mutex_lock(&args->client_ctxt->lock);
    while (args->client_ctxt->client_inactive == false) {
        pthread_cond_wait(&args->client_ctxt->cond, &args->client_ctxt->lock);
    }
    AGA_TRACE_INFO("Rcvd signal from backend thread to quit, releasing "
                   "client {} ctxt {}, event stream {}",
                   args->client_ctxt->client.c_str(),
                   (void *)args->client_ctxt,
                   (void *)args->client_ctxt->stream);
    pthread_mutex_unlock(&args->client_ctxt->lock);
    pthread_mutex_destroy(&args->client_ctxt->lock);

    // free the client context
    delete args->client_ctxt;
    delete args;
    return SDK_RET_OK;
}

static void
aga_event_gen_rsp_cb (sdk::ipc::ipc_msg_ptr msg, const void *status)
{
    if (msg) {
        *(sdk_ret_t *)status = *(sdk_ret_t *)msg->data();
    } else {
        *(sdk_ret_t *)status = SDK_RET_ERR;
    }
}

sdk_ret_t
aga_event_gen (aga_event_gen_req_t *req)
{
    sdk_ret_t ret;
    gpu_entry *entry;
    aga_event_gen_args_t args = {};

    for (auto i = 0; i < req->num_gpu; i++) {
        entry = gpu_db()->find(&req->gpu[i]);
        if (unlikely(entry == NULL)) {
            AGA_TRACE_ERR("Failed to generate events, GPU {} not found",
                          req->gpu[i].str());
            return SDK_RET_INVALID_ARG;
        }
        args.gpu_ids.push_back(entry->id());
        AGA_TRACE_VERBOSE("Generating test event for gpu {}",
                          req->gpu[i].str());
    }
    for (auto i = 0; i < req->num_event; i++) {
        args.events.push_back(req->event[i]);
        AGA_TRACE_VERBOSE("Generating test event {}", req->event[i]);
    }

    // send this request to backend event monitor thread
    sdk::ipc::FIXME_request(aga::AGA_THREAD_ID_EVENT_MONITOR,
                            aga::AGA_IPC_MSG_ID_EVENT_GEN, &args,
                            sizeof(aga_event_gen_args_t),
                            aga_event_gen_rsp_cb, &ret);
    if (unlikely(ret != SDK_RET_OK)) {
        AGA_TRACE_ERR("Failed to generate events, err {}", ret());
    }
    return ret;
}
