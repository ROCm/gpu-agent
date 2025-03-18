
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
/// This module defines protobuf conversion APIs for device topology object
///
//----------------------------------------------------------------------------

#ifndef __AGA_SVC_TOPO_TO_PROTO_HPP__
#define __AGA_SVC_TOPO_TO_PROTO_HPP__

#include <string>
#include "nic/gpuagent/svc/topo.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"

static inline amdgpu::DeviceType
aga_device_type_to_proto (aga_device_type_t type)
{
    switch (type) {
    case AGA_DEVICE_TYPE_GPU:
        return amdgpu::DEVICE_TYPE_GPU;
    default:
        return amdgpu::DEVICE_TYPE_NONE;
    }
}

static inline amdgpu::IOLinkType
aga_io_link_type_to_proto (aga_io_link_type_t type)
{
    switch (type) {
    case AGA_IO_LINK_TYPE_PCIE:
        return amdgpu::IO_LINK_TYPE_PCIE;
    case AGA_IO_LINK_TYPE_XGMI:
        return amdgpu::IO_LINK_TYPE_XGMI;
    default:
        return amdgpu::IO_LINK_TYPE_NONE;
    }
}

// populate proto buf from device topology info
static inline void
aga_device_topology_api_info_to_proto (aga_device_topology_info_t *info,
                                       void *ctxt)
{
    DeviceTopologyGetResponse *proto_rsp = (DeviceTopologyGetResponse *)ctxt;
    auto topo = proto_rsp->add_devicetopology();
    auto device = topo->mutable_device();

    device->set_type(aga_device_type_to_proto(info->device.type));
    device->set_name(info->device.name);

    for (uint32_t i = 0; i < AGA_MAX_PEER_DEVICE; i++) {
        auto peer_info = &info->peer_device[i];
        if (!peer_info->valid) {
            break;
        }
        auto peer_device = topo->add_peerdevice();
        peer_device->mutable_peerdevice()->set_type(
                         aga_device_type_to_proto(peer_info->peer_device.type));
        peer_device->mutable_peerdevice()->set_name(
                         peer_info->peer_device.name);
        peer_device->mutable_connection()->set_type(
                         aga_io_link_type_to_proto(peer_info->connection.type));
        peer_device->set_numhops(peer_info->num_hops);
        peer_device->set_weight(peer_info->link_weight);
    }
}

#endif    // __AGA_SVC_TOPO_TO_PROTO_HPP__
