
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
/// \file    aga_state.hpp
///
/// This file captures all the state maintained in s/w
//----------------------------------------------------------------------------

#ifndef __AGA_STATE_HPP__
#define __AGA_STATE_HPP__

#include "nic/gpuagent/core/state_base.hpp"
#include "nic/gpuagent/api/gpu_state.hpp"
#include "nic/gpuagent/api/task_state.hpp"
#include "nic/gpuagent/api/gpu_watch_state.hpp"
#include "nic/gpuagent/api/internal/aga_api_params.hpp"

namespace aga {

/// \brief s/w state/db types
#define AGA_STATE(ENTRY)                                                 \
    ENTRY(AGA_STATE_MIN,                       0,  "min")                \
    ENTRY(AGA_STATE_GPU,                       1,  "gpu")                \
    ENTRY(AGA_STATE_TASK,                      2,  "task")               \
    ENTRY(AGA_STATE_GPU_WATCH,                 3,  "gpu_watch")          \
    ENTRY(AGA_STATE_MAX,                       4,  "max")

SDK_DEFINE_ENUM(aga_state_t, AGA_STATE)
SDK_DEFINE_ENUM_TO_STR(aga_state_t, AGA_STATE)
#undef AGA_STATE

/// \brief ctxt passed as argument to state_walk cb
typedef struct state_walk_ctxt_s {
    /// name of the state object
    std::string name;
    /// instance of state object
    state_base *state;
} state_walk_ctxt_t;

/// \defgroup AGA_STATE - internal state of GPU agent
/// \ingroup AGA
/// \@{

// all the agent's internal state in one place for easy access
class aga_state : public state_base {
public:
    /// \brief constructor
    aga_state();

    /// \brief destructor
    ~aga_state();

    /// \brief    initialization routine
    /// \return SDK_RET_OK or error code in case of failure
    sdk_ret_t init(void);

    /// \brief    destroy given instance of state object
    /// \param[in] ps    pointer to the state instance
    static void destroy(aga_state *ps);

    /// \brief return the gpu object database instance
    /// \return    gpu object database instance
    gpu_state *gpu_db(void) {
        return (gpu_state *)state_[AGA_STATE_GPU];
    }

    /// \brief return the task object database instance
    /// \return    task object database instance
    task_state *task_db(void) {
        return (task_state *)state_[AGA_STATE_TASK];
    }

    /// \brief return the gpu watch object database instance
    /// \return    gpu watch object database instance
    gpu_watch_state *gpu_watch_db(void) {
        return (gpu_watch_state *)state_[AGA_STATE_GPU_WATCH];
    }

    /// \brief    allocate an instance of API params
    /// \return    pointer to API params instance or NULL
    api_params_base *aga_api_params_alloc(void) {
        return (api_params_base *)calloc(1, sizeof(aga_api_params));
    }

    /// \brief    free the given instance of API params object
    /// \param[in] api_params    API params object to be freed
    void aga_api_params_free(api_params_base *api_params) {
        free(api_params);
    }

    /// \brief    walk all the internal databases and invokve the given callback
    /// \return    SDK_RET_OK or error code in case of failure
    sdk_ret_t walk(state_walk_cb_t walk_cb, void *ctxt) override;

private:
    /// \brief    initialize all internal databases
    void store_init_(void);
private:
    /// all object databases
    state_base *state_[AGA_STATE_MAX];

};
/// \brief    global (singleton) GPU state instance
extern aga_state g_aga_state;
/// \@}

}    // namespace aga

static inline gpu_state *
gpu_db (void)
{
    return aga::g_aga_state.gpu_db();
}

static inline task_state *
task_db (void)
{
    return aga::g_aga_state.task_db();
}

static inline gpu_watch_state *
gpu_watch_db (void)
{
    return aga::g_aga_state.gpu_watch_db();
}

#endif    // __AGA_STATE_HPP__
