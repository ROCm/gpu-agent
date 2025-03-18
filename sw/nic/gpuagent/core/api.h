
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
/// generic API operation definitions
//----------------------------------------------------------------------------

#ifndef __AGA_CORE_API_H__
#define __AGA_CORE_API_H__

#include "nic/sdk/include/sdk/base.hpp"

// TODO:
// explore adding namespace here and in obj.h and get rid of aga_ prefix for
// enums

#ifdef __cplusplus
extern "C" {
#endif

/// \brief    max. size of API object name
#define AGA_OBJ_NAME_SIZE        32

/// \brief    API operations
typedef enum api_op_e {
    API_OP_NONE,       ///< none
    API_OP_CREATE,     ///< create
    API_OP_DELETE,     ///< delete
    API_OP_UPDATE,     ///< update
    API_OP_INVALID,    ///< invalid
} api_op_t;

/// \brief    helper API to validate the API call
/// \param[in] op      API operation
/// \param[in] key     key of the object
/// \param[in] spec    config specification of the object
/// \return    SDK_RET_OK or error code in case of failure
static inline sdk_ret_t
obj_api_validate (api_op_t op, void *key, void *spec)
{
    if ((op == API_OP_DELETE) && key) {
        return SDK_RET_OK;
    }

    if (((op == API_OP_CREATE) || (op == API_OP_UPDATE)) && spec) {
        return SDK_RET_OK;
    }
    return SDK_RET_INVALID_ARG;
}

#ifdef __cplusplus
}
#endif

#endif    // __AGA_CORE_API_H__
