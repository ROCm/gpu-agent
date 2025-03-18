
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
/// this file contains agent's API params definition
///
//----------------------------------------------------------------------------

#include "nic/gpuagent/core/api_params.hpp"
#include "nic/gpuagent/core/api_ctxt.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"
#include "nic/gpuagent/api/include/aga_task.hpp"
#include "nic/gpuagent/api/include/aga_gpu_watch.hpp"

#ifndef __INTERNAL_AGA_API_PARAMS_HPP__
#define __INTERNAL_AGA_API_PARAMS_HPP__

namespace aga {

/// \brief API specific parameters
class aga_api_params : public api_params_base {
public:
    /// \brief constructor
    aga_api_params() {};
    /// \brief destructor
    ~aga_api_params() {};
    /// \brief    return reference to the object key
    /// param[in] obj_id    object id/type
    /// param[in] api_op    API operation being performed
    /// \return reference to the object's key
    virtual const aga_obj_key_t& obj_key(obj_id_t obj_id,
                                         api_op_t api_op) override;

public:
    // all object specs
    union {
        aga_obj_key_t key;
        aga_gpu_spec_t gpu_spec;
        aga_task_spec_t task_spec;
        aga_gpu_watch_spec_t gpu_watch_spec;
    };
};

}    // namespage aga

using aga::aga_api_params;

#define AGA_API_PARAMS_FROM_API_CTXT(_api_ctxt_)                   \
            ((aga_api_params *)((_api_ctxt_)->api_params))
#define AGA_GPU_SPEC(_api_params_)                                 \
            &(((aga_api_params *)(_api_params_))->gpu_spec);
#define AGA_TASK_SPEC(_api_params_)                                \
            &(((aga_api_params *)(_api_params_))->task_spec);
#define AGA_GPU_WATCH_SPEC(_api_params_)                           \
            &(((aga_api_params *)(_api_params_))->gpu_watch_spec);

#endif    // __INTERNAL_AGA_API_PARAMS_HPP__
