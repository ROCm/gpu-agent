
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
/// APIs for GPU object at the svc layer
///
//----------------------------------------------------------------------------

#ifndef __AGA_SVC_GPU_SVC_HPP__
#define __AGA_SVC_GPU_SVC_HPP__

#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/include/base.hpp"
#include "nic/gpuagent/svc/utils.hpp"
#include "nic/gpuagent/svc/gpu.hpp"
#include "nic/gpuagent/svc/gpu_to_proto.hpp"
#include "nic/gpuagent/svc/gpu_to_spec.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"

static inline sdk_ret_t
aga_svc_gpu_get (const GPUGetRequest *proto_req, GPUGetResponse *proto_rsp)
{
    sdk_ret_t ret;
    aga_obj_key_t key;
    aga_gpu_info_t info;

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }
    aga_api_trace_verbose("GPU", "Get", proto_req);
    if (proto_req->id_size() == 0) {
        ret = aga_gpu_read_all(aga_gpu_api_info_to_proto, proto_rsp);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
        return ret;
    }
    for (int i = 0; i < proto_req->id_size(); i ++) {
        aga_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        memset(&info, 0, sizeof(aga_gpu_info_t));
        ret = aga_gpu_read(&key, &info);
        if (unlikely(ret != SDK_RET_OK)) {
            proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
            break;
        }
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_OK);
        aga_gpu_api_info_to_proto(&info, proto_rsp);
    }
    return ret;
}

static inline sdk_ret_t
aga_svc_gpu_update (const GPUUpdateRequest *proto_req,
                    GPUUpdateResponse *proto_rsp)
{
    sdk_ret_t ret;
    aga_gpu_spec_t api_spec;

    if ((proto_req == NULL) || (proto_req->spec_size() == 0)) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }
    aga_api_trace_verbose("GPU", "Update", proto_req);
    for (int i = 0; i < proto_req->spec_size(); i++) {
        auto spec = proto_req->spec(i);

        api_spec = {};
        ret = aga_gpu_proto_to_api_spec(&api_spec, spec);
        if (unlikely(ret != SDK_RET_OK)) {
            goto end;
        }
        ret = aga_gpu_update(&api_spec);
        if (ret != SDK_RET_OK) {
            goto end;
        }
    }
end:
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

static inline sdk_ret_t
aga_svc_gpu_reset (const GPUResetRequest *proto_req,
                   GPUResetResponse *proto_rsp)
{
    sdk_ret_t ret;
    aga_task_spec_t api_spec = {};

    // validate the request
    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }
    aga_api_trace_verbose("GPU", "Reset", proto_req);
    // convert the reset request to canonical form
    ret = aga_gpu_reset_request_proto_to_api_spec(
              &api_spec.gpu_reset_task_spec, proto_req);
    if (unlikely(ret != SDK_RET_OK)) {
        goto end;
    }
    // TODO: we can generate internal uuid for tasks
    api_spec.task = AGA_TASK_GPU_RESET;
    ret = aga_task_create(&api_spec);
    if (ret != SDK_RET_OK) {
        goto end;
    }
end:
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

static inline sdk_ret_t
aga_svc_gpu_bad_page_get (const GPUBadPageGetRequest *proto_req,
                          grpc::ServerWriter<GPUBadPageGetResponse> *writer)
{
    sdk_ret_t ret;
    streaming_get_ctxt_t get_ctxt;
    GPUBadPageGetResponse rsp_msg, *proto_rsp;
    aga_obj_key_t key = k_aga_obj_key_invalid;

    proto_rsp = &rsp_msg;
    get_ctxt.msg_ctxt = proto_rsp;
    get_ctxt.writer_ctxt = writer;

    aga_api_trace_verbose("GPUBadPage", "Get", proto_req);
    if ((proto_req == NULL) || (proto_req->id_size() == 0)) {
        ret = aga_gpu_bad_page_read(&key, aga_gpu_bad_page_api_info_to_proto,
                                    &get_ctxt);
    }
    for (int i = 0; i < proto_req->id_size(); i ++) {
        aga_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        ret = aga_gpu_bad_page_read(&key, aga_gpu_bad_page_api_info_to_proto,
                                    &get_ctxt);
    }
    if (proto_rsp->record_size() > 0) {
        // flush the final batch of records
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
        proto_rsp->set_errorcode(sdk_ret_to_error_code(ret));
        if (!writer->Write(rsp_msg)) {
            AGA_TRACE_ERR("Failed to write gpu bad page info to gRPC stream");
            ret = SDK_RET_ERR;
        }
    } else if (get_ctxt.count == 0) {
        // no object found send empty response
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
        proto_rsp->set_errorcode(sdk_ret_to_error_code(ret));
        if (!writer->Write(rsp_msg)) {
            AGA_TRACE_ERR("Failed to write gpu bad page info to gRPC stream");
            ret = SDK_RET_ERR;
        }
    }
    return ret;
}

static inline sdk_ret_t
aga_svc_gpu_compute_partition_get (
    const GPUComputePartitionGetRequest *proto_req,
    GPUComputePartitionGetResponse *proto_rsp)
{
    sdk_ret_t ret;
    aga_obj_key_t key;
    aga_gpu_compute_partition_info_t info;

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }
    aga_api_trace_verbose("GPUComputePartition", "Get", proto_req);
    if (proto_req->id_size() == 0) {
        ret = aga_gpu_compute_partition_read_all(
                  aga_gpu_compute_partition_info_to_proto, proto_rsp);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
        return ret;
    }
    for (int i = 0; i < proto_req->id_size(); i ++) {
        aga_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        memset(&info, 0, sizeof(aga_gpu_compute_partition_info_t));
        ret = aga_gpu_compute_partition_read(&key, &info);
        if (unlikely(ret != SDK_RET_OK)) {
            proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
            break;
        }
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_OK);
        aga_gpu_compute_partition_info_to_proto(&info, proto_rsp);
    }
    return ret;

}

#endif    // __AGA_SVC_GPU_SVC_HPP__
