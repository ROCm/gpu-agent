
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
/// init time APIs for agent
///
//----------------------------------------------------------------------------

#ifndef __AGA_INIT_HPP__
#define __AGA_INIT_HPP__

#include <string>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/api/include/aga_event.hpp"
#include "nic/gpuagent/api/internal/aga_event.hpp"

/// \defgroup AGA_INIT initialization APIs
/// @{

/// \brief initialization parameters
typedef struct aga_init_params_s {
    // gRPC server (IP:port)
    std::string grpc_server;
    // rdcd gRPC server (IP:port) to connect to
    std::string rdc_server;
} aga_init_params_t;

/// \brief    initialize the agent state, threads etc.
/// \param[in] init_params    init time parameters
/// \return     SDK_RET_OK or error status in case of failure
sdk_ret_t aga_init(aga_init_params_t *init_params);

#endif    // __AGA_INIT_HPP__
