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


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "lib/thread/thread.hpp"
#include "include/sdk/timerfd.hpp"
#include "lib/periodic/periodic_internal.hpp"
#include "lib/periodic/periodic.hpp"

//------------------------------------------------------------------------------
//          ALTERNATE DESIGN TO CONSIDER LATER
//
// timer_fd can be made to sit inside twheel class and
// timerfd_init(), timerfd_prepare() can be inside the
// init() method of that class, timerfd() API on twheel
// can then give fd, so app can do select or timerfd_wait()
// this way most details of timerfd can be hidden inside twheel class
//
//------------------------------------------------------------------------------

namespace sdk {
namespace lib {

#define BATCH_SLICE_SIZE 10

// global timer wheel for periodic thread's use
sdk::lib::twheel *g_twheel = NULL;
volatile bool g_twheel_is_running = false;
volatile bool g_periodic_thread_ready = false;

// thread local variables
thread_local timerfd_info_t timerfd_info;

//------------------------------------------------------------------------------
// periodic thread starting point
//------------------------------------------------------------------------------
void *
periodic_thread_init (void *ctxt)
{
    // opting for graceful termination
    SDK_THREAD_INIT(ctxt);

    // create a timer wheel
    g_twheel = sdk::lib::twheel::factory(TWHEEL_DEFAULT_SLICE_DURATION,
                                         TWHEEL_DEFAULT_DURATION, true);
    if (g_twheel == NULL) {
        SDK_TRACE_ERR("Periodic thread failed to create timer wheel");
        return NULL;
    }

    // prepare the timer fd(s)
    sdk::lib::timerfd_init(&timerfd_info);
    timerfd_info.usecs = TWHEEL_DEFAULT_SLICE_DURATION * TIME_USECS_PER_MSEC;
    if (sdk::lib::timerfd_prepare(&timerfd_info) < 0) {
        SDK_TRACE_ERR("Periodic thread failed to intiialize timerfd");
        return NULL;
    }
    g_twheel_is_running = true;

    return g_twheel;
}

//------------------------------------------------------------------------------
// periodic thread main loop
//------------------------------------------------------------------------------
void *
periodic_thread_run (void *ctxt)
{
    uint64_t            missed;
    sdk::lib::thread    *curr_thread = (sdk::lib::thread *)ctxt;

    pthread_cleanup_push(periodic_thread_cleanup, NULL);
    // mark periodic thread as ready
    g_periodic_thread_ready = true;
    curr_thread->set_ready(true);
    while (TRUE) {
        // wait for timer to fire
        if (sdk::lib::timerfd_wait(&timerfd_info, &missed) < 0) {
            // timerfd_wait can fail if the read system call is interrupted
            // which sets errno to EINTR
            if (errno == EINTR) {
                continue;
            }
            SDK_TRACE_ERR("Periodic thread failed to wait on timer");
            break;
        }

        // drive the timer wheel if enough time elapsed
        while (missed) {
            g_twheel->tick(((missed > BATCH_SLICE_SIZE) ?
                            BATCH_SLICE_SIZE : missed) *
                           TWHEEL_DEFAULT_SLICE_DURATION);
            curr_thread->punch_heartbeat();
            missed -= (missed > BATCH_SLICE_SIZE) ?
                BATCH_SLICE_SIZE : missed;
        }
    }
    g_twheel_is_running = false;
    pthread_cleanup_pop(1);
    SDK_TRACE_ERR("Periodic thread exiting !!!");

    return NULL;
}

//------------------------------------------------------------------------------
// periodic thread cleanup
//------------------------------------------------------------------------------
void
periodic_thread_cleanup (void *arg)
{
    // destroy timer wheel
    // As g_twheel is accessed across threads, cleanup should be
    // done only after all dependent threads exits. TODO
    //sdk::lib::twheel::destroy(g_twheel);
}

//------------------------------------------------------------------------------
// returns true only if thread timer wheel is running
//------------------------------------------------------------------------------
bool
periodic_thread_is_running (void)
{
    return g_twheel_is_running;
}

//------------------------------------------------------------------------------
// returns true only if periodic thread is fully initialized
//------------------------------------------------------------------------------
bool
periodic_thread_is_ready (void)
{
    return g_periodic_thread_ready;
}

//------------------------------------------------------------------------------
// API invoked by other threads to trigger cb after timeout
// Returns the timer entry used to update/delete the timer
//------------------------------------------------------------------------------
void *
timer_schedule (uint32_t timer_id, uint64_t timeout, void *ctxt,
                sdk::lib::twheel_cb_t cb, bool periodic, uint64_t initial_delay)
{
    if (g_twheel) {
        return g_twheel->add_timer(timer_id, timeout, ctxt, cb,
                                   periodic, initial_delay);
    }
    return NULL;
}

//------------------------------------------------------------------------------
// API invoked by other threads to get timeout remaining for the timer.
//------------------------------------------------------------------------------
uint64_t
get_timeout_remaining (void *timer)
{
    if (g_twheel) {
        return g_twheel->get_timeout_remaining(timer);
    }
    return 0;
}

//------------------------------------------------------------------------------
// API invoked by other threads to delete the scheduled timer
//------------------------------------------------------------------------------
void *
timer_delete (void *timer)
{
    if (g_twheel) {
        return g_twheel->del_timer(timer);
    }
    return NULL;
}

//------------------------------------------------------------------------------
// API invoked by other threads to update the scheduled timer
//------------------------------------------------------------------------------
void *
timer_update_ctxt (void *timer, void *ctxt)
{
    if (g_twheel) {
        return g_twheel->upd_timer_ctxt(timer, ctxt);
    }
    return NULL;
}

void *
timer_update (void *timer, uint64_t timeout, bool periodic, void *ctxt)
{
    if (g_twheel) {
        return g_twheel->upd_timer(timer, timeout, periodic, ctxt);
    }
    return NULL;
}

}    // namespace lib
}    // namespace sdk
