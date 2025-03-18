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


#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include "include/sdk/timerfd.hpp"
#include "include/sdk/timestamp.hpp"
#include "lib/logger/logger.h"

namespace sdk {
namespace lib {

//------------------------------------------------------------------------------
// initiaize information about a given timer fd
//------------------------------------------------------------------------------
void
timerfd_init (timerfd_info_t *pinfo)
{
    pinfo->timer_fd = -1;
    pinfo->usecs = 0;
    pinfo->missed_wakeups = 0;
}

//------------------------------------------------------------------------------
// create and initialize a timer fd, this fd can then be used
// in poll/select system calls eventually
//------------------------------------------------------------------------------
int
timerfd_prepare (timerfd_info_t *pinfo)
{
    int                  fd;
    struct itimerspec    itspec;
    timespec_t           tspec;

    // create timer fd
    fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (fd == -1) {
        return fd;
    }
    pinfo->missed_wakeups = 0;
    pinfo->timer_fd = fd;

    // initialize the timeout
    sdk::timestamp_from_nsecs(&tspec, pinfo->usecs * TIME_NSECS_PER_USEC);
    itspec.it_interval = tspec;
    itspec.it_value = tspec;
    return timerfd_settime(fd, 0, &itspec, NULL);
}

//------------------------------------------------------------------------------
// wait on a given timer fd and return number of missed wakeups, if any
// TODO: in future, if we have multiple of these, we can use select()
//------------------------------------------------------------------------------
int
timerfd_wait (timerfd_info_t *pinfo, uint64_t *missed)
{
    int         rv;

    // wait for next timer event, and warn any missed events
    *missed = 0;
    rv = read(pinfo->timer_fd, missed, sizeof(*missed));
    if (rv == -1) {
        return -1;
    }
    if (*missed > 1) {
        SDK_TRACE_VERBOSE("Periodic thread missed %" PRIu64 " wakeups", *missed);
    }
    pinfo->missed_wakeups += *missed;
    return 0;
}

}    // namespace lib
}    // namespace sdk
