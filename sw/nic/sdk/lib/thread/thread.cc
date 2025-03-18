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


#include <unistd.h>
#include <cstring>
#include "include/sdk/base.hpp"
#include "include/sdk/mem.hpp"
#include "lib/thread/thread.hpp"
#include "lib/utils/thread_pool.hpp"
#include <sys/sysinfo.h>

namespace sdk {
namespace lib {

thread_local
thread*  thread::t_curr_thread_      = NULL;

uint64_t thread::control_cores_mask_ = 0;
uint64_t thread::data_cores_free_    = 0;
uint64_t thread::data_cores_mask_    = 0;
bool     thread::super_user_ =
             (getenv("USER") && !strcmp(getenv("USER"), "root")) ? true : false;

// initialize the thread store
thread_store_t thread::g_thread_store_;

// as thread pool is a header only file, defining it here
uint32_t thread_pool::active_nthreads_ = 0;

//------------------------------------------------------------------------------
// thread instance initialization
//------------------------------------------------------------------------------
int
thread::init(const char *name, uint32_t thread_id,
             thread_role_t thread_role, uint64_t cores_mask,
             thread_entry_func_t entry_func, uint32_t prio,
             int sched_policy, uint32_t flags)
{
    if (!name || !entry_func) {
        return -1;
    }

    if (cores_mask_validate(thread_role, cores_mask) != SDK_RET_OK) {
        return -1;
    }

    strncpy(name_, name, SDK_MAX_THREAD_NAME_LEN);
    name_[SDK_MAX_THREAD_NAME_LEN] = '\0';
    thread_id_ = thread_id;
    entry_func_ = entry_func;
    prio_ = prio;
    sched_policy_ = sched_policy;
    flags_ = flags;
    cores_mask_ = cores_mask;
    thread_role_ = thread_role;
    pthread_id_ = 0;
    running_ = false;
    memset(&hb_ts_, 0, sizeof(hb_ts_));
    lfq_ = lfq::factory(32);
    suspend_cond_ = PTHREAD_COND_INITIALIZER;
    suspend_cond_lock_ = PTHREAD_MUTEX_INITIALIZER;
    suspend_ = false;
    suspended_ = false;
    suspend_cb_ = NULL;
    resume_cb_ = NULL;
    return 0;
}

sdk_ret_t
thread::cores_mask_validate(thread_role_t thread_role,
                            uint64_t mask)
{
    switch (thread_role) {
    case THREAD_ROLE_CONTROL:
        // check if the mask bits are present in control cores mask
        if (mask != 0 &&
            (mask & control_cores_mask_) == 0) {
            SDK_TRACE_ERR("Invalid control core mask 0x%lx."
                          " Expected: 0x%lx\n",
                          mask, control_cores_mask_);
            return SDK_RET_ERR;
        }
        break;

    default:
        // check if the mask bit is present in data cores mask
        if ((mask & data_cores_free_) == 0) {
            SDK_TRACE_ERR("Invalid data core mask 0x%lx."
                          " Expected, one of: 0x%lx\n",
                          mask, data_cores_free_);
            return SDK_RET_ERR;
        }

        // check if only one bit is set in mask
        if ((mask & (mask - 1)) != 0) {
            SDK_TRACE_ERR("Invalid data core mask 0x%lx."
                          " Expected, one of: 0x%lx\n",
                          mask, data_cores_free_);
            return SDK_RET_ERR;
        }

        // mark the core bit as taken
        data_cores_free_ = data_cores_free_ & ~mask;
        break;
    }

    return SDK_RET_OK;
}

//------------------------------------------------------------------------------
// factory method
//------------------------------------------------------------------------------
thread *
thread::factory(const char *name, uint32_t thread_id,
                thread_role_t thread_role, uint64_t cores_mask,
                thread_entry_func_t entry_func, uint32_t prio,
                int sched_policy, uint32_t flags)
{
    int       rv;
    void      *mem;
    thread    *new_thread;

    mem = SDK_CALLOC(SDK_MEM_ALLOC_LIB_THREAD, sizeof(thread));
    if (!mem) {
        return NULL;
    }
    new_thread = new (mem) thread();
    rv = new_thread->init(name, thread_id, thread_role, cores_mask,
                          entry_func, prio, sched_policy, flags);
    if (rv < 0) {
        new_thread->~thread();
        SDK_FREE(SDK_MEM_ALLOC_LIB_THREAD, new_thread);
        return NULL;
    }
    g_thread_store_.add(thread_id, new_thread);
    return new_thread;
}

//------------------------------------------------------------------------------
// destructor
//------------------------------------------------------------------------------
thread::~thread()
{
    if (running_) {
        this->stop();
    }
}

void
thread::destroy(thread *th)
{
    if (!th) {
        return;
    }
    lfq::destroy(th->lfq_);
    g_thread_store_.remove(th->thread_id_);
    th->~thread();
    SDK_FREE(SDK_MEM_ALLOC_LIB_THREAD, th);
}

thread *
thread::find(uint32_t thread_id)
{
    return g_thread_store_.find(thread_id);
}

uint64_t
thread::get_cpu_mask (cpu_set_t cpu_set)
{
    uint64_t cpu_mask = 0x0;

    for (int i = 0; i < get_nprocs(); ++i) {
        if (CPU_ISSET(i, &cpu_set) != 0) {
            cpu_mask |= (1 << i);
        }
    }

    return cpu_mask;
}

uint64_t
thread::get_cpu_mask (void)
{
    return get_cpu_mask(cpu_set_);
}

//------------------------------------------------------------------------------
// start the thread
//------------------------------------------------------------------------------
sdk_ret_t
thread::start(void *ctxt)
{
    int                   rv;
    pthread_attr_t        attr;
    struct sched_param    sched_params;
    uint64_t              mask = cores_mask_;

    if (running_) {
        return SDK_RET_OK;
    }

    // initialize the pthread attributes
    rv = pthread_attr_init(&attr);
    if (rv != 0) {
        SDK_TRACE_ERR("pthread_attr_init failure, err : %d", rv);
        return SDK_RET_ERR;
    }

    CPU_ZERO(&cpu_set_);

    switch (thread_role_) {
    case THREAD_ROLE_CONTROL:
        if (mask == 0x0) {
            mask = thread::control_cores_mask_;
        }
        break;

    default:
        break;
    }

    // set core affinity
    if (mask != 0) {
        while (mask != 0) {
            CPU_SET(ffsl(mask) - 1, &cpu_set_);
            mask = mask & (mask - 1);
        }
        rv = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_set_);
        if (rv != 0) {
            SDK_TRACE_ERR("pthread_attr_setaffinity_np failure, err : %d", rv);
            return SDK_RET_ERR;
        }
    }

