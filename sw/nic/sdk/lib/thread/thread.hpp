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
// thread library
//------------------------------------------------------------------------------

#ifndef __SDK_THREAD_HPP__
#define __SDK_THREAD_HPP__

#include <unordered_map>
#include <pthread.h>
#include "include/sdk/base.hpp"
#include "include/sdk/lock.hpp"
#include "include/sdk/timestamp.hpp"
#include "lib/logger/logger.h"
#include "lib/lfq/lfq.hpp"
#define pthread_yield sched_yield

using std::unordered_map;

namespace sdk {
namespace lib {

typedef enum thread_role_e {
    THREAD_ROLE_CONTROL,
    THREAD_ROLE_DATA
} thread_role_t;

// thread flags
typedef enum thread_flags_e {
    THREAD_FLAGS_NONE      = 0,
    THREAD_YIELD_ENABLE    = (1 << 0),
    THREAD_SUSPEND_DISABLE = (1 << 1),
} thread_flags_t;

//------------------------------------------------------------------------------
// thread entry function
//------------------------------------------------------------------------------
typedef void *(*thread_entry_func_t)(void *ctxt);
typedef sdk_ret_t (*thread_suspend_cb_t)(void *arg);
typedef sdk_ret_t (*thread_resume_cb_t)(void *arg);

#define SDK_MAX_THREAD_NAME_LEN        30

// all SDK threads must invoke this macro in the entry function
#define SDK_THREAD_INIT(ctxt)                                          \
{                                                                      \
    sdk::lib::thread::set_current_thread((sdk::lib::thread *)ctxt);    \
    SDK_TRACE_DEBUG("Initializing %s thread ...",                      \
                    sdk::lib::thread::current_thread()->name());       \
}

// forward declaration
class thread;

// walk callback function type
typedef bool (thread_walk_cb_t)(sdk::lib::thread *thr, void *ctxt);

// thread suspend request function type
typedef sdk_ret_t (*thread_suspend_req_func_t)(sdk::lib::thread *thr);

// thread strore
typedef struct thread_store_s {
    thread_store_s() {
        SDK_SPINLOCK_INIT(&slock_, PTHREAD_PROCESS_PRIVATE);
    }

    ~thread_store_s() {
        SDK_SPINLOCK_DESTROY(&slock_);
    }

    void add(uint32_t thread_id, thread *thr) {
        SDK_SPINLOCK_LOCK(&slock_);
        thread_db_[thread_id] = thr;
        SDK_SPINLOCK_UNLOCK(&slock_);
    }

    void remove(uint32_t thread_id) {
        SDK_SPINLOCK_LOCK(&slock_);
        thread_db_.erase(thread_id);
        SDK_SPINLOCK_UNLOCK(&slock_);
    }

    thread *find(uint32_t thread_id) {
        thread *thr;

        SDK_SPINLOCK_LOCK(&slock_);
        auto it = thread_db_.find(thread_id);
        if (it != thread_db_.end()) {
            thr = it->second;
        } else {
            thr = NULL;
        }
        SDK_SPINLOCK_UNLOCK(&slock_);
        return thr;
    }

    sdk_ret_t walk(thread_walk_cb_t walk_cb, void *ctxt) {
        sdk::lib::thread *thr;
        bool end_walk = false;

        SDK_SPINLOCK_LOCK(&slock_);
        for (auto it = thread_db_.begin(); it != thread_db_.end(); ) {
            thr = it->second;
            it++;
            SDK_SPINLOCK_UNLOCK(&slock_);
            end_walk = walk_cb(thr, ctxt);
            SDK_SPINLOCK_LOCK(&slock_);
            if (end_walk) {
                // break the walk
                goto end;
            }
        }
    end:
        SDK_SPINLOCK_UNLOCK(&slock_);
        return SDK_RET_OK;
    }

private:
    sdk_spinlock_t slock_;
    unordered_map<uint32_t, thread *> thread_db_;

} thread_store_t;

class thread {
public:
    static thread *factory(const char *name, uint32_t thread_id,
                           thread_role_t thread_role, uint64_t cores_mask,
                           thread_entry_func_t entry_func,
                           uint32_t prio, int sched_policy,
                           uint32_t flags = THREAD_FLAGS_NONE);
    static void destroy(thread *th);
    static thread *find(uint32_t thread_id);
    static void *dummy_entry_func(void *ctxt) { return NULL; }
    virtual sdk_ret_t start(void *ctxt);
    virtual sdk_ret_t stop(void);
    sdk_ret_t wait(int seconds = 0);
    sdk_ret_t wait_until_complete(void);
    sdk_ret_t wait_with_timeout(int seconds);
    const char *name(void) const { return name_; }
    uint32_t thread_id(void) const { return thread_id_; }
    uint32_t priority(void) const { return prio_; }
    uint32_t sched_policy(void) const {return sched_policy_; };
    void set_pthread_id(pthread_t pthread_id) { pthread_id_ = pthread_id; }
    pthread_t pthread_id(void) const { return pthread_id_; }
    void punch_heartbeat(void);    // punch heart-beat
    timespec_t heartbeat_ts(void) const { return hb_ts_; }
    bool can_yield(void) const { return (flags_ & THREAD_YIELD_ENABLE); }
    bool is_running(void) const { return running_; }
    void set_running(bool running) { running_ = running; }
    bool ready(void) const { return ready_; }
    void set_ready(bool ready) { ready_ = ready; }
    void set_data(void *data) { data_ = data; }
    void *data(void) const { return data_; }
    thread_role_t thread_role(void) const { return thread_role_; }
    uint64_t cores_mask(void) const { return cores_mask_; }
    uint64_t get_cpu_mask(void);
    static sdk_ret_t walk(thread_walk_cb_t walk_cb, void *ctxt);

