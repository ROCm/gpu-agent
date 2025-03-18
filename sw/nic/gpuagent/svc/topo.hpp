

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
/// class that implements APIs in TopoSvc
///
//----------------------------------------------------------------------------

#ifndef __AGA_SVC_TOPO_HPP__
#define __AGA_SVC_TOPO_HPP__

#include "grpc++/grpc++.h"
#include "gen/proto/gpuagent/types.pb.h"
#include "gen/proto/gpuagent/topo.pb.h"
#include "gen/proto/gpuagent/topo.grpc.pb.h"

using grpc::Status;
using grpc::ServerContext;

using types::Empty;
using amdgpu::TopoSvc;
using amdgpu::DeviceTopologyGetRequest;
using amdgpu::DeviceTopologyGetResponse;
using amdgpu::DeviceConnection;
using amdgpu::Device;
using amdgpu::PeerDevice;
using amdgpu::DeviceTopology;

class TopoSvcImpl final : public TopoSvc::Service {
public:
    Status DeviceTopologyGet(ServerContext *context,
                             const DeviceTopologyGetRequest *proto_req,
                             DeviceTopologyGetResponse *proto_rsp) override;
};

#endif    // __AGA_SVC_TOPO_HPP__
