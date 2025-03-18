
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
/// class that implements APIs in GPUWatchSvc
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/svc/utils.hpp"
#include "nic/gpuagent/svc/gpu_watch.hpp"
#include "nic/gpuagent/svc/gpu_watch_svc.hpp"

Status
GPUWatchSvcImpl::GPUWatchCreate(ServerContext *context,
                                const GPUWatchRequest *proto_req,
                                GPUWatchResponse *proto_rsp) {
    sdk_ret_t ret;

    ret = aga_svc_gpu_watch_create(proto_req, proto_rsp);
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;
}

Status
GPUWatchSvcImpl::GPUWatchDelete(ServerContext *context,
                                const GPUWatchDeleteRequest *proto_req,
                                GPUWatchDeleteResponse *proto_rsp) {
    sdk_ret_t ret;

    ret = aga_svc_gpu_watch_delete(proto_req, proto_rsp);
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;
}

Status
GPUWatchSvcImpl::GPUWatchGet(ServerContext *context,
                             const GPUWatchGetRequest *proto_req,
                             GPUWatchGetResponse *proto_rsp) {
    sdk_ret_t ret;

    ret = aga_svc_gpu_watch_get(proto_req, proto_rsp);
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;
}

Status
GPUWatchSvcImpl::GPUWatchSubscribe(ServerContext *context,
                     const GPUWatchSubscribeRequest *proto_req,
                     grpc::ServerWriter<GPUWatch> *writer) {
    sdk_ret_t ret;

    ret = aga_svc_gpu_watch_subscribe(context, proto_req, writer);
    return Status::OK;
}
