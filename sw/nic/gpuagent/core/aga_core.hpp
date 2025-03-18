
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
/// this file deals with internal API context information
///
//----------------------------------------------------------------------------

#ifndef __AGA_CORE_CORE_HPP__
#define __AGA_CORE_CORE_HPP__

#include "nic/gpuagent/include/globals.hpp"

namespace aga {

enum {
    AGA_THREAD_ID_NONE = aga_thread_id_t::AGA_THREAD_ID_MIN,
    // API thread that handles the configuration operations
    AGA_THREAD_ID_API,
    // event monitoring thread
    AGA_THREAD_ID_EVENT_MONITOR,
    // GPU field watcher thread
    AGA_THREAD_ID_WATCHER,
};

}    // namesapce aga

#endif    // __AGA_CORE_CORE_HPP__
