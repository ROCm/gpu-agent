
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
/// task state handling
///
//----------------------------------------------------------------------------

#ifndef __AGA_TASK_STATE_HPP__
#define __AGA_TASK_STATE_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/mem.hpp"
#include "nic/gpuagent/api/task.hpp"
#include "nic/gpuagent/core/state_base.hpp"
#include "nic/gpuagent/api/aga_state.hpp"

namespace aga {

/// \defgroup AGA_TASK_STATE - task state handling
/// \ingroup AGA
/// @{

/// \brief    state maintained for tasks
class task_state : public state_base {
public:
    /// \brief    constructor
    task_state() {}

    /// \brief    destructor
    ~task_state() {}

    /// \brief    allocate memory required for a task
    /// \return pointer to the allocated task, NULL if no memory
    task *alloc(void) {
        return (task *)SDK_CALLOC(AGA_MEM_ALLOC_TASK, sizeof(task));
    }

    /// \brief      free the given task instance
    /// \param[in]  mapping   pointer to the allocated task instance
    void free(task *task) {
        SDK_FREE(AGA_MEM_ALLOC_TASK, task);
    }

    /// \brief     lookup a task in database given the key
    /// \param[in] key key for the task object
    /// \return    pointer to the task instance found or NULL
    task *find(aga_obj_key_t *key) const {
        return NULL;
    }

    /// \brief API to walk all the db elements
    /// \param[in] walk_cb    callback to be invoked for every node
    /// \param[in] ctxt       opaque context passed back to the callback
    /// \return   SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t walk(state_walk_cb_t walk_cb, void *ctxt) override {
        return SDK_RET_OK;
    }
};

/// \@}

}    // namespace aga

using aga::task_state;

#endif    // __AGA_TASK_STATE_HPP__
