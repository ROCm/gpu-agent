
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
/// gpuagent init handlers
///
//----------------------------------------------------------------------------

#include <memory>
#include <grpc++/grpc++.h>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/logger/logger.h"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/mem.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/include/base.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"
#include "nic/gpuagent/api/include/aga_init.hpp"
#include "nic/gpuagent/init.hpp"
#include "nic/gpuagent/include/globals.hpp"
#include "nic/gpuagent/svc/gpu.hpp"
#include "nic/gpuagent/svc/gpu_watch.hpp"
#include "nic/gpuagent/svc/topo.hpp"
#include "nic/gpuagent/svc/debug.hpp"
#include "nic/gpuagent/svc/events.hpp"
#include "nic/gpuagent/api/smi/smi_api.hpp"

using grpc::Server;
using grpc::ServerBuilder;

// TODO:
// move to aga_state
static std::unique_ptr<Server> g_grpc_server = NULL;

/// \brief    create GPU objects during init time
/// \return     SDK_RET_OK or error status in case of failure
static inline sdk_ret_t
create_gpus (void)
{
    sdk_ret_t ret;
    uint32_t num_gpu;
    uint32_t partition_id;
    aga_gpu_spec_t spec = { 0 };
    aga_obj_key_t gpu_key[AGA_MAX_GPU];
    aga_gpu_handle_t gpu_handles[AGA_MAX_GPU];
    std::unordered_map<aga_obj_key_t, uint32_t,
        aga_obj_key_hash> key_count_map;
    std::unordered_map<aga_obj_key_t, bool,
        aga_obj_key_hash> parent_gpu_map;

    ret = aga::smi_discover_gpus(&num_gpu, gpu_handles, gpu_key);
    if (ret != SDK_RET_OK) {
        AGA_TRACE_ERR("GPU discovery failed, err {}", ret());
        return SDK_RET_ERR;
    }
    // go through GPUs to detect partitions if any; when a GPU is
    // partitioned, all the children share the same UUID; we use this to detect
    // partitions
    for (uint32_t i = 0; i < num_gpu; i++) {
        key_count_map[gpu_key[i]]++;
    }
    // first create parent GPUs for any partitioned GPUs discovered
    for (uint32_t i = 0; i < num_gpu; i++) {
        // check if it is a partitioned GPU (multiple GPUs with the same UUID)
        if (key_count_map[gpu_key[i]] > 1) {
            if (parent_gpu_map.find(gpu_key[i]) != parent_gpu_map.end()) {
                // parent GPU already created
                continue;
            }
            // set parent GPU uuid
            spec.key = gpu_key[i];
            // parent GPUs cannot have a parent themselves
            spec.parent_gpu.reset();
        } else {
            continue;
        }
        AGA_TRACE_DEBUG("Creating parent GPU {}", spec.key.str());
        // attempt to create gpu object
        ret = aga_gpu_create(&spec);
        if (unlikely(ret != SDK_RET_OK)) {
            AGA_TRACE_ERR("GPU {} creation failed, err {}", spec.key.str(),
                          ret());
            // continue to next gpu
            continue;
        }
        // add parent GPU to map
        parent_gpu_map[spec.key] = true;
    }
    // start creating the GPU objects
    AGA_TRACE_DEBUG("Creating {} GPU objects ...", num_gpu);
    for (uint32_t i = 0; i < num_gpu; i++) {
        spec.key = gpu_key[i];
        // check if it is a partitioned GPU
        if (key_count_map[gpu_key[i]] > 1) {
            // set parent GPU key
            spec.parent_gpu = spec.key;
            // set GPU partition ID
            ret = aga::smi_get_gpu_partition_id(gpu_handles[i],
                                                &partition_id);
            if (unlikely(ret != SDK_RET_OK)) {
                AGA_TRACE_ERR("GPU {} creation failed, err {}", spec.key.str(),
                              ret());
                // continue to next gpu
                continue;
            }
            // all child GPUs share the UUID of the parent; to differentiate we
            // encode the partition id in the UUID
            *(uint32_t *)&spec.key.id[4] = partition_id;
        } else {
            // set parent gpu to be invalid
            spec.parent_gpu.reset();
            // set partition ID to invalid
            partition_id = AGA_GPU_INVALID_PARTITION_ID;
        }
        AGA_TRACE_DEBUG("Creating GPU {} id {} handle {}",
                        spec.key.str(), i, gpu_handles[i]);
        // attempt to create gpu object
        ret = aga_gpu_create(&spec);
        if (unlikely(ret != SDK_RET_OK)) {
            AGA_TRACE_ERR("GPU {} creation failed, err {}", spec.key.str(),
                          ret());
            // continue to next gpu
            continue;
        }
        // gpu objects need to be searchable by handle; so add them to a map
        // indexed by their handles
        auto entry = gpu_db()->find(&spec.key);
        if (!entry) {
            AGA_TRACE_ERR("GPU {} entry not found", spec.key.str());
            // continue to next gpu
            continue;
        }
        // set GPU id
        entry->set_id(i);
        // set GPU handle
        entry->set_handle(gpu_handles[i]);
        // set partition id
        entry->set_partition_id(partition_id);
        // initialize GPU spec
        entry->init_spec();
        // insert in handle db
        gpu_db()->insert_in_handle_db(entry);
        // if GPU is a child GPU, add to the parent GPU
        if (spec.parent_gpu.valid()) {
            auto parent_entry = gpu_db()->find(&spec.parent_gpu);
            if (!parent_entry) {
                // should we error out; this shouldn't happen
                AGA_TRACE_ERR("Parent GPU {} for GPU {} not found",
                              spec.parent_gpu.str(), spec.key.str());
                continue;
            }
            parent_entry->add_child_gpu(&spec.key);
            parent_entry->set_compute_partition_type(
                              spec.compute_partition_type);
        }
    }
    return SDK_RET_OK;
}

