
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
/// initialization APIs
///
//----------------------------------------------------------------------------

#ifndef __API_INCLUDE_AGA_INIT_HPP__
#define __API_INCLUDE_AGA_INIT_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/api/include/aga_event.hpp"

/// \brief    initialization parameters
typedef struct aga_api_init_params_s {
} aga_api_init_params_t;

/// \brief    initialization routine for API layer
/// \param[in] init_params    init time parameters
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_api_init(aga_api_init_params_t *init_params);

/// \brief    cleanup routine for API layer
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_api_teardown(void);

#endif    /// __API_INCLUDE_AGA_INIT_HPP__
