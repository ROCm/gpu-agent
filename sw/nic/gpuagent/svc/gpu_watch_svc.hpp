


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
/// APIs for GPU watch object at the svc layer
///
//----------------------------------------------------------------------------

#ifndef __AGA_SVC_GPU_WATCH_SVC_HPP__
#define __AGA_SVC_GPU_WATCH_SVC_HPP__

#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/include/base.hpp"
#include "nic/gpuagent/svc/utils.hpp"
#include "nic/gpuagent/svc/gpu_watch.hpp"
#include "nic/gpuagent/svc/gpu_watch_to_proto.hpp"
#include "nic/gpuagent/svc/gpu_watch_to_spec.hpp"
#include "nic/gpuagent/api/include/aga_gpu_watch.hpp"
#include "nic/gpuagent/api/internal/aga_gpu_watch.hpp"

static inline sdk_ret_t
aga_svc_gpu_watch_create (const GPUWatchRequest *proto_req,
                          GPUWatchResponse *proto_rsp)
{
    sdk_ret_t ret;
    aga_gpu_watch_spec_t api_spec;

    if ((proto_req == NULL) || (proto_req->spec_size() == 0)){
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }
    aga_api_trace_verbose("GPUWatch", "Create", proto_req);
    for (int i = 0; i < proto_req->spec_size(); i ++) {
        auto spec = proto_req->spec(i);

        api_spec = {};
        ret = aga_gpu_watch_proto_to_api_spec(&api_spec, spec);
        if (unlikely(ret != SDK_RET_OK)) {
            goto end;
        }
        ret = aga_gpu_watch_create(&api_spec);
        if (unlikely(ret != SDK_RET_OK)) {
            goto end;
        }
    }
end:
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

static inline sdk_ret_t
aga_svc_gpu_watch_delete (const GPUWatchDeleteRequest *proto_req,
                          GPUWatchDeleteResponse *proto_rsp)
{
    sdk_ret_t ret;
    aga_obj_key_t key;

    if ((proto_req == NULL) || (proto_req->id_size() == 0)) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }
    aga_api_trace_verbose("GPUWatch", "Delete", proto_req);
    for (int i = 0; i < proto_req->id_size(); i++) {
        aga_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        ret = aga_gpu_watch_delete(&key);
        if (unlikely(ret != SDK_RET_OK)) {
            goto end;
        }
    }
end:
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

static inline sdk_ret_t
aga_svc_gpu_watch_get (const GPUWatchGetRequest *proto_req,
                       GPUWatchGetResponse *proto_rsp)
{
    sdk_ret_t ret;
    aga_obj_key_t key;
    aga_gpu_watch_info_t info;

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }
    aga_api_trace_verbose("GPUWatch", "Get", proto_req);
    if (proto_req->id_size() == 0) {
        ret = aga_gpu_watch_read_all(aga_gpu_watch_info_to_get_rsp_proto,
                                     proto_rsp);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
        return ret;
    }
    for (int i = 0; i < proto_req->id_size(); i ++) {
        aga_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        memset(&info, 0, sizeof(aga_gpu_watch_info_t));
        ret = aga_gpu_watch_read(&key, &info);
        if (unlikely(ret != SDK_RET_OK)) {
            proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
            break;
        }
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_OK);
        aga_gpu_watch_info_to_get_rsp_proto(proto_rsp, &info);
    }
    return ret;
}

sdk_ret_t
aga_svc_gpu_watch_subscribe_write_cb (const aga_gpu_watch_info_t *info,
                                      void *ctxt)
{
    bool rv;
    GPUWatch proto_rsp;
    aga_gpu_watch_client_ctxt_t *client_ctxt;

    client_ctxt = (aga_gpu_watch_client_ctxt_t *)ctxt;
    aga_gpu_watch_info_to_proto(&proto_rsp, info);
    // try to write to the client stream
    rv =
        ((grpc::ServerWriter<GPUWatch> *)client_ctxt->stream)->Write(proto_rsp);
    if (unlikely(rv == false)) {
        AGA_TRACE_ERR("Failed to notify gpu watch {} to client {}",
                      info->spec.key.str(), client_ctxt->client.c_str());
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

static inline sdk_ret_t
aga_svc_gpu_watch_subscribe(ServerContext* context,
    const GPUWatchSubscribeRequest *proto_req,
    grpc::ServerWriter<GPUWatch> *stream) {
    aga_gpu_watch_subscribe_req_t req;

    if (proto_req->id_size() == 0) {
        // empty event subscribe request is not supported
        AGA_TRACE_ERR("Empty GPU watch subscribe request is not supported");
        return SDK_RET_INVALID_ARG;
    }
    aga_api_trace_verbose("GPUWatch", "Subscribe", proto_req);
    AGA_TRACE_VERBOSE("Rcvd GPU watch subscribe request from client {}, "
                      "stream {}", context->peer().c_str(), (void *)stream);

    for (auto i = 0; i < proto_req->id_size(); i++) {
        aga_obj_key_proto_to_api_spec(&req.gpu_watch_ids[i], proto_req->id(i));
        AGA_TRACE_DEBUG("Client {}, stream {} subscribed for watch group {}",
                        context->peer().c_str(), (void *)stream,
                        req.gpu_watch_ids[i].str());
    }
    req.num_gpu_watch_ids = proto_req->id_size();
    req.write_cb = aga_svc_gpu_watch_subscribe_write_cb;
    strncpy(req.client, context->peer().c_str(), AGA_MAX_GPU_WATCH_CLIENT_STR);
    req.stream = stream;
    aga_gpu_watch_subscribe(&req);

    return SDK_RET_OK;
}

#endif    // __AGA_SVC_GPU_WATCH_SVC_HPP__
