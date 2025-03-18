
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
/// this file contains API params definition
///
//----------------------------------------------------------------------------

#ifndef __AGA_CORE_API_PARAMS_HPP__
#define __AGA_CORE_API_PARAMS_HPP__

#include "nic/gpuagent/core/api.h"
#include "nic/gpuagent/core/obj.h"
#include "nic/gpuagent/api/include/base.hpp"

namespace aga {

/// \brief    this class is used by core framework to stash all the  API
///           specific attributes while the object is being operated on
class api_params_base {
public:
    /// \brief    allocate an instance of this class
    static api_params_base *factory(void);

    /// \brief    destroy and free given instance of this class
    /// param[in] obj_id        object id/type
    /// param[in] api_op        API operation being performed
    /// param[in] api_params    api params instance to be freed
    static void destroy(obj_id_t obj_id, api_op_t api_op,
                        api_params_base *api_params);

    /// \brief    return reference to the object key
    /// param[in] obj_id    object id/type
    /// param[in] api_op    API operation being performed
    /// \return reference to the object's key
    virtual const aga_obj_key_t& obj_key(obj_id_t obj_id,
                                         api_op_t api_op) {
        static const aga_obj_key_t aga_obj_key_invalid = { 0 };

        return aga_obj_key_invalid;
    }
protected:
    /// \brief constructor
    api_params_base() {}

    /// \brief destructor
    virtual ~api_params_base() {}
};

}    // namespace aga

using aga::api_params_base;

#endif    // __AGA_CORE_API_PARAMS_HPP__
