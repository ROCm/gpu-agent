

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

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/svc/utils.hpp"
#include "nic/gpuagent/svc/topo.hpp"
#include "nic/gpuagent/svc/topo_svc.hpp"

Status
TopoSvcImpl::DeviceTopologyGet(ServerContext *context,
                               const DeviceTopologyGetRequest *proto_req,
                               DeviceTopologyGetResponse *proto_rsp) {
    sdk_ret_t ret;

    ret = aga_svc_device_topology_get(proto_req, proto_rsp);
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    proto_rsp->set_errorcode(sdk_ret_to_error_code(ret));
    return Status::OK;
}
