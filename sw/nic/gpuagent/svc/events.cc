
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
/// class that implements APIs in EventSvc
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/svc/utils.hpp"
#include "nic/gpuagent/svc/events.hpp"
#include "nic/gpuagent/svc/events_svc.hpp"

Status
EventSvcImpl::EventGet(ServerContext *server_ctx,
                       const EventRequest *req,
                       EventResponse *rsp) {
    sdk_ret_t ret;

    ret = aga_svc_event_get(req, rsp);
    rsp->set_apistatus(sdk_ret_to_api_status(ret));
    rsp->set_errorcode(sdk_ret_to_error_code(ret));
    return Status::OK;
}

Status
EventSvcImpl::EventSubscribe(ServerContext* context,
                             const EventSubscribeRequest *proto_req,
                             ServerWriter<Event> *stream) {
    aga_svc_event_subscribe(context, proto_req, stream);
    return Status::OK;
}

Status
DebugEventSvcImpl::EventGen(ServerContext* context,
                            const EventGenRequest *req,
                            EventGenResponse *rsp) {
    sdk_ret_t ret;

    ret = aga_svc_event_gen(req);
    rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;
}
