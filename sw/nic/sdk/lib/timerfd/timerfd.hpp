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


//------------------------------------------------------------------------------

//
// timerfd library is a wrapper around linux timerfd APIs
//------------------------------------------------------------------------------

#ifndef __SDK_TIMERFD_HPP__
#define __SDK_TIMERFD_HPP__

#include <sdk/base.hpp>

namespace sdk {
namespace lib {

typedef struct timerfd_info_s {
    int         timer_fd;
    uint64_t    usecs;
    uint64_t    missed_wakeups;
} timerfd_info_t;

void timerfd_init(timerfd_info_t *timerfd_info);
int timerfd_prepare(timerfd_info_t *timerfd_info);
int timerfd_wait(timerfd_info_t *timerfd_info, uint64_t *missed);

}    // namespace lib
}    // namespace sdk

using sdk::lib::timerfd_info_t;

#endif    // __SDK_TIMERFD_HPP__

