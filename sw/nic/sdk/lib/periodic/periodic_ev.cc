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
/// periodic event thread implementation
///
//----------------------------------------------------------------------------

#include "include/sdk/globals.hpp"
#include "lib/event_thread/event_thread.hpp"
#include "lib/periodic/periodic_internal.hpp"
#include "lib/periodic/periodic.hpp"

namespace event = sdk::event_thread;

namespace sdk {
namespace lib {

#define EVENT_THREAD_WAIT_TIMEOUT   20      //  in seconds

sdk::event_thread::event_thread *g_periodic_ev_thread = NULL;

static void
twheel_timer_cb (event::timer_t *timer)
{
    // punch the heartbeat
    g_periodic_ev_thread->punch_heartbeat();
    // tick the timer wheel
    g_twheel->tick(TWHEEL_DEFAULT_SLICE_DURATION);
}

static void
periodic_ev_thread_exit (void *ctxt)
{
    g_twheel_is_running = false;
    // TODO:
    //sdk::lib::twheel::destroy(g_twheel);
}

static void
periodic_ev_event_handler (void *msg, void *ctxt)
{
}

static void
periodic_ev_thread_init (void *ctxt)
{
    static event::timer_t twheel_timer;
    sdk::lib::thread *curr_thread = (sdk::lib::thread *)ctxt;

    SDK_THREAD_INIT(ctxt);
    // create a timer wheel
    g_twheel = sdk::lib::twheel::factory(TWHEEL_DEFAULT_SLICE_DURATION,
                                         TWHEEL_DEFAULT_DURATION, true);
    if (g_twheel == NULL) {
        SDK_TRACE_ERR("Periodic thread failed to create timer wheel");
        return;
    }
    // start periodic timer to tick the timer wheel
    event::timer_init(&twheel_timer, twheel_timer_cb,
                      1.0, TWHEEL_DEFAULT_SLICE_DURATION_IN_SECS);
    event::timer_start(&twheel_timer);
    g_twheel_is_running = true;
    g_periodic_thread_ready = true;
    curr_thread->set_ready(true);
}

sdk_ret_t
spawn_periodic_event_thread (void *ctxt)
{
    g_periodic_ev_thread =
        sdk::event_thread::event_thread::factory(
            "periodic_ev", SDK_THREAD_ID_PERIODIC,
            sdk::lib::THREAD_ROLE_CONTROL, 0x0,    // use all control cores
            periodic_ev_thread_init, periodic_ev_thread_exit,
            periodic_ev_event_handler,
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            THREAD_YIELD_ENABLE);
    SDK_ASSERT_TRACE_RETURN((g_periodic_ev_thread != NULL), SDK_RET_ERR,
                            "periodic ev thread create failure");
    g_periodic_ev_thread->set_data(ctxt);
    g_periodic_ev_thread->start(g_periodic_ev_thread);
    return SDK_RET_OK;
}

void
periodic_ev_thread_stop (void)
{
    if (g_periodic_ev_thread != NULL) {
        SDK_TRACE_DEBUG("Stopping thread %s", g_periodic_ev_thread->name());
        g_periodic_ev_thread->stop();
    }
}

void
periodic_ev_thread_wait (void)
{
    if (g_periodic_ev_thread != NULL) {
        SDK_TRACE_DEBUG("Waiting thread %s to exit",
                        g_periodic_ev_thread->name());
        SDK_ASSERT(g_periodic_ev_thread->wait(EVENT_THREAD_WAIT_TIMEOUT)
                == SDK_RET_OK);
        // free the allocated thread
        sdk::lib::thread::destroy(g_periodic_ev_thread);
    }
}

}    // namespace lib
}    // namespace sdk
