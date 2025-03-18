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


#ifndef __SDK_IPC_MS_HPP__
#define __SDK_IPC_MS_HPP__

#include <stdint.h>

#include "ipc.hpp"

namespace sdk {
namespace ipc {

typedef void (*handler_ms_cb)(int fd, int, void *ctx);

typedef void (*fd_watch_ms_cb)(int fd, handler_ms_cb cb, void *cb_ctx);

extern void ipc_init_metaswitch(uint32_t client_id,
                                fd_watch_ms_cb fw_watch_ms_cb);

}
}

#endif
