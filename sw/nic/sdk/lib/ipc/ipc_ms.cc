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


#include "ipc.hpp"
#include "ipc_ms.hpp"

namespace sdk {
namespace ipc {

struct fd_watcher_t {
    handler_cb cb;
    const void *ctx;
};

static void
fd_cb_wrap (int fd, int, void *ctx)
{
    fd_watcher_t *watcher = (fd_watcher_t *)ctx;

    watcher->cb(fd, watcher->ctx);
}

static void *
fd_watch (int fd, handler_cb cb, const void *ipc_ctx, const void *infra_ctx)
{
    fd_watch_ms_cb ms_cb = (fd_watch_ms_cb) infra_ctx;
    fd_watcher_t *watcher = new fd_watcher_t();

    watcher->cb = cb;
    watcher->ctx = ipc_ctx;

    ms_cb(fd, fd_cb_wrap, watcher);
    
    return watcher;
}

static void
fd_unwatch (int fd, void *watcher, const void *infra_ctx)
{
    fd_watcher_t *w = (fd_watcher_t *)watcher;

    // We must call the MS unwatch function here

    delete w;
        
}

void
ipc_init_metaswitch (uint32_t client_id, fd_watch_ms_cb fd_watch_ms_cb)
{

    ipc_init_async(client_id, std::unique_ptr<infra_t>(new infra_t{
                    .fd_watch = fd_watch,
                    .fd_watch_ctx = (void *)fd_watch_ms_cb,
                    .fd_unwatch = fd_unwatch,
                    .fd_unwatch_ctx = NULL,
                    .timer_add = NULL,
                    .timer_add_ctx = NULL,
                    .timer_del = NULL,
                    .timer_del_ctx =  NULL,
                    }), true);
}
        
} // namespace ipc
} // namespace sdk
