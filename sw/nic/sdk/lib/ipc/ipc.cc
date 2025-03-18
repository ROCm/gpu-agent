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
#include <mutex>
#include <queue>
#include <set>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/eventfd.h>
#include <time.h>
#include <unistd.h>
#include <vector>

#include "ai.hpp"
#include "include/sdk/base.hpp"
#include "lib/logger/logger.h"
#include "ipc.hpp"
#include "ipc_internal.hpp"
#include "subscribers.hpp"
#include "zmq_ipc.hpp"


namespace sdk {
namespace ipc {

const double MAX_CALLBACK_DURATION = 0.25;

typedef struct req_callback_ {
    request_cb cb;
    const void *ctx;
} req_callback_t;

typedef struct rsp_callback_ {
    response_cb cb;
    const void *ctx;
} rsp_callback_t;

typedef struct sub_callback_ {
    subscription_cb cb;
    const void *ctx;
} sub_callback_t;

class ipc_service_async;
typedef struct client_receive_cb_ctx_ {
    ipc_service_async *svc;
    uint64_t recipient;
} client_receive_cb_ctx_t;

class ipc_service {
public:
    ~ipc_service();
    ipc_service();
    ipc_service(uint32_t client_id);
    uint32_t get_id(void) { return get_id_(); }
    virtual void request(uint32_t recipient, uint32_t msg_code,
                         const void *data, size_t data_length,
                         response_oneshot_cb cb, const void *cookie,
                         double timeout) = 0;
    virtual void client_receive(uint32_t recipient) = 0;
    virtual ipc_msg_ptr blocked_request(uint32_t recipient, uint32_t msg_code,
                                        const void *data, size_t length,
                                        double timeout) = 0;
    virtual ipc_msg_ptr zrequest(uint32_t card_id, uint32_t endpoint, uint32_t opcode,
            void *data, size_t length);
    void respond(ipc_msg_ptr msg, const void *data, size_t data_length);
    void broadcast(uint32_t msg_code, const void *data, size_t data_length);
    void broadcast_handled(ipc_msg_ptr msg);
    void reg_request_handler(uint32_t msg_code, request_cb callback,
                             const void *ctx);
    void reg_response_handler(uint32_t msg_code, response_cb callback,
                              const void *ctx);
    void subscribe(uint32_t msg_code, subscription_cb callback,
                   const void *ctx);
    void receive(void);
    void server_receive(void);
    void eventfd_receive(void);
    void set_drip_feeding(bool drip_feed);
protected:
    virtual zmq_ipc_client_ptr new_client_(uint32_t recipient) = 0;
    uint32_t get_id_(void);
    void set_server_(zmq_ipc_server_ptr ipc_server);
    void handle_response_(uint32_t msg_code, ipc_msg_ptr msg,
                          response_oneshot_cb cb, const void *cookie);
    zmq_ipc_client_ptr get_client_(uint32_t recipient);
    bool should_serialize_(void);
    void serialize_(ipc_msg_ptr msg);
    void deserialize_(void);
    void deliver_direct_(ipc_msg_ptr msg);
    void deliver_broadcast_(ipc_msg_ptr msg);
    void deliver_(ipc_msg_ptr msg);
    int get_eventfd_(void);
    // Another eventfd used to poll the zmq in the cases where the zmq fd
    // will not trigger
    int receive_eventfd_;
    infra_ptr infra_;
private:
    uint32_t id_;
    zmq_ipc_server_ptr ipc_server_;
    zmq_ipc_client_ptr ipc_clients_[IPC_MAX_CLIENT_ID + 1];
    std::map<uint32_t, ai_client_ptr> ai_clients_;
    std::map<uint32_t, req_callback_t> req_cbs_;
    std::map<uint32_t, rsp_callback_t> rsp_cbs_;
    std::map<uint32_t, sub_callback_t> sub_cbs_;
    // the fields below are used for serialized delivery
    bool serializing_enabled_;
    bool message_in_flight_ = false;
    // hold queues are used to serialize messages
    std::queue<ipc_msg_ptr> hold_queue_;
    // deliver queue is used to actually deliver held messages to
    // the client at the next tick
    std::queue<ipc_msg_ptr> delivery_queue_;
    // eventfd is used to notify the client that things are ready in
    // delivery_queue_
    int eventfd_;
};
typedef std::shared_ptr<ipc_service> ipc_service_ptr;

class ipc_service_sync : public ipc_service {
public:
    ipc_service_sync();
    ipc_service_sync(uint32_t client_id);
    ipc_service_sync(uint32_t client_id, infra_ptr infra);
    virtual void request(uint32_t recipient, uint32_t msg_code,
                         const void *data, size_t data_length,
                         response_oneshot_cb cb, const void *cookie,
                         double timeout) override;
    virtual void client_receive(uint32_t recipient) override;
    virtual ipc_msg_ptr blocked_request(uint32_t recipient, uint32_t msg_code,
                                        const void *data, size_t length,
                                        double timeout) override;
protected:
    virtual zmq_ipc_client_ptr new_client_(uint32_t recipient) override;
};
typedef std::shared_ptr<ipc_service_sync> ipc_service_sync_ptr;


struct message_ctx_t {
    class ipc_service_async *service;
    response_oneshot_cb oneshot_cb;
    const void *cookie;
    void *timer;
    uint64_t id;
    uint32_t code;
};

class ipc_service_async : public ipc_service {

public:
    ipc_service_async(uint32_t client_id, infra_ptr infra);
    virtual void request(uint32_t recipient, uint32_t msg_code,
                         const void *data, size_t data_length,
                         response_oneshot_cb cb, const void *cookie,
                         double timeout) override;
    virtual void client_receive(uint32_t recipient) override;
    virtual ipc_msg_ptr blocked_request(uint32_t recipient, uint32_t msg_code,
                                        const void *data, size_t length,
                                        double timeout) override;
    void timer_expired(uint64_t msg_ctx_id);

protected:
    virtual zmq_ipc_client_ptr new_client_(uint32_t recipient) override;

private:
    client_receive_cb_ctx_t client_rx_cb_ctx_[IPC_MAX_CLIENT_ID + 1];
    int ipc_client_eventfds_[IPC_MAX_CLIENT_ID + 1];
    std::map<uint64_t, message_ctx_t *> msg_contexts_;
    uint64_t msg_context_id = 0;
    std::mutex msg_contexts_lock_;
};
typedef std::shared_ptr<ipc_service_async> ipc_service_async_ptr;

thread_local ipc_service_ptr t_ipc_service = nullptr;

static std::mutex g_thread_handles_lock;
static std::map<std::string, ipc_service_ptr> g_thread_handles;

const int THREAD_NAME_BUFFER_SZ = 64;

static std::string
thread_name (void)
{
    int rc;
    char buffer[THREAD_NAME_BUFFER_SZ];

    rc = pthread_getname_np(pthread_self(), buffer, THREAD_NAME_BUFFER_SZ);
    assert(rc == 0);

    return buffer;
}

static void
server_receive (int fd, const void *ctx)
{
    ipc_service *svc = (ipc_service *)ctx;

    svc->server_receive();
}

static void
eventfd_receive (int fd, const void *ctx)
{
    ipc_service *svc = (ipc_service *)ctx;

    svc->eventfd_receive();
}

static void
client_receive (int fd, const void *ctx)
{
    client_receive_cb_ctx_t *c_rx_cb_ctx = (client_receive_cb_ctx_t *)ctx;


    c_rx_cb_ctx->svc->client_receive(c_rx_cb_ctx->recipient);
}

static void
timer_handler (void *timer, const void *ipc_ctx)
{
    message_ctx_t *msg_ctx = (message_ctx_t *)ipc_ctx;

    msg_ctx->service->timer_expired(msg_ctx->id);
}

ipc_service::~ipc_service() {
    if (this->eventfd_ != -1) {
        close(this->eventfd_);
    }
    if (this->receive_eventfd_ != -1) {
        close(this->receive_eventfd_);
    }
}

ipc_service::ipc_service() : ipc_service(IPC_MAX_CLIENT_ID + 1) {};

ipc_service::ipc_service(uint32_t id) {
    this->id_ = id;
    this->ipc_server_ = nullptr;
    this->eventfd_ = -1;
    this->receive_eventfd_ = -1;
    this->serializing_enabled_ = false;

    for (int i = 0; i < IPC_MAX_CLIENT_ID + 1; i++) {
        this->ipc_clients_[i] = nullptr;
    }
}

uint32_t
ipc_service::get_id_(void) {
    return this->id_;
}

void
ipc_service::set_server_(zmq_ipc_server_ptr server) {
    assert(this->ipc_server_ == nullptr);
    this->ipc_server_ = server;

    this->eventfd_ = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    assert(this->eventfd_ != -1);

    this->receive_eventfd_ = eventfd(0, EFD_CLOEXEC| EFD_NONBLOCK);
    assert(this->receive_eventfd_ != 1);
}

zmq_ipc_client_ptr
ipc_service::get_client_(uint32_t recipient) {
    zmq_ipc_client_ptr client;

    client = this->ipc_clients_[recipient];
    if (client == nullptr) {
        client = this->new_client_(recipient);

        this->ipc_clients_[recipient] = client;
    }
    return client;
}

void
ipc_service::respond(ipc_msg_ptr msg, const void *data, size_t data_length) {
    uint64_t buffer = 1;
    assert(msg != nullptr);
    this->ipc_server_->reply(msg, data, data_length);

    // Queue a check for the receive socket
    write(this->receive_eventfd_, &buffer, sizeof(buffer));

    this->message_in_flight_ = false;

    SDK_TRACE_VERBOSE("will deserialize because we responded to direct msg - %s",
                      msg->debug().c_str());
    this->deserialize_();
}

void
ipc_service::broadcast_handled(ipc_msg_ptr msg) {
    assert(msg != nullptr);
    this->message_in_flight_ = false;

    SDK_TRACE_VERBOSE("will deserialize because we responded to broadcast msg - %s",
                      msg->debug().c_str());
    this->deserialize_();
}

void
ipc_service::handle_response_(uint32_t msg_code, ipc_msg_ptr msg,
                              response_oneshot_cb cb, const void *cookie) {
    if (cb) {
        clock_t start = clock();
        double cpu_time_used;
        cb(msg, cookie);
        cpu_time_used = ((double) (clock() - start)) / CLOCKS_PER_SEC;
        if (cpu_time_used > MAX_CALLBACK_DURATION) {
            SDK_TRACE_DEBUG("response handling for %d took %f seconds",
                            msg_code, cpu_time_used);
        }
    } else {
        assert(this->rsp_cbs_.count(msg_code) > 0);
        rsp_callback_t rsp_cb = this->rsp_cbs_[msg_code];
        if (rsp_cb.cb != NULL) {
            clock_t start = clock();
            double cpu_time_used;
            rsp_cb.cb(msg, cookie, rsp_cb.ctx);
            cpu_time_used = ((double) (clock() - start)) / CLOCKS_PER_SEC;
            if (cpu_time_used > MAX_CALLBACK_DURATION) {
                SDK_TRACE_DEBUG("response handling for %d took %f seconds",
                                msg_code, cpu_time_used);
            }
        }
    }
}

ipc_msg_ptr
ipc_service::zrequest(uint32_t card_id, uint32_t endpoint, uint32_t opcode,
        void *data, size_t length) {
    if (this->ai_clients_.count(card_id) == 0) {
        this->ai_clients_.insert(std::pair<uint32_t, std::shared_ptr<sdk::ipc::ai_client>>(card_id, ai_client::create(card_id)));
    } else {
        fflush(stdout);
    }
    ai_client_ptr aic = this->ai_clients_[card_id];
    return aic->request(endpoint, opcode, data, length);
}

ipc_service_sync::ipc_service_sync() {
}

ipc_service_sync::ipc_service_sync(uint32_t client_id)
    : ipc_service(client_id) {
}

ipc_service_sync::ipc_service_sync(uint32_t client_id,
                                   infra_ptr infra)
    : ipc_service(client_id) {

    assert(infra != nullptr);

    zmq_ipc_server_ptr server = std::make_shared<zmq_ipc_server>(
        this->get_id_());

    this->infra_ = std::move(infra);

    this->set_server_(server);
    this->infra_->fd_watch(server->fd(),
                           sdk::ipc::server_receive,
                           (void *)this,
                           this->infra_->fd_watch_ctx);
    this->infra_->fd_watch(this->receive_eventfd_,
                           sdk::ipc::server_receive,
                           (void *)this,
                           this->infra_->fd_watch_ctx);
    
    // Queue a check for the receive socket
    uint64_t buffer = 1;
    SDK_TRACE_DEBUG("sync server asking for server pending message check"); 
    write(this->receive_eventfd_, &buffer, sizeof(buffer));
}

ipc_msg_ptr
ipc_service_sync::blocked_request(uint32_t recipient, uint32_t msg_code,
                                  const void *data, size_t data_length,
                                  double timeout) {

    zmq_ipc_client_sync_ptr client =
        std::dynamic_pointer_cast<zmq_ipc_client_sync>(
            this->get_client_(recipient));

    ipc_msg_ptr msg = client->send_recv(msg_code, data, data_length, timeout);

    return msg;
}

void
ipc_service_sync::request(uint32_t recipient, uint32_t msg_code,
                          const void *data, size_t data_length,
                          response_oneshot_cb cb, const void *cookie,
                          double timeout) {

    zmq_ipc_client_sync_ptr client =
        std::dynamic_pointer_cast<zmq_ipc_client_sync>(
            this->get_client_(recipient));

    ipc_msg_ptr msg = client->send_recv(msg_code, data, data_length, timeout);

    this->handle_response_(msg_code, msg, cb, cookie);
}

zmq_ipc_client_ptr
ipc_service_sync::new_client_(uint32_t recipient) {
    return std::make_shared<zmq_ipc_client_sync>(recipient);
}

void
ipc_service_sync::client_receive(uint32_t sender){
}

ipc_service_async::ipc_service_async(uint32_t client_id,
                                     infra_ptr infra)
    : ipc_service(client_id) {

    assert(infra != nullptr);
    this->infra_ = std::move(infra);

    zmq_ipc_server_ptr server = std::make_shared<zmq_ipc_server>(
        this->get_id_());

    this->set_server_(server);
    this->infra_->fd_watch(server->fd(),
                                 sdk::ipc::server_receive,
                                 (void *)this,
                                 this->infra_->fd_watch_ctx);
    this->infra_->fd_watch(this->get_eventfd_(),
                                 sdk::ipc::eventfd_receive,
                                 (void *)this,
                                 this->infra_->fd_watch_ctx);
    this->infra_->fd_watch(this->receive_eventfd_,
                                 sdk::ipc::server_receive,
                                 (void *)this,
                                 this->infra_->fd_watch_ctx);

    // Queue a check for the receive socket
    uint64_t buffer = 1;
    SDK_TRACE_DEBUG("async server asking for server pending message check"); 
    write(this->receive_eventfd_, &buffer, sizeof(buffer));
}

zmq_ipc_client_ptr
ipc_service_async::new_client_(uint32_t recipient) {
    zmq_ipc_client_async_ptr client =
        std::make_shared<zmq_ipc_client_async>(this->get_id_(), recipient);

    this->client_rx_cb_ctx_[recipient] = {this, recipient};

    this->infra_->fd_watch(client->fd(),
                                 sdk::ipc::client_receive,
                                 &this->client_rx_cb_ctx_[recipient],
                                 this->infra_->fd_watch_ctx);

    // We create an event fd, so we check for incoming messages only from the
    // async callback
    this->ipc_client_eventfds_[recipient] =
        eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

    assert(this->ipc_client_eventfds_[recipient] != -1);
    SDK_TRACE_DEBUG("registering eventfd (%u) for client (%u)",
                    this->ipc_client_eventfds_[recipient],
                    recipient);

    this->infra_->fd_watch(this->ipc_client_eventfds_[recipient],
                           sdk::ipc::client_receive,
                           &this->client_rx_cb_ctx_[recipient],
                           this->infra_->fd_watch_ctx);

    this->infra_->fd_watch(client->fd(),
                           sdk::ipc::client_receive,
                           &this->client_rx_cb_ctx_[recipient],
                           this->infra_->fd_watch_ctx);

    // If we don't do this we don't get any events coming from ZMQ
    // It doesn't play very well with libevent
    client->recv();

    return client;
}

ipc_msg_ptr
ipc_service_async::blocked_request(uint32_t recipient, uint32_t msg_code,
                                   const void *data, size_t length,
                                   double timeout)
{
    zmq_ipc_client_async_ptr client =
        std::dynamic_pointer_cast<zmq_ipc_client_async>(
            this->get_client_(recipient));

    ipc_msg_ptr msg = client->send_recv(msg_code, data, length, timeout);

    uint64_t buffer = 1;
    write(this->ipc_client_eventfds_[recipient], &buffer, sizeof(buffer));
    SDK_TRACE_DEBUG("asking for client check for %u", recipient);

    return msg;
}

void
ipc_service_async::request(uint32_t recipient, uint32_t msg_code,
                           const void *data, size_t data_length,
                           response_oneshot_cb cb, const void *cookie,
                           double timeout) {

    zmq_ipc_client_async_ptr client =
        std::dynamic_pointer_cast<zmq_ipc_client_async>(
            this->get_client_(recipient));

    message_ctx_t *msg_ctx = new message_ctx_t();
    msg_ctx->service = this;
    msg_ctx->code = msg_code;
    msg_ctx->cookie = cookie;
    msg_ctx->oneshot_cb = cb;
    if (timeout != 0.0) {
        msg_ctx->timer = this->infra_->timer_add(
            timer_handler, msg_ctx, timeout,
            this->infra_->timer_add_ctx);
    } else {
        msg_ctx->timer = NULL;
    }
    this->msg_contexts_lock_.lock();
    msg_ctx->id = ++this->msg_context_id;
    this->msg_contexts_[msg_ctx->id] = msg_ctx;
    this->msg_contexts_lock_.unlock();

    client->send(msg_code, data, data_length, cb, (void *)msg_ctx->id);

    // zmq requires us to check for incoming messages every time we perform an
    // action on the zmq_socket. Use the event fd to queue the check
    uint64_t buffer = 1;
    write(this->ipc_client_eventfds_[recipient], &buffer, sizeof(buffer));
    SDK_TRACE_DEBUG("asking for client check for %u", recipient);
}

void
ipc_service_async::client_receive(uint32_t sender) {
    assert(sender < IPC_MAX_CLIENT_ID + 1);

    zmq_ipc_client_async_ptr client =
        std::dynamic_pointer_cast<zmq_ipc_client_async>(
            this->get_client_(sender));

    SDK_TRACE_VERBOSE("client receive check for %u", sender);

    if (this->ipc_client_eventfds_[sender] != -1) {
        // Read the eventfd to clear the flag
        uint64_t buffer;
        int rc;

        do {
            rc = read(this->ipc_client_eventfds_[sender], &buffer,
                      sizeof(buffer));
        } while (rc != -1);

    }

    while (true) {
        zmq_ipc_user_msg_ptr msg = client->recv();
        if (msg == nullptr) {
            return;
        }

        uint64_t msg_ctx_id = (uint64_t)msg->cookie();
        message_ctx_t *msg_ctx = NULL;
        this->msg_contexts_lock_.lock();
        if (this->msg_contexts_.count(msg_ctx_id) == 0) {
            // Timer expired and we have already freed this,
            // and called the callback
            SDK_TRACE_DEBUG("We have already dealt with %" PRIu64 ". Ignoring response",
                            msg_ctx_id);
        } else {
            msg_ctx = this->msg_contexts_[msg_ctx_id];
            this->msg_contexts_.erase(msg_ctx_id);
        }
        this->msg_contexts_lock_.unlock();
        if (msg_ctx == NULL) {
            continue;
        }

        this->handle_response_(msg_ctx->code, msg, msg_ctx->oneshot_cb,
                               msg_ctx->cookie);
        if (msg_ctx->timer != NULL) {
            this->infra_->timer_del(msg_ctx->timer,
                                    this->infra_->timer_del_ctx);
        }
        delete msg_ctx;
    }
}

void
ipc_service_async::timer_expired (uint64_t msg_ctx_id)
{
    message_ctx_t *msg_ctx = NULL;
    this->msg_contexts_lock_.lock();
    if (this->msg_contexts_.count(msg_ctx_id) == 0) {
        // Timer expired and we have already freed this,
        // and called the callback
        SDK_TRACE_DEBUG("We have already dealt with %" PRIu64 ". Ignoring timer",
                        msg_ctx_id);
    } else {
        msg_ctx = this->msg_contexts_[msg_ctx_id];
        this->msg_contexts_.erase(msg_ctx_id);
    }
    this->msg_contexts_lock_.unlock();
    if (msg_ctx == NULL) {
        return;
    }

    SDK_TRACE_DEBUG("Timed out waiting a response for %" PRIu64, msg_ctx->id);
    // If timer expires, we call the callback with an empty message
    this->handle_response_(msg_ctx->code, nullptr, msg_ctx->oneshot_cb,
                           msg_ctx->cookie);
    if (msg_ctx->timer != NULL) {
        this->infra_->timer_del(msg_ctx->timer,
                                this->infra_->timer_del_ctx);
    }
    delete msg_ctx;
}

bool
ipc_service::should_serialize_(void) {
    if (!this->serializing_enabled_) {
        return false;
    }

    // We should serialize if there is a message in flight, or
    // there are messages waiting to be delivered
    if ((this->message_in_flight_) ||
        (!this->hold_queue_.empty())) {
        return true;
    }

    return false;
}

void
ipc_service::serialize_(ipc_msg_ptr msg) {
    SDK_TRACE_DEBUG("serializing message - %s", msg->debug().c_str());

    this->hold_queue_.push(msg);
}

void
ipc_service::deserialize_(void) {
    uint64_t buffer = 1;

    if (this->hold_queue_.size() == 0) {
        return;
    }

    SDK_TRACE_DEBUG("messages waiting: %lu", this->hold_queue_.size());

    // Notify the client we have messages for delivery
    write(this->eventfd_, &buffer, sizeof(buffer));
}

void
ipc_service::deliver_broadcast_(ipc_msg_ptr msg) {
    // We shouldn't be receiving this if we don't have a handler
    // assert(this->sub_cbs_.count(msg->code()) > 0);
    //
    // A race condtion during recovery, may cause us to receive
    // messages we haven't subscribed for yet
    if (this->sub_cbs_.count(msg->code()) == 0) {
        SDK_TRACE_ERR("Unexpected Broadcast Message - %s",
            msg->debug().c_str());
        return;
    }

    sub_callback_t sub_cb = this->sub_cbs_[msg->code()];;

    SDK_TRACE_VERBOSE("delivering message - %s", msg->debug().c_str());

    if (sub_cb.cb != NULL) {
        clock_t start = clock();
        double cpu_time_used;

        this->message_in_flight_ = true;
        sub_cb.cb(msg, sub_cb.ctx);
        cpu_time_used = ((double) (clock() - start)) / CLOCKS_PER_SEC;
        if (cpu_time_used > MAX_CALLBACK_DURATION) {
            SDK_TRACE_DEBUG("request handling for %d took %f seconds",
                            msg->code(), cpu_time_used);
        }
    }
}

void
ipc_service::deliver_direct_(ipc_msg_ptr msg) {
    // We received an IPC message but don't have a handler
    assert(this->req_cbs_.count(msg->code()) > 0);

    req_callback_t req_cb = this->req_cbs_[msg->code()];

    SDK_TRACE_VERBOSE("delivering message - %s", msg->debug().c_str());

    if (req_cb.cb != NULL) {
        clock_t start = clock();
        double cpu_time_used;

        this->message_in_flight_ = true;
        req_cb.cb(msg, req_cb.ctx);
        cpu_time_used = ((double) (clock() - start)) / CLOCKS_PER_SEC;
        if (cpu_time_used > MAX_CALLBACK_DURATION) {
            SDK_TRACE_DEBUG("request handling for %d took %f seconds",
                            msg->code(), cpu_time_used);
        }
    }
}

void
ipc_service::deliver_(ipc_msg_ptr msg) {
    if (msg->type() == sdk::ipc::DIRECT) {
        this->deliver_direct_(msg);
    } else if (msg->type() == sdk::ipc::BROADCAST) {
        this->deliver_broadcast_(msg);
    } else {
        assert(0);
    }
}

int
ipc_service::get_eventfd_(void) {
    return this->eventfd_;
}

void
ipc_service::server_receive(void) {
    uint64_t buffer;
    int rc;

    // clear out the receive_eventfd_ socket
    do {
        rc = read(this->receive_eventfd_, &buffer, sizeof(buffer));
    } while (rc != -1);

    while (true) {
        ipc::ipc_msg_ptr msg = this->ipc_server_->recv();
        if (msg == nullptr) {
            return;
        }
        if (this->should_serialize_()) {
            this->serialize_(msg);
        } else {
            this->deliver_(msg);
        }
    }
}

void
ipc_service::eventfd_receive(void) {
    uint64_t buffer;
    int rc;

    // Clear the eventfd flag
    do {
        rc = read(this->eventfd_, &buffer, sizeof(buffer));
    } while (rc != -1);

    if (!this->hold_queue_.empty()) {
        ipc_msg_ptr msg = this->hold_queue_.front();
        SDK_TRACE_DEBUG("deserializing msg - %s", msg->debug().c_str());
        this->hold_queue_.pop();
        this->deliver_(msg);
    }
}

void
ipc_service::broadcast(uint32_t msg_code, const void *data,
                       size_t data_length) {
    std::vector<uint32_t> recipients = subscribers::instance()->get(msg_code);

    if (recipients.size() == 0) {
        SDK_TRACE_VERBOSE("no subscribers for message: msg_code: %u", msg_code);
    }
    for (uint32_t recipient : recipients) {
        this->get_client_(recipient)->broadcast(msg_code, data, data_length);
    }
    for (uint32_t recipient : recipients) {
        this->client_receive(recipient);
    }
}

void
ipc_service::reg_request_handler(uint32_t msg_code, request_cb callback,
                                 const void *ctx) {
    assert(this->req_cbs_.count(msg_code) == 0);
    this->req_cbs_[msg_code] = {.cb = callback, .ctx = ctx};
}

void
ipc_service::reg_response_handler(uint32_t msg_code, response_cb callback,
                                  const void *ctx) {
    assert(this->rsp_cbs_.count(msg_code) == 0);
    this->rsp_cbs_[msg_code] = {.cb = callback, .ctx = ctx};
}

void
ipc_service::subscribe(uint32_t msg_code, subscription_cb callback,
                       const void *ctx) {
    assert(this->sub_cbs_.count(msg_code) == 0);
    this->sub_cbs_[msg_code] = {.cb = callback, .ctx = ctx};
    this->ipc_server_->subscribe(msg_code);
}

void
ipc_service::set_drip_feeding(bool enabled) {
    this->serializing_enabled_ = enabled;
}

// We use this as for non asynchronous thread creating a client
// upfront is not required for request or broadcast.
// We create the first time an API gets called
ipc_service_ptr
service (void)
{
    // first try to find a handle based on the thread name
    if (t_ipc_service == nullptr) {
        g_thread_handles_lock.lock();
        t_ipc_service = g_thread_handles[thread_name()];
        g_thread_handles_lock.unlock();
    }

    // if it's still null, create one ad-hoc
    if (t_ipc_service == nullptr) {
        t_ipc_service = std::make_shared<ipc_service_sync>();
    }

    return t_ipc_service;
}

void
ipc_init_async (uint32_t client_id, infra_ptr infra, bool associate_thread_name)
{
    if (associate_thread_name) {
        g_thread_handles_lock.lock();
        assert(g_thread_handles.count(thread_name()) == 0);

        t_ipc_service = nullptr;
        t_ipc_service = std::make_shared<ipc_service_async>(client_id,
                                                            std::move(infra));
        g_thread_handles[thread_name()] = t_ipc_service;
        g_thread_handles_lock.unlock();
    } else {
        assert(t_ipc_service == nullptr);
        t_ipc_service = std::make_shared<ipc_service_async>(client_id,
                                                            std::move(infra));
    }
}

void
ipc_init_sync (uint32_t client_id)
{
    assert(t_ipc_service == nullptr);
    t_ipc_service = std::make_shared<ipc_service_sync>(client_id);
}

void
ipc_init_sync (uint32_t client_id,
               infra_ptr infra)
{
    assert(t_ipc_service == nullptr);
    t_ipc_service = std::make_shared<ipc_service_sync>(
        client_id, std::move(infra));
}


uint32_t
ipc_id()
{
    assert(t_ipc_service != nullptr);
    return t_ipc_service->get_id();
}

ipc_msg_ptr
blocked_request(uint32_t recipient, uint32_t msg_code, const void *data,
                size_t length, double timeout)
{
    return service()->blocked_request(recipient, msg_code, data, length,
                                      timeout);
}

// !!!FIXME WARNING!!!
// All callers of this function are currently using a local variable as
// cookie which, if the underlying IPC channel is configured for async,
// can potentially cause memory corruption.
//
// All such calls to FIXME_request() must be converted to either
// blocked_request() to force a blocking behavior, or to request()
// using a non-local or NULL cookie context.
void FIXME_request(uint32_t recipient, uint32_t msg_code,
                   const void *data, size_t length,
                   response_oneshot_cb cb, const void *cookie,
                   double timeout)
{
    ipc_msg_ptr msg = service()->blocked_request(recipient, msg_code,
                                                 data, length, timeout);
    if (cb) {
        cb(msg, cookie);
    }
}

void
request (uint32_t recipient, uint32_t msg_code, const void *data,
         size_t data_length, const void *cookie, double timeout)
{
    service()->request(recipient, msg_code, data, data_length, NULL, cookie,
                       timeout);
}

void
request (uint32_t recipient, uint32_t msg_code, const void *data,
         size_t data_length, response_oneshot_cb cb, const void *cookie,
         double timeout)
{
    service()->request(recipient, msg_code, data, data_length, cb, cookie,
                       timeout);
}

ipc_msg_ptr
zrequest (uint32_t card_id, uint32_t endpoint, uint32_t opcode, void *data,
        size_t length)
{
    return service()->zrequest(card_id, endpoint, opcode, data, length);
}

void
broadcast (uint32_t msg_code, const void *data, size_t data_length)
{
    service()->broadcast(msg_code, data, data_length);
}

// We need to call init or init_async before using this method
void
reg_request_handler (uint32_t msg_code, request_cb callback, const void *ctx)
{
    assert(t_ipc_service != nullptr);
    t_ipc_service->reg_request_handler(msg_code, callback, ctx);
}

void
reg_response_handler (uint32_t msg_code, response_cb callback, const void *ctx)
{
    service()->reg_response_handler(msg_code, callback, ctx);
}

// We need to call init or init_async before using this method
void
subscribe (uint32_t msg_code, subscription_cb callback, const void *ctx)
{
    assert(t_ipc_service != nullptr);
    t_ipc_service->subscribe(msg_code, callback, ctx);
}

void
respond (ipc_msg_ptr msg, const void *data, size_t data_length)
{
    assert(t_ipc_service != nullptr);
    t_ipc_service->respond(msg, data, data_length);
}

void
broadcast_handled (ipc_msg_ptr msg)
{
    assert(t_ipc_service != nullptr);
    t_ipc_service->broadcast_handled(msg);
}


void
set_drip_feeding (bool enabled)
{
    service()->set_drip_feeding(enabled);
}

void
set_debug_delays (bool enabled)
{
}

} // namespace ipc
} // namespace sdk