    // set the current thread instance
    static void set_current_thread(thread *curr_thread) {
        t_curr_thread_ = curr_thread;
    }

    // get the current thread instance
    static thread* current_thread(void) {
        return sdk::lib::thread::t_curr_thread_;
    }

    static void control_cores_mask_set(uint64_t mask) {
        control_cores_mask_ = mask;
        SDK_TRACE_PRINT("control_cores_mask : 0x%lx", control_cores_mask_);
    }

    static void data_cores_mask_set(uint64_t mask) {
        data_cores_mask_ = data_cores_free_ = mask;
        SDK_TRACE_PRINT("data_cores_mask : 0x%lx", data_cores_mask_);
    }
    static uint64_t get_cpu_mask(cpu_set_t cpu_set);
    static uint64_t control_cores_mask(void) { return control_cores_mask_; }
    static uint64_t data_cores_mask(void) { return data_cores_mask_; }
    static int sched_policy_by_role(thread_role_t role) {
        return SCHED_OTHER;
    }
    static int priority_by_role(thread_role_t role) {
        int    prio, sched_policy;

        sched_policy = sched_policy_by_role(role);
        prio = sched_get_priority_max(sched_policy);
        if (sched_policy == SCHED_FIFO) {
            prio = 50;    // don't consume 100%
        }
        return prio;
    }
    // enqueue event to this thread
    bool enqueue(void *item) {
        return lfq_->enqueue(item);
    }

    // dequeue event from this thread
    void *dequeue(void) {
        return lfq_->dequeue();
    }

    lfq *lfqueue(void) { return lfq_; }

    void register_suspend_cb(thread_suspend_cb_t suspend_cb,
                             thread_resume_cb_t resume_cb, void *arg) {
        suspend_cb_ = suspend_cb;
        resume_cb_ =  resume_cb;
        suspend_cb_arg_ =  arg;
    }
    // invoked from caller thread context
    virtual sdk_ret_t suspend_req(thread_suspend_req_func_t);
    // invoked from caller thread context
    sdk_ret_t resume_req(void);
    // invoked from the target thread context
    void check_and_suspend(void);
    bool suspended(void) { return suspended_; }
    bool skip_suspend(void) { return (flags_ & THREAD_SUSPEND_DISABLE); }
    uint32_t flags(void) { return flags_; }

private:
    char                          name_[SDK_MAX_THREAD_NAME_LEN+1];
    uint32_t                      thread_id_;
    thread_entry_func_t           entry_func_;
    uint32_t                      prio_;
    int                           sched_policy_;
    void                          *data_;
    pthread_t                     pthread_id_;
    bool                          running_;
    bool                          ready_;
    thread_role_t                 thread_role_;
    uint64_t                      cores_mask_;
    timespec_t                    hb_ts_;
    cpu_set_t                     cpu_set_;
    lfq                           *lfq_;
    static thread_local thread    *t_curr_thread_;
    static uint64_t               control_cores_mask_;
    static uint64_t               data_cores_free_;
    static uint64_t               data_cores_mask_;
    static bool                   super_user_;
    bool                          suspended_;
    thread_suspend_cb_t           suspend_cb_;
    thread_resume_cb_t            resume_cb_;
    pthread_cond_t                suspend_cond_;
    pthread_mutex_t               suspend_cond_lock_;
    void                          *suspend_cb_arg_;
    uint32_t                      flags_; // see thread_flags_t

protected:
    static thread_store_t         g_thread_store_;

protected:
    virtual int init(const char *name, uint32_t thread_id,
        thread_role_t thread_role, uint64_t cores_mask,
        thread_entry_func_t entry_func,
        uint32_t prio, int sched_policy, uint32_t flags);
    bool suspend_;
    thread() {};
    ~thread();

private:
    static sdk_ret_t cores_mask_validate(thread_role_t thread_role,
                                         uint64_t mask);
};

}    // namespace lib
}    // namespace sdk

using sdk::lib::thread;
using sdk::lib::thread_flags_t::THREAD_FLAGS_NONE;
using sdk::lib::thread_flags_t::THREAD_YIELD_ENABLE;
using sdk::lib::thread_flags_t::THREAD_SUSPEND_DISABLE;

#endif    // __SDK_THREAD_HPP__
