
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
/// this file implements gpu watch CRUD APIs
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/assert.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/core/aga_core.hpp"
#include "nic/gpuagent/core/api.h"
#include "nic/gpuagent/core/aga_core.hpp"
#include "nic/gpuagent/core/api_ctxt.hpp"
#include "nic/gpuagent/core/api_msg.hpp"
#include "nic/gpuagent/api/include/base.hpp"
#include "nic/gpuagent/api/internal/aga_api_params.hpp"
#include "nic/gpuagent/api/include/aga_gpu_watch.hpp"
#include "nic/gpuagent/api/gpu_watch.hpp"
#include "nic/gpuagent/api/aga_state.hpp"

static sdk_ret_t
aga_gpu_watch_api_handle (api_op_t op, aga_obj_key_t *key,
                          aga_gpu_watch_spec_t *spec)
{
    sdk_ret_t ret;
    api_ctxt_t *api_ctxt;

    if ((ret = obj_api_validate(op, key, spec)) != SDK_RET_OK) {
        // basic API validation failure
        return ret;
    }
    // allocate API context
    api_ctxt = aga::api_ctxt_alloc(AGA_OBJ_ID_GPU_WATCH, op);
    if (likely(api_ctxt != NULL)) {
        if (op == API_OP_DELETE) {
            AGA_API_PARAMS_FROM_API_CTXT(api_ctxt)->key = *key;
        } else {
            AGA_API_PARAMS_FROM_API_CTXT(api_ctxt)->gpu_watch_spec = *spec;
        }
        return process_api(api_ctxt);
    }
    return SDK_RET_OOM;
}

static inline sdk_ret_t
aga_gpu_watch_entry_find (_In_ aga_obj_key_t *key, _Out_ gpu_watch_entry **gpu)
{
    gpu_watch_entry *entry;

    *gpu = NULL;
    entry = gpu_watch_db()->find(key);
    if (entry == NULL) {
        return SDK_RET_ENTRY_NOT_FOUND;
    }
    if (entry->in_use()) {
        return SDK_RET_IN_USE;
    }
    *gpu = entry;
    return SDK_RET_OK;
}

sdk_ret_t
aga_gpu_watch_create (_In_ aga_gpu_watch_spec_t *spec)
{
    return aga_gpu_watch_api_handle(API_OP_CREATE, NULL, spec);
}

sdk_ret_t
aga_gpu_watch_read (_In_ aga_obj_key_t *key, _Out_ aga_gpu_watch_info_t *info)
{
    sdk_ret_t ret;
    gpu_watch_entry *entry;

    if (unlikely((key == NULL) || (info == NULL))) {
        return SDK_RET_INVALID_ARG;
    }
    ret = aga_gpu_watch_entry_find(key, &entry);
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }
    return entry->read(info);
}

typedef struct aga_gpu_watch_read_args_s {
    void *ctxt;
    gpu_watch_read_cb_t cb;
} aga_gpu_watch_read_args_t;

bool
aga_gpu_watch_info_from_entry (void *entry, void *ctxt)
{
    aga_gpu_watch_info_t info;
    gpu_watch_entry *gpu_watch = (gpu_watch_entry *)entry;
    aga_gpu_watch_read_args_t *args = (aga_gpu_watch_read_args_t *)ctxt;

    if (gpu_watch->in_use()) {
        // some API operation is in progress on this object, skip it
        return false;
    }
    memset(&info, 0, sizeof(aga_gpu_watch_info_t));
    // call entry read
    gpu_watch->read(&info);
    // call cb on info
    args->cb(args->ctxt, &info);
    return false;
}

sdk_ret_t
aga_gpu_watch_read_all (gpu_watch_read_cb_t gpu_watch_read_cb, void *ctxt)
{
    aga_gpu_watch_read_args_t args = { 0 };

    args.ctxt = ctxt;
    args.cb = gpu_watch_read_cb;
    return gpu_watch_db()->walk(aga_gpu_watch_info_from_entry, &args);
}

sdk_ret_t
aga_gpu_watch_delete (_In_ aga_obj_key_t *key)
{
    return aga_gpu_watch_api_handle(API_OP_DELETE, key, NULL);
}

static void
aga_gpu_watch_subscribe_rsp_cb (sdk::ipc::ipc_msg_ptr msg, const void *status)
{
    if (msg) {
        *(sdk_ret_t *)status = *(sdk_ret_t *)msg->data();
    } else {
        *(sdk_ret_t *)status = SDK_RET_ERR;
    }
}

sdk_ret_t
aga_gpu_watch_subscribe (aga_gpu_watch_subscribe_req_t *req)
{
    sdk_ret_t ret;
    gpu_watch_entry *entry;
    aga_task_spec_t task_spec = {};
    aga_gpu_watch_subscribe_args_t *args;

    for (auto i = 0; i < req->num_gpu_watch_ids; i++) {
        ret = aga_gpu_watch_entry_find(&req->gpu_watch_ids[i], &entry);
        if (unlikely(ret != SDK_RET_OK)) {
            AGA_TRACE_ERR("Invalid subscribe request, GPU watch {} "
                          "does not exist",
                          req->gpu_watch_ids[i].str())
            return SDK_RET_INVALID_ARG;
        }
    }
    args = new aga_gpu_watch_subscribe_args_t();
    // allocate the client context
    args->client_ctxt = new aga_gpu_watch_client_ctxt_t();
    AGA_TRACE_VERBOSE("GPU watch subscribe request client_ctxt {}",
                      (void *)args->client_ctxt);
    // initialize the client context
    args->client_ctxt->client = req->client;
    args->client_ctxt->client_inactive = false;
    args->client_ctxt->cond = PTHREAD_COND_INITIALIZER;
    args->client_ctxt->lock = PTHREAD_MUTEX_INITIALIZER;
    args->client_ctxt->stream = req->stream;
    args->client_ctxt->write_cb = req->write_cb;

    for (auto i = 0; i < req->num_gpu_watch_ids; i++) {
        args->gpu_watch_ids.push_back(req->gpu_watch_ids[i]);
    }
    // send this request to backend service thread
    // NOTE: intention is to send pointer to the backend thread
    sdk::ipc::FIXME_request(aga::AGA_THREAD_ID_WATCHER,
                            aga::AGA_IPC_MSG_ID_GPU_WATCH_SUBSCRIBE, &args,
                            sizeof(args),
                            aga_gpu_watch_subscribe_rsp_cb, &ret);
    // post task to api thread to increment the subscriber count
    task_spec.task = AGA_TASK_GPU_WATCH_SUBSCRIBE_ADD;
    task_spec.subscriber_spec.num_gpu_watch_ids = req->num_gpu_watch_ids;
    for (auto i = 0; i < req->num_gpu_watch_ids; i++) {
        task_spec.subscriber_spec.gpu_watch_ids[i] = req->gpu_watch_ids[i];
    }
    ret = aga_task_create(&task_spec);
    if (unlikely(ret != SDK_RET_OK)) {
        AGA_TRACE_ERR("Failed to create task to increment subscriber ref count "
                      "for GPU watch, client {}, client ctxt {}",
                      req->client, (void *)args->client_ctxt);
    }

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
