
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
/// class that implements APIs in GPUSvc
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/svc/utils.hpp"
#include "nic/gpuagent/svc/gpu.hpp"
#include "nic/gpuagent/svc/gpu_svc.hpp"

Status
GPUSvcImpl::GPUGet(ServerContext *context,
                   const GPUGetRequest *proto_req,
                   GPUGetResponse *proto_rsp) {
    sdk_ret_t ret;

    ret = aga_svc_gpu_get(proto_req, proto_rsp);
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    proto_rsp->set_errorcode(sdk_ret_to_error_code(ret));
    return Status::OK;
}

Status
GPUSvcImpl::GPUUpdate(ServerContext *context,
                      const GPUUpdateRequest *proto_req,
                      GPUUpdateResponse *proto_rsp) {
    sdk_ret_t ret;

    ret = aga_svc_gpu_update(proto_req, proto_rsp);
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    proto_rsp->set_errorcode(sdk_ret_to_error_code(ret));
    return Status::OK;
}

Status
GPUSvcImpl::GPUReset(ServerContext *context,
                     const GPUResetRequest *proto_req,
                     GPUResetResponse *proto_rsp) {
    sdk_ret_t ret;

    ret = aga_svc_gpu_reset(proto_req, proto_rsp);
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    proto_rsp->set_errorcode(sdk_ret_to_error_code(ret));
    return Status::OK;
}

Status
DebugGPUSvcImpl::GPUBadPageGet(ServerContext *context,
                     const GPUBadPageGetRequest *proto_req,
                     grpc::ServerWriter<GPUBadPageGetResponse> *writer) {
    aga_svc_gpu_bad_page_get(proto_req, writer);
    return Status::OK;
}

Status
GPUSvcImpl::GPUComputePartitionSet(ServerContext *context,
                const GPUComputePartitionSetRequest *proto_req,
                GPUComputePartitionSetResponse *proto_rsp) {
    sdk_ret_t ret = SDK_RET_INVALID_OP;

    // TODO uncomment after implementing it
    //ret = aga_svc_gpu_compute_partition_set(proto_req, proto_rsp);
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    proto_rsp->set_errorcode(sdk_ret_to_error_code(ret));
    return Status::OK;
}

Status
GPUSvcImpl::GPUComputePartitionGet(ServerContext *context,
                const GPUComputePartitionGetRequest *proto_req,
                GPUComputePartitionGetResponse *proto_rsp) {
    sdk_ret_t ret;

    ret = aga_svc_gpu_compute_partition_get(proto_req, proto_rsp);
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    proto_rsp->set_errorcode(sdk_ret_to_error_code(ret));
    return Status::OK;
}
