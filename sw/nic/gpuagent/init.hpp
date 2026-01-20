
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

#ifndef __AGA_INIT_H__
#define __AGA_INIT_H__

#include <string>
#include "nic/sdk/include/sdk/base.hpp"

/// \defgroup AGA_INIT initialization APIs
/// @{

/// GPU agent external gRPC port
#define AGA_DEFAULT_GRPC_SERVER_PORT          50061
/// gRPC server:port string length
#define AGA_GRPC_SERVER_STR_LEN               64

/// \brief initialization parameters
typedef struct aga_init_params_s {
    // gRPC server (IP:port)
    char grpc_server[AGA_GRPC_SERVER_STR_LEN];
} aga_init_params_t;

/// \brief    initialize the agent state, threads etc.
/// \param[in] init_params    init time parameters
/// \return     SDK_RET_OK or error status in case of failure
sdk_ret_t aga_init(aga_init_params_t *init_params);

#endif    // __AGA_INIT_H__