/// \brief    start the gRPC server
/// \param[in] grpc_server    gRPC server (IP:port) string
static void
grpc_server_start (std::string grpc_server)
{
    GPUSvcImpl gpu_svc;
    TopoSvcImpl topo_svc;
    DebugSvcImpl debug_svc;
    EventSvcImpl event_svc;
    ServerBuilder server_builder;
    DebugGPUSvcImpl debug_gpu_svc;
    GPUWatchSvcImpl gpu_watch_svc;
    grpc::ResourceQuota rsc_quota;
    DebugEventSvcImpl debug_event_svc;

    // do gRPC initialization
    grpc_init();
    server_builder.SetMaxReceiveMessageSize(INT_MAX);
    server_builder.SetMaxSendMessageSize(INT_MAX);

    // enable keepalive for all the server connections
    // keepalive period, in milliseconds
    server_builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 60000);
    // response timeout for the keepalive request, in milliseconds
    server_builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 5000);
    // send keepalive even if there are no ongoing RPCs in the connection
    server_builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS,
                                      1);
    // send continuous keepalive messages as long as channel is open
    server_builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);
    server_builder.AddListeningPort(grpc_server,
                                    grpc::InsecureServerCredentials());
    // restrict max. no. of gRPC threads that can be spawned & active at any
    // given point of time
    rsc_quota.SetMaxThreads(AGA_MAX_GRPC_THREADS);
    server_builder.SetResourceQuota(rsc_quota);
    // register for all the services
    server_builder.RegisterService(&gpu_svc);
    server_builder.RegisterService(&debug_svc);
    server_builder.RegisterService(&event_svc);
    server_builder.RegisterService(&debug_event_svc);
    server_builder.RegisterService(&debug_gpu_svc);
    server_builder.RegisterService(&topo_svc);
    server_builder.RegisterService(&gpu_watch_svc);
    // start the gRPC server now
    AGA_TRACE_DEBUG("gRPC server listening on {} ...",
                    grpc_server.c_str());
    g_grpc_server = server_builder.BuildAndStart();
    g_grpc_server->Wait();
}

static int
sdk_logger_ (uint32_t mod_id, trace_level_e level, const char *logbuf)
{
    switch (level) {
    case trace_level_err:
        AGA_MOD_TRACE_ERR_NO_META(mod_id, "{}", logbuf);
        break;
    case trace_level_warn:
        AGA_MOD_TRACE_WARN_NO_META(mod_id, "{}", logbuf);
        break;
    case trace_level_info:
        AGA_MOD_TRACE_INFO_NO_META(mod_id, "{}", logbuf);
        break;
    case trace_level_debug:
        AGA_MOD_TRACE_DEBUG_NO_META(mod_id, "{}", logbuf);
        break;
    case trace_level_verbose:
        AGA_MOD_TRACE_VERBOSE_NO_META(mod_id, "{}", logbuf);
        break;
    default:
        break;
    }
    return 0;
}

//------------------------------------------------------------------------------
// logger callback passed to SDK and PDS lib
//------------------------------------------------------------------------------
static int
sdk_logger (uint32_t mod_id, trace_level_e trace_level, const char *format, ...)
{
    va_list args;
    char logbuf[1024];

    va_start(args, format);
    vsnprintf(logbuf, sizeof(logbuf), format, args);
    sdk_logger_(mod_id, trace_level, logbuf);
    va_end(args);
    return 0;
}

sdk_ret_t
aga_init (aga_init_params_t *init_params)
{
    sdk_ret_t ret;
    aga_api_init_params_t api_init_params = {};

    // initialize tracing
    core::trace_init();
    // initialize sdk logger
    logger_init(sdk_logger);
    // initialize API layer
    aga_api_init(&api_init_params);
    // do gRPC library init
    grpc_init();
    // create the GPU objects now
    ret = create_gpus();
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }
    // register for all gRPC services and start the gRPC server
    grpc_server_start(init_params->grpc_server);
    return SDK_RET_OK;
}
