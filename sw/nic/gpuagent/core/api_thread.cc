
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
/// API thread specific helper functions
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/gpuagent/include/globals.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/core/aga_core.hpp"
#include "nic/gpuagent/core/api_msg.hpp"
#include "nic/gpuagent/core/api_msg.hpp"
#include "nic/gpuagent/core/api_thread.hpp"
#include "nic/gpuagent/core/api_engine.hpp"
#include "nic/gpuagent/core/state_base.hpp"

namespace event = sdk::event_thread;

namespace aga {

static void
api_thread_heartbeat_punch_cb (event::timer_t *timer)
{
    static sdk::lib::thread *curr_thread = sdk::lib::thread::current_thread();
    curr_thread->punch_heartbeat();
}

static void
api_thread_ipc_msg_cb (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    sdk_ret_t ret;
    api_msg_t *api_msg = *(api_msg_t **)msg->data();

    AGA_TRACE_DEBUG("Rcvd API IPC msg");
    // basic validation
    assert(likely(api_msg != NULL));
    assert(likely(api_msg->msg_id == AGA_IPC_MSG_ID_CFG));
    ret = api_msg_handle_cb(api_msg, msg);
    sdk::ipc::respond(msg, &ret, sizeof(ret));
}

void
api_thread_init_fn (void *ctxt)
{
    sdk_ret_t ret;
    static event::timer_t hb_timer;
    sdk::lib::thread *curr_thread = (sdk::lib::thread *)ctxt;
    state_base *state = (state_base *)curr_thread->data();

    api_engine_init(state);
    sdk::ipc::reg_request_handler(AGA_IPC_MSG_ID_CFG,
                                  api_thread_ipc_msg_cb, NULL);
    // start heartbeat timer
    event::timer_init(&hb_timer, api_thread_heartbeat_punch_cb,
                      AGA_HEARTBEAT_PUNCH_START_DELAY,
                      AGA_HEARTBEAT_PUNCH_TIMEOUT);
    event::timer_start(&hb_timer);
}

void
api_thread_exit_fn (void *ctxt)
{
}

void
api_thread_event_cb (void *msg, void *ctxt)
{
}

bool
is_api_thread_ready (void) {
    auto thr = sdk::lib::thread::find(AGA_THREAD_ID_API);
    return (thr && thr->ready());
}

sdk_ret_t
spawn_api_thread (state_base *state)
{
    sdk::event_thread::event_thread *new_thread;

    new_thread =
        sdk::event_thread::event_thread::factory(
            "api", AGA_THREAD_ID_API,
            sdk::lib::THREAD_ROLE_CONTROL,
            0x0,    // use all control cores
            api_thread_init_fn,
            api_thread_exit_fn,
            api_thread_event_cb,
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            THREAD_YIELD_ENABLE);
     SDK_ASSERT_TRACE_RETURN((new_thread != NULL), SDK_RET_ERR,
                             "API thread create failure");
     new_thread->set_data(state);
     new_thread->start(new_thread);
     return SDK_RET_OK;
}

}    // namespace aga
