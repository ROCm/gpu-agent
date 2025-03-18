
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

#ifndef __AGA_SVC_GPU_WATCH_HPP__
#define __AGA_SVC_GPU_WATCH_HPP__

#include "grpc++/grpc++.h"
#include "gen/proto/gpuagent/types.pb.h"
#include "gen/proto/gpuagent/gpu_watch.pb.h"
#include "gen/proto/gpuagent/gpu_watch.grpc.pb.h"

using grpc::Status;
using grpc::ServerContext;

using types::Empty;
using amdgpu::GPUWatchAttrId;
using amdgpu::GPUWatchSvc;
using amdgpu::GPUWatchSpec;
using amdgpu::GPUWatchStatus;
using amdgpu::GPUWatchStats;
using amdgpu::GPUWatchRequest;
using amdgpu::GPUWatchResponse;
using amdgpu::GPUWatchDeleteRequest;
using amdgpu::GPUWatchDeleteResponse;
using amdgpu::GPUWatchGetRequest;
using amdgpu::GPUWatchGetResponse;
using amdgpu::GPUWatchSubscribeRequest;
using amdgpu::GPUWatch;

class GPUWatchSvcImpl final : public GPUWatchSvc::Service {
public:
    Status GPUWatchCreate(ServerContext *context,
                          const GPUWatchRequest *proto_req,
                          GPUWatchResponse *proto_rsp) override;
    Status GPUWatchDelete(ServerContext *context,
                          const GPUWatchDeleteRequest *proto_req,
                          GPUWatchDeleteResponse *proto_rsp) override;
    Status GPUWatchGet(ServerContext *context,
                       const GPUWatchGetRequest *proto_req,
                       GPUWatchGetResponse *proto_rsp) override;
    Status GPUWatchSubscribe(ServerContext *context,
               const GPUWatchSubscribeRequest *proto_req,
               grpc::ServerWriter<GPUWatch> *writer) override;
};

#endif    // __AGA_SVC_GPU_WATCH_HPP__
