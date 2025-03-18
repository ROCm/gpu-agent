
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
/// API message processing helper functions
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/gpuagent/core/api_msg.hpp"
#include "nic/gpuagent/core/api_ctxt.hpp"
#include "nic/gpuagent/core/aga_core.hpp"
#include "nic/gpuagent/core/trace.hpp"

namespace aga {

/// \brief    wrapper function to allocate an API msg
/// \return   pointer to allocated API msg or NULL
static inline api_msg_t *
api_msg_alloc (void)
{
    void *mem;
    api_msg_t *api_msg;

    mem = calloc(1, sizeof(api_msg_t));
    if (unlikely(mem == NULL)) {
        return NULL;
    }
    api_msg = new (mem) api_msg_t();
    return api_msg;
}

/// \brief    wrapper function to free an api msg
/// \param[in] msg    API msg to be freed
static inline void
api_msg_free (api_msg_t *msg)
{
    msg->~api_msg_t();
    free(msg);
}

// allocate and initialize API IPC msg
static inline api_msg_t *
api_msg_init (api_ctxt_t *api_ctxt)
{
    api_msg_t *api_msg;

    api_msg = api_msg_alloc();
    if (unlikely(api_msg == NULL)) {
        return NULL;
    }
    api_msg->msg_id = AGA_IPC_MSG_ID_CFG;
    api_msg->req.async = api_ctxt->async;
    api_msg->req.response_cb = api_ctxt->response_cb;
    api_msg->req.cookie = api_ctxt->cookie;
    api_msg->req.apis.reserve(1);

    // stash the given API context
    api_msg->req.apis.push_back(api_ctxt);
    return api_msg;
}

static inline sdk_ret_t
api_msg_destroy (api_msg_t *api_msg)
{
    // free all the API contexts
    for (auto it = api_msg->req.apis.begin();
         it != api_msg->req.apis.end(); ++it) {
        api_ctxt_free(*it);
    }
    // clear the contents of the API vector
    api_msg->req.apis.clear();
    // free the API message
    api_msg_free(api_msg);
    return SDK_RET_OK;
}

// callback that gets called when process_api() is done with it's return code
static void
api_process_sync_result_ (sdk::ipc::ipc_msg_ptr msg, const void *ret)
{
    *(sdk_ret_t *)ret = *(sdk_ret_t *)msg->data();
}

static void
api_process_async_result_ (sdk::ipc::ipc_msg_ptr msg, const void *ctx)
{
    api_msg_t *api_msg = (api_msg_t *)ctx;
    sdk_ret_t ret = *(sdk_ret_t *)msg->data();

    if (api_msg->req.response_cb != NULL) {
        api_msg->req.response_cb(ret, api_msg->req.cookie);
    }
    // destroy the API msg
    api_msg_destroy(api_msg);
}

sdk_ret_t
process_api (api_ctxt_t *api_ctxt)
{
    sdk_ret_t ret;
    api_msg_t *api_msg;

    // allocate and initilize API context
    api_msg = api_msg_init(api_ctxt);
    if (unlikely(api_msg == NULL)) {
        return SDK_RET_OOM;
    }
    if (api_ctxt->async) {
        // send API msg to API and receive the response asynchronously
        sdk::ipc::request(AGA_THREAD_ID_API, AGA_IPC_MSG_ID_CFG, &api_msg,
                          sizeof(api_msg), api_process_async_result_, api_msg);
        return SDK_RET_OK;
    } else {
        // send API msg to API thread and receive the response synchronously
        sdk::ipc::FIXME_request(AGA_THREAD_ID_API, AGA_IPC_MSG_ID_CFG,
                                &api_msg, sizeof(api_msg),
                                api_process_sync_result_, &ret);
        // free the API msg
        api_msg_destroy(api_msg);
    }
    return ret;
}

}    // namespace aga