    if ((sched_policy_ == SCHED_FIFO) || (sched_policy_ == SCHED_RR)) {
        // set explicit scheduling policy option
        rv = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if (rv != 0) {
            SDK_TRACE_ERR("pthread_attr_setinheritsched failure, err : %d", rv);
            return SDK_RET_ERR;
        }

        // set the scheduling policy
        rv = pthread_attr_setschedpolicy(&attr, sched_policy_);
        if (rv != 0) {
            SDK_TRACE_ERR("pthread_attr_setschedpolicy failure, err : %d", rv);
            return SDK_RET_ERR;
        }

        // set the thread priority
        sched_params.sched_priority = prio_;
        rv = pthread_attr_setschedparam(&attr, &sched_params);
        if (rv != 0) {
            SDK_TRACE_ERR("pthread_attr_setschedparam failure, err : %d", rv);
            return SDK_RET_ERR;
        }
    }

    // create the thread now
    rv = pthread_create(&pthread_id_, &attr, entry_func_, ctxt);
    if (rv != 0) {
        int conf_cores, avail_cores;
        conf_cores = get_nprocs_conf();
        avail_cores = get_nprocs();
        SDK_TRACE_ERR("pthread_create failure, err : %d, role : %d, "
                      "cores_mask : %lu, sched_policy : %d, "
                      "sched_priority : %u, conf_cores : %d avail_cores : %d",
                      rv, thread_role_, cores_mask_, sched_policy_, prio_,
                      conf_cores, avail_cores);
        return SDK_RET_ERR;
    } else {
        int conf_cores, avail_cores;
        conf_cores = get_nprocs_conf();
        avail_cores = get_nprocs();
        SDK_TRACE_DEBUG("Instantiated thread, "
                        "name : %s, id : %u, pthread id %u, role %u, "
                        "sched_policy : %d, priority : %u, "
                        "CPU mask: 0x%lx, cores mask : %lu, "
                        "configured cores : %d, available cores : %d",
                        name_, thread_id_, (uint32_t)pthread_id_, thread_role_,
                        sched_policy_, prio_,
                        get_cpu_mask(), cores_mask_,
                        conf_cores, avail_cores);
    }

    // set thread's first heartbeat timestamp
    clock_gettime(CLOCK_MONOTONIC, &hb_ts_);

    // set the thread's name, for debugging
    rv = pthread_setname_np(pthread_id_, name_);
    if (rv != 0) {
        SDK_TRACE_ERR("pthread_setname_np failure, err : %d", rv);
        return SDK_RET_ERR;
    }

    // clean up
    rv = pthread_attr_destroy(&attr);
    if (rv != 0) {
        SDK_TRACE_ERR("pthread_attr_destroy failure, err : %d", rv);
        return SDK_RET_ERR;
    }

    //  thread is ready now
    running_ = true;

