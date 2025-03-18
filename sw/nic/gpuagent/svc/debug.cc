
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
/// class that implements APIs in DebugSvc
///
//----------------------------------------------------------------------------

#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/svc/debug.hpp"

Status
DebugSvcImpl::TraceUpdate(ServerContext *context,
                          const amdgpu::TraceRequest *proto_req,
                          amdgpu::TraceResponse *proto_rsp) {
    trace_level_e trace_level;

    switch (proto_req->spec().tracelevel()) {
    case amdgpu::TRACE_LEVEL_NONE:
        trace_level = trace_level_none;
        break;
    case amdgpu::TRACE_LEVEL_DEBUG:
        trace_level = trace_level_debug;
        break;
    case amdgpu::TRACE_LEVEL_ERROR:
        trace_level = trace_level_err;
        break;
    case amdgpu::TRACE_LEVEL_WARN:
        trace_level = trace_level_warn;
        break;
    case amdgpu::TRACE_LEVEL_INFO:
        trace_level = trace_level_info;
        break;
    case amdgpu::TRACE_LEVEL_VERBOSE:
        trace_level = trace_level_verbose;
        break;
    default:
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::OK;
        break;
    }
    core::trace_update(trace_level);
    core::set_api_trace_enabled(proto_req->spec().apitraceen());
    proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_OK);
    return Status::OK;
}

Status
DebugSvcImpl::TraceGet(ServerContext *context, const Empty *proto_req,
                       amdgpu::TraceGetResponse *proto_rsp) {
    trace_level_e trace_level;

    trace_level = core::trace_level();
    switch (trace_level) {
    case trace_level_none:
        proto_rsp->set_tracelevel(amdgpu::TRACE_LEVEL_NONE);
        break;
    case trace_level_debug:
        proto_rsp->set_tracelevel(amdgpu::TRACE_LEVEL_DEBUG);
        break;
    case trace_level_err:
        proto_rsp->set_tracelevel(amdgpu::TRACE_LEVEL_ERROR);
        break;
    case trace_level_warn:
        proto_rsp->set_tracelevel(amdgpu::TRACE_LEVEL_WARN);
        break;
    case trace_level_info:
        proto_rsp->set_tracelevel(amdgpu::TRACE_LEVEL_INFO);
        break;
    case trace_level_verbose:
        proto_rsp->set_tracelevel(amdgpu::TRACE_LEVEL_VERBOSE);
        break;
    default:
        proto_rsp->set_tracelevel(amdgpu::TRACE_LEVEL_NONE);
        break;
    }
    proto_rsp->set_apitraceen(core::api_trace_enabled());
    // fill trace file
    proto_rsp->set_tracefile(core::trace_file_name());
    return Status::OK;
}

Status
DebugSvcImpl::TraceFlush(ServerContext *context, const Empty *req,
                         Empty *rsp) {
    core::flush_logs();
    return Status::OK;
}
