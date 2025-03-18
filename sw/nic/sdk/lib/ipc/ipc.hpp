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


#ifndef __IPC_H__
#define __IPC_H__

#include <memory>
#include <mutex>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <vector>

#include <stddef.h>
#include <stdint.h>

#define IPC_MAX_BROADCAST_ID 255
#define IPC_MAX_CLIENT_ID 255

namespace sdk {
namespace ipc {

typedef enum ipc_msg_type {
    DIRECT    = 0,
    BROADCAST = 1,
} ipc_msg_type_t;

class ipc_msg {
public:
    virtual uint32_t code(void) = 0;
    /// \brief get a pointer to the data of the message
    virtual void *data(void) = 0;
    /// \brief get the size of the data payload
    virtual size_t length(void) = 0;
    /// \brief get the type of the message (DIRECT or BROADCAST)
    virtual ipc_msg_type_t type(void) = 0;
    /// \brief get the sender of the message
    virtual uint32_t sender(void) = 0;
    /// \brief get a debug string for the message
    virtual std::string debug(void) = 0;
};
typedef std::shared_ptr<struct ipc_msg> ipc_msg_ptr;

///
/// Callbacks
///

typedef void (*request_cb)(ipc_msg_ptr msg, const void *ctx);

typedef void (*response_cb)(ipc_msg_ptr msg, const void *request_cookie,
                            const void *ctx);

typedef void (*response_oneshot_cb)(ipc_msg_ptr msg,
                                    const void *request_cookie);

typedef void (*subscription_cb)(ipc_msg_ptr msg, const void *ctx);

///
/// Init
///

typedef void (*handler_cb)(int fd, const void *ctx);
typedef void *(*fd_watch_fn)(int fd, handler_cb cb, const void *ipc_ctx,
                             const void *infra_ctx);
typedef void (*fd_unwatch_fn)(int fd, void *watcher, const void *infra_ctx);
typedef void (*timer_callback)(void *timer, const void *ipc_ctx);
typedef void *(*timer_add_fn)(timer_callback callback, const void *ipc_ctx,
                              double timeout, const void *infra_ctx);
typedef void (*timer_del_fn)(void *timer, const void *infra_ctx);

typedef struct infra_t_ {
    fd_watch_fn fd_watch;
    const void *fd_watch_ctx;
    fd_unwatch_fn fd_unwatch;
    const void *fd_unwatch_ctx;
    timer_add_fn timer_add;
    const void *timer_add_ctx;
    timer_del_fn timer_del;
    const void *timer_del_ctx;
} infra_t;
typedef std::unique_ptr<infra_t> infra_ptr;

extern void ipc_init_async(uint32_t client_id, infra_ptr infra,
                           bool associate_thread_name = false);

extern void ipc_init_sync(uint32_t client_id);

extern void ipc_init_sync(uint32_t client_id, infra_ptr infra);

extern uint32_t ipc_id();

///
/// Sending
///

extern ipc_msg_ptr blocked_request(uint32_t recipient, uint32_t msg_code,
                                   const void *data, size_t legnth,
                                   double timeout = 0.0);

extern void request(uint32_t recipient, uint32_t msg_code, const void *data,
                    size_t length, const void *cookie, double timeout = 0.0);

extern void request(uint32_t recipient, uint32_t msg_code, const void *data,
                    size_t length, response_oneshot_cb response_cb,
                    const void *cookie, double timeout = 0.0);

extern ipc_msg_ptr zrequest(uint32_t card_id, uint32_t endpoint, uint32_t opcode,
        void *data, size_t length);

// !!!FIXME WARNING!!!
// All callers of this function are currently using a local variable as
// cookie which, if the underlying IPC channel is configured for async,
// can potentially cause memory corruption.
// 
// All such calls to FIXME_request() must be converted to either
// blocked_request() to force a blocking behavior, or to request()
// using a non-local or NULL cookie context.
extern void FIXME_request(uint32_t recipient, uint32_t msg_code,
                          const void *data, size_t legnth,
                          response_oneshot_cb cb, const void *cookie,
                          double timeout = 0.0);

extern void broadcast(uint32_t msg_code, const void *data, size_t data_length);

extern void respond(ipc_msg_ptr msg, const void *data, size_t data_length);

extern void broadcast_handled(ipc_msg_ptr msg);

///
/// Receiving
///

extern void reg_request_handler(uint32_t msg_code, request_cb callback,
                                const void *ctx);


extern void reg_response_handler(uint32_t msg_code, response_cb callback,
                                 const void *ctx);

extern void subscribe(uint32_t msg_code, subscription_cb callback,
                      const void *ctx);

// This is to be used in sync cases when we want to receive requests or
// subscription messages
extern void receive(void);

///
/// Configuration
///

extern void set_drip_feeding(bool enabled);

extern void set_debug_delays(bool enabled);

} // namespace ipc
} // namespace sdk

#endif // __IPC_H__
