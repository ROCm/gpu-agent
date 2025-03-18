
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
/// This file implements APIs EventSvc
///
//----------------------------------------------------------------------------

#ifndef __AGA_SVC_EVENTS_HPP__
#define __AGA_SVC_EVENTS_HPP__

#include "grpc++/grpc++.h"
#include "gen/proto/gpuagent/types.pb.h"
#include "gen/proto/gpuagent/events.pb.h"
#include "gen/proto/gpuagent/events.grpc.pb.h"

using grpc::Status;
using grpc::ServerContext;
using grpc::ServerWriter;

using amdgpu::EventSvc;
using amdgpu::Event;
using amdgpu::EventSubscribeRequest;
using amdgpu::EventRequest;
using amdgpu::EventResponse;
using amdgpu::DebugEventSvc;
using amdgpu::EventGenRequest;
using amdgpu::EventGenResponse;

class EventSvcImpl final : public EventSvc::Service {
public:
    Status EventGet(ServerContext* context,
                    const EventRequest *request,
                    EventResponse *response) override;
    Status EventSubscribe(ServerContext* context,
                          const EventSubscribeRequest *request,
                          ServerWriter<Event> *stream) override;
};

class DebugEventSvcImpl final : public DebugEventSvc::Service {
public:
    Status EventGen(ServerContext* context,
                    const EventGenRequest *request,
                    EventGenResponse *response) override;
};

#endif    // __AGA_SVC_EVENTS_HPP__