    return SDK_RET_OK;
}

//------------------------------------------------------------------------------
// stop the thread
//------------------------------------------------------------------------------
sdk_ret_t
thread::stop(void)
{
    int    rv;

    if (!running_) {
        return SDK_RET_OK;
    }

    rv = pthread_cancel(pthread_id_);
    if (rv != 0) {
        SDK_TRACE_ERR("pthread cancel failed on thread %s", name_);
    }

    running_ = false;
    return SDK_RET_OK;
}

//------------------------------------------------------------------------------
// wait for the thread to complete
//------------------------------------------------------------------------------
sdk_ret_t
thread::wait_until_complete(void)
{
    int    rv;
    void   *res;

    rv = pthread_join(pthread_id_, &res);
    if (rv != 0) {
        SDK_TRACE_ERR("pthread cancel failed on thread %s", name_);
    }
    SDK_TRACE_DEBUG("pthread exit code for thread %s is %s", name_,
                    res == PTHREAD_CANCELED ? "cancelled" : "exit");
    return SDK_RET_OK;
}

//------------------------------------------------------------------------------
// wait for the thread to complete with timeout
//------------------------------------------------------------------------------
sdk_ret_t
thread::wait_with_timeout(int seconds)
{
    int    rv;
    void   *res;
    struct timespec ts;

    rv = clock_gettime(CLOCK_REALTIME, &ts);
    if (rv != 0) {
        SDK_TRACE_ERR("Failed to get system time");
        return SDK_RET_ERR;
    }

    ts.tv_sec += seconds;
    rv = pthread_timedjoin_np(pthread_id_, &res, &ts);
    if (rv != 0) {
        SDK_TRACE_ERR("pthread cancel failed on thread %s", name_);
        return SDK_RET_TIMEOUT;
    }
    SDK_TRACE_DEBUG("pthread exit code for thread %s is %s", name_,
                    res == PTHREAD_CANCELED ? "cancelled" : "exit");
    return SDK_RET_OK;
}

//------------------------------------------------------------------------------
// wait for the thread to complete. Thread needs to be stopped first.
//------------------------------------------------------------------------------
sdk_ret_t
thread::wait(int seconds)
{
    if (running_) {
        SDK_TRACE_ERR("pthread cancel not done on thread %s", name_);
        return SDK_RET_ERR;
    }
    if (seconds == 0) {
        return wait_until_complete();
    } else {
        return wait_with_timeout(seconds);
    }
}

//------------------------------------------------------------------------------
// punch heart beat
//------------------------------------------------------------------------------
void
thread::punch_heartbeat(void)
{
    clock_gettime(CLOCK_MONOTONIC, &hb_ts_);
}

sdk_ret_t
thread::walk(thread_walk_cb_t walk_cb, void *ctxt)
{
    return g_thread_store_.walk(walk_cb, ctxt);
}

void
thread::check_and_suspend(void) {
    sdk_ret_t ret;

    while (suspend_) {
        if (suspend_cb_) {
            ret = suspend_cb_(suspend_cb_arg_);
            if (ret != SDK_RET_OK) {
                suspend_ = false; // clearing the state to non suspended
                SDK_TRACE_ERR("Suspend request for thread %s failed, err %u",
                              name_, ret());
                return;
            }
        }

        pthread_mutex_lock(&suspend_cond_lock_);
        suspended_ = true;
        SDK_TRACE_VERBOSE("Thread %s, id %u waiting in suspend", name_,
                          thread_id_);
        pthread_cond_wait(&suspend_cond_, &suspend_cond_lock_);
        SDK_TRACE_VERBOSE("Thread %s, id %u woken up after suspend wait, "
                          "suspend %u", name_, thread_id_, suspend_);
        suspended_ = false;
        pthread_mutex_unlock(&suspend_cond_lock_);

        if (!suspend_ && resume_cb_) {
            ret = resume_cb_(suspend_cb_arg_);
            if (ret != SDK_RET_OK) {
                SDK_TRACE_INFO("Resume request for thread %s failed, err %u",
                               name_, ret());
                suspended_ = true; // keeping the state as suspended
            }
        }
    }
}

sdk_ret_t
thread::suspend_req(thread_suspend_req_func_t func) {
    sdk_ret_t ret;

    SDK_TRACE_INFO("Suspend request for thread %s", name_);
    // ignore if there is a pending request
    if (suspend_ == true) {
        SDK_TRACE_INFO("Suspend request for thread %s, prev request pending",
                       name_);
    } else {
        suspend_ = true;
    }
    if (func) {
        ret = func(this);
        if (ret != SDK_RET_OK) {
            SDK_TRACE_ERR("Suspend request for thread %s failed", name_);
            return ret;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
thread::resume_req(void) {
    SDK_TRACE_INFO("Resume request for thread %s", name_);
    // ignore if there is no pending suspend request
    if (suspend_ == false) {
        SDK_TRACE_INFO("Resume request for thread %s that is not in "
                       "suspended state", name_);
        return SDK_RET_OK;
    }
    suspend_ = false;
    pthread_mutex_lock(&suspend_cond_lock_);
    if (suspended_ == true) {
        pthread_cond_signal(&suspend_cond_);
    }
    pthread_mutex_unlock(&suspend_cond_lock_);
    return SDK_RET_OK;
}

}    // namespace lib
}    // namespace sdk
