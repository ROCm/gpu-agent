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


#include <assert.h>
#include <map>
#include <memory>
#include <queue>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/eventfd.h>
#include <unistd.h>
#include <vector>
#include <ev.h>

#include "include/sdk/base.hpp"
#include "ipc.hpp"
#include "ipc_internal.hpp"
#include "subscribers.hpp"
#include "zmq_ipc.hpp"

namespace sdk {
namespace ipc {

typedef struct ev_watcher_ {
    ev_io ev;
    handler_cb cb;
    const void *ctx;
} ev_watcher_t;

struct timer_t {
    ev_timer ev_watcher;
    ipc::timer_callback cb;
    const void *ctx;
};

static void
ev_watch_cb_wrap (struct ev_loop *loop, ev_io *w, int revents)
{
    ev_watcher_t *watcher = (ev_watcher_t *)w;

    watcher->cb(watcher->ev.fd, watcher->ctx);
}
    
static void *
ev_watch_cb (int fd, handler_cb cb, const void *set_ctx, const void *ctx)
{
    ev_watcher_t *watcher = (ev_watcher_t *)malloc(sizeof(*watcher));

    watcher->cb = cb;
    watcher->ctx = set_ctx;
    
    ev_io_init((ev_io *)watcher, ev_watch_cb_wrap, fd, EV_READ);
    ev_io_start(EV_DEFAULT, (ev_io *)watcher);

    return watcher;
}

static void
fd_unwatch (int fd, void *watcher, const void *infra_ctx)
{
    ev_watcher_t *w = (ev_watcher_t *)watcher;
    
    ev_io_stop(EV_DEFAULT, (ev_io *)w);

    delete w;
}

static void
timer_cb_wrap (struct ev_loop *loop, ev_timer *w, int revents)
{
    timer_t *timer = (timer_t *)w;

    timer->cb(timer, timer->ctx);
}

static void *
timer_add (timer_callback ipc_cb, const void *ipc_ctx, double timeout,
           const void *infra_ctx)
{
    timer_t *timer = new timer_t;

    timer->cb = ipc_cb;
    timer->ctx = ipc_ctx;

    ev_timer_init((ev_timer *)timer, timer_cb_wrap, timeout, 0.0);
    ev_timer_start(EV_DEFAULT, (ev_timer *)timer);
    
    return timer;
}

static void
timer_del (void *timer, const void *infra_ctx)
{
    timer_t *t = (timer_t *)timer;

    ev_timer_stop(EV_DEFAULT, (ev_timer *)timer);

    delete t;
}

void
ipc_init_ev_default (uint32_t client_id)
{
    ipc_init_async(client_id, std::unique_ptr<infra_t>(new infra_t{
                    .fd_watch = ev_watch_cb,
                    .fd_watch_ctx = NULL,
                    .fd_unwatch = fd_unwatch,
                    .fd_unwatch_ctx = NULL,
                    .timer_add = timer_add,
                    .timer_add_ctx = NULL,
                    .timer_del = timer_del,
                    .timer_del_ctx =  NULL,
                    }));
}

}
}
