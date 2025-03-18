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
/// internal header file for periodic thread
///
//----------------------------------------------------------------------------

#ifndef __PERIODIC_INTERNAL_HPP__
#define __PERIODIC_INTERNAL_HPP__

#include "include/sdk/base.hpp"
#include "lib/twheel/twheel.hpp"

namespace sdk {
namespace lib {

// global timer wheel for periodic thread's use
extern sdk::lib::twheel *g_twheel;
// g_twheel_is_running is set to true when timer wheel starts
extern volatile bool g_twheel_is_running;
// g_periodic_thread_ready is set to true when thread starts running
extern volatile bool g_periodic_thread_ready;

}    // namespace lib
}    // namespace sdk

#endif    // __PERIODIC_INTERNAL_HPP__

