
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


#ifndef __AGA_SVC_DEBUG_HPP__
#define __AGA_SVC_DEBUG_HPP__

#include "grpc++/grpc++.h"
#include "gen/proto/gpuagent/types.pb.h"
#include "gen/proto/gpuagent/debug.grpc.pb.h"

using grpc::Status;
using grpc::ServerContext;

using amdgpu::DebugSvc;
using types::Empty;
using amdgpu::TraceRequest;
using amdgpu::TraceResponse;
using amdgpu::TraceGetResponse;

class DebugSvcImpl final : public DebugSvc::Service {
public:
    Status TraceUpdate(ServerContext *context, const TraceRequest *req,
                       TraceResponse *rsp) override;
    Status TraceGet(ServerContext *context, const Empty *req,
                         TraceGetResponse *rsp) override;
    Status TraceFlush(ServerContext *context, const Empty *req,
                      Empty *rsp) override;
};

#endif    // __AGA_SVC_DEBUG_HPP__
