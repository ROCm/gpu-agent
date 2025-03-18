
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
/// API handlers definitions that all objects are expected to implement
//----------------------------------------------------------------------------

#ifndef __AGA_API_HANDLERS_HPP__
#define __AGA_API_HANDLERS_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/core/api.h"
#include "nic/gpuagent/core/obj.h"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/core/api_base.hpp"
#include "nic/gpuagent/core/api_ctxt.hpp"
#include "nic/gpuagent/core/api_params.hpp"

namespace aga {

/// \brief    create callback on an object
/// \param[in] api_obj    new object instantiated to handle create operation
/// \param[in] api_params API params containing the context of the API call
/// \return    SDK_RET_OK or error code in case of failure
typedef sdk_ret_t (*aga_obj_create_cb_t)(api_base *api_obj,
                                         api_params_base *api_params);
/// \brief    delete callback on an object
/// \param[in] api_obj    object being deleted
/// \param[in] api_params API params containing the context of the API call
/// \return    SDK_RET_OK or error code in case of failure
typedef sdk_ret_t (*aga_obj_delete_cb_t)(api_base *api_obj,
                                         api_params_base *api_params);
/// \brief    update callback on an object
/// \param[in] api_obj    object being updated
/// \param[in] api_params API params containing the context of the API call
/// \return    SDK_RET_OK or error code in case of failure
typedef sdk_ret_t (*aga_obj_update_cb_t)(api_base *api_obj,
                                         api_params_base *api_params);

/// \brief    per object callbacks in one place
typedef struct aga_obj_api_cb_s {
    /// API handler to handle create operation
    aga_obj_create_cb_t create_cb;
    /// API handler to handle delete operation
    aga_obj_delete_cb_t delete_cb;
    /// API handler to handle update operation
    aga_obj_update_cb_t update_cb;
} aga_obj_api_cb_t;

// g_api_obj_cb needs to be implemented by the agent
extern aga_obj_api_cb_t g_api_obj_cb[AGA_OBJ_ID_MAX];

}    // namespace aga

#endif    // __AGA_API_HANDLERS_HPP__
