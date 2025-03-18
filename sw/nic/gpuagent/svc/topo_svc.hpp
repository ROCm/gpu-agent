
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
/// APIs for device topology object at the svc layer
///
//----------------------------------------------------------------------------

#ifndef __AGA_SVC_TOPO_SVC_HPP__
#define __AGA_SVC_TOPO_SVC_HPP__

#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/include/base.hpp"
#include "nic/gpuagent/svc/utils.hpp"
#include "nic/gpuagent/svc/topo.hpp"
#include "nic/gpuagent/svc/topo_to_proto.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"

static inline sdk_ret_t
aga_svc_device_topology_get (const DeviceTopologyGetRequest *proto_req,
                             DeviceTopologyGetResponse *proto_rsp)
{
    sdk_ret_t ret;

    aga_api_trace_verbose("DeviceTopology", "Get", proto_req);
    ret = aga_gpu_read_topology_all(aga_device_topology_api_info_to_proto,
                                    proto_rsp);
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

#endif    // __AGA_SVC_TOPO_SVC_HPP__
