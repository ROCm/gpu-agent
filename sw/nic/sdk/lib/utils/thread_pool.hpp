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
/// thread pool
///
//----------------------------------------------------------------------------

#ifndef __SDK_THREAD_POOL_HPP__
#define __SDK_THREAD_POOL_HPP__

#include <pthread.h>
#include <semaphore.h>
#include <sys/queue.h>
#include "include/sdk/base.hpp"
#include "include/sdk/mem.hpp"
#include "include/sdk/assert.hpp"
#include "lib/lfq/lfq.hpp"
#define pthread_yield sched_yield

namespace sdk {
namespace lib {

#define MAX_TPOOL_THREADS  64
#define MAX_TPOOL_MESSAGES 32 // sender will block when the queue is full

typedef uint32_t work_id_t;
typedef sdk_ret_t (*work_cb_t)(void *);
typedef void (*work_status_cb_t)(work_id_t id, sdk_ret_t status);
class thread_pool;

// barrier to know the whether the given work or group of work has been done.
class work_barrier {
public:
    // initialize the mutex as taken
    work_barrier() { pending_ = 0; }
    ~work_barrier() { }
private:
    // initialize by specifying the number of works
    void init_(uint32_t count) {
        pending_ = count;
        pthread_mutex_init(&wait_lock_, NULL);
        pthread_mutex_lock(&wait_lock_);
    }
    /// \brief wait for the completion of the work
    void wait_(void) { pthread_mutex_lock(&wait_lock_); }

    // update the completion of the work. this is an internal function
    void done_(void) {
        SDK_ATOMIC_FETCH_SUB(&pending_, 1);
        SDK_ASSERT(pending_ >= 0);
        if (!pending_) {
            pthread_mutex_unlock(&wait_lock_);
        }
    }
private:
    int32_t pending_;            // how many works are waiting to be completed
    pthread_mutex_t wait_lock_;  // barrier wait lock
    friend class thread_pool;    // thread pool class can access barrier class
};

// work entry used to submit work to a thread_pool
typedef struct work_entry_s {
    work_cb_t cb;               // work callback function
    void *cb_arg;               // callback argument
    work_status_cb_t status_cb; // status callback
    work_id_t id;               // user given id for this work
    work_barrier *barrier;      // barrier
} work_entry_t;


//------------------------------------------------------------------------------
// thread_pool creates a pool of worker threads bound to a set of cpu cores.
// worker threads can be assigned work by posting a work_entry_t via the
// work_post method. Single service queue is used to assign work to available
// worker threads
//------------------------------------------------------------------------------
class thread_pool {
public:
    thread_pool() {
        nthreads_ = 0;
        stop_ = false;
        affinity_set_ = true;
        pending_ = 0;
        tid_ = 0;
    }
    ~thread_pool() {}

    /// \brief create and initialize the thread_pool
    /// \param[in] nthreads number of worker threads
    /// \param[in] cpu_mask cpu mask to bind the worker threads to. if it zero
    ///                     threads no affinity will be set
    /// \param[in] single_cpu_per_thread worker threads affinity will be set to
    ///                                  single cpu derived from the cpu mask.
    ///                                  cpu_mask is mandatory if it is true
    /// \return pointer to thread pool on success, null on failure
    static thread_pool *factory(int nthreads, uint64_t cpu_mask,
                                bool single_cpu_per_thread) {
        thread_pool *tp;
        void *mem;

        SDK_ASSERT(nthreads <= MAX_TPOOL_THREADS);
        mem = SDK_CALLOC(SDK_MEM_ALLOC_LIB_THREAD_POOL, sizeof(thread_pool));
        if (mem == NULL) {
            SDK_TRACE_ERR("Failed to create thread pool");
            return NULL;
        }
        tp = new (mem) thread_pool();
        tp->ncpus_ = 0;
        if (single_cpu_per_thread) {
            SDK_ASSERT(cpu_mask != 0ULL);
            for (uint32_t cpu = 0; cpu < sizeof(cpu_mask) * 8; cpu++) {
                SDK_ASSERT(tp->ncpus_ <= MAX_TPOOL_THREADS);
                if (cpu_mask & (1ULL << cpu)) {
                    tp->cpu_ids_[tp->ncpus_++] = cpu;
                }
            }
        } else if (cpu_mask != 0ULL) {
            CPU_ZERO(&tp->cpu_mask_);
            for (uint32_t cpu = 0; cpu < sizeof(cpu_mask) * 8; cpu++) {
                if (cpu_mask & (1ULL << cpu)) {
                    CPU_SET(cpu, &tp->cpu_mask_);
                }
            }
        } else {
            tp->affinity_set_ = false;
        }
        tp->lfq_ = lfq::factory(MAX_TPOOL_MESSAGES);
        sem_init(&tp->sem_, 0, 0);
        memset(tp->run_count_, 0, nthreads * sizeof(uint32_t));
        tp->start_(nthreads);
        return tp;
    }

    /// \brief de-init a thread pool, all workers are stopped and released
    /// \return none
    static sdk_ret_t destroy(thread_pool *tp) {
        void *ret = nullptr;

        if (!tp->tpool_) {
            return SDK_RET_ERR;
        }

        // return error if there are pending tasks in the queue
        // this will cause memory leak . TODO : drain() if required
        if (tp->pending_) {
            SDK_TRACE_ERR("Thread pool queue is not empty, pending %u", tp->pending_);
            return SDK_RET_RETRY;
        }

        tp->stop_ = true;
        for (uint32_t t = 0; t < tp->nthreads_; ++t) {
            sem_post(&tp->sem_);
        }
        for (uint32_t t = 0; t < tp->nthreads_; ++t) {
            SDK_TRACE_VERBOSE("Tasks executed by worker %u is %u",
                              t, tp->run_count_[t]);
            pthread_join(tp->tpool_[t], &ret);
        }

        sem_destroy(&tp->sem_);
        lfq::destroy(tp->lfq_);
        SDK_ATOMIC_FETCH_SUB(&active_nthreads_, tp->nthreads_);
        SDK_FREE(SDK_MEM_ALLOC_LIB_THREAD_POOL, tp);
        return SDK_RET_OK;
    }

    /// \brief post a work entry to the thread pool
    /// \param[in] cb application callback invoked from the worker thread
    /// \param[in] arg application callback argument
    /// \param[in] id application specific work id for this work
    /// \param[in] status_cb application work done status callback
    /// \param[in] barrier for waiting for the completion. library supports
    ///                    specifying status_cb and barrier togother or both
    ///                    can be none also
    /// \return none
    void work_post(work_cb_t cb, void *arg, work_id_t id,
                   work_status_cb_t status_cb, work_barrier *barrier) {
        work_entry_t *we;

        we = (work_entry_t *)SDK_CALLOC(SDK_MEM_ALLOC_LIB_THREAD_POOL,
                                        sizeof(work_entry_t));
        we->cb = cb;
        we->cb_arg = arg;
        we->status_cb = status_cb;
        we->id = id;
        we->barrier = barrier;
        SDK_ATOMIC_FETCH_ADD(&pending_, 1);
        work_entry_push_(we);
        sem_post(&sem_);
    }

    /// \brief run the given function on a new thread. this is a application
    ///        friendly given function to run a single function on a new thread.
    ///        thread will be teared down after the invocation
    /// \param[in] cb application callback invoked from the worker thread
    /// \param[in] arg application callback argument
    /// \param[in] id application specific work id for this work
    /// \param[in] status_cb application work done status callback
    /// \return none
    static void work_post_once(work_cb_t cb, void *arg, work_id_t id,
                               work_status_cb_t status_cb) {
        work_entry_t *we;
        pthread_t thr_id;

        we = (work_entry_t *)SDK_CALLOC(SDK_MEM_ALLOC_LIB_THREAD_POOL,
                                        sizeof(work_entry_t));
        we->cb = cb;
        we->cb_arg = arg;
        we->status_cb = status_cb;
        we->id = id;
        SDK_ATOMIC_FETCH_ADD(&active_nthreads_, 1);
        pthread_create(&thr_id, nullptr, &thread_pool::thread_run_,
                       (void *)we);
    }

    /// \brief initialize by specifying the number of works
    /// \param[in] count number of works
    void barrier_init(work_barrier *barrier, uint32_t count) {
        SDK_ASSERT(barrier);
        barrier->init_(count);
    }

    /// \brief pending returns the number of pending works for the given barrier
    /// \param[in] barrier pointer to the barrier
    /// \return returns the number of pending works
    uint32_t pending(work_barrier *barrier) {
        SDK_ASSERT(barrier);
        return barrier->pending_;
    }

    /// \brief pending returns the number of pending works for the thread pool
    /// \return returns the number of pending works
    uint32_t pending(void) {
        return pending_;
    }

    /// \brief wait for the completion of the work
    /// \param[in] barrier pointer to the barrier
    void barrier_wait(work_barrier *barrier) {
        SDK_ASSERT(barrier);
        barrier->wait_();
    }

    /// \brief waits for all the active threads to exit
    static void wait(void) {
        if (active_nthreads_) {
            SDK_TRACE_DEBUG("Waiting for active threads to exit, pending %u",
                            active_nthreads_);
        }
        while (active_nthreads_) {
            sched_yield();
        }
    }

private:

    // push work into the work queue
    void work_entry_push_(work_entry_t *we) {
        bool rv = lfq_->enqueue(we);
        SDK_ASSERT(rv == true);
    }

    // pop work from the work queue if available
    work_entry_t *work_entry_pop_(void) {
        return (work_entry_t *)lfq_->dequeue();
    }

    /// start the thread pool
    void start_(uint32_t nthreads) {
        SDK_ATOMIC_FETCH_ADD(&active_nthreads_, nthreads);
        SDK_ATOMIC_FETCH_ADD(&nthreads_, nthreads);
        for (uint32_t t = 0; t < nthreads; ++t) {
            pthread_create(&tpool_[t], nullptr, &thread_pool::thread_start_,
                           (void *)this);
        }
    }

    // thread worker loop
    void work_loop_(uint32_t id) {
       while (!stop_) {
            work_entry_t *we = work_get_();
            if (we) {
                sdk_ret_t ret = we->cb(we->cb_arg);
                if (we->status_cb) {
                    we->status_cb(we->id, ret);
                }
                run_count_[id]++;
                SDK_ATOMIC_FETCH_SUB(&pending_, 1);
                if (we->barrier) {
                    we->barrier->done_();
                }
                SDK_FREE(SDK_MEM_ALLOC_LIB_THREAD_POOL, we);
            }
        }
    }

    // get work method for worker threads
    work_entry_t *work_get_(void) {
        work_entry_t *we = nullptr;

        we = work_entry_pop_();
        if (!we) {
            sem_wait(&sem_);
        }
        return we;
    }

    // thread function
    static void *thread_start_(void *arg) {
        pthread_t thread = pthread_self();
        thread_pool *tp = static_cast<thread_pool *>(arg);
        uint32_t id = SDK_ATOMIC_FETCH_ADD(&tp->tid_, 1);

        // if single cpu is assigned to a worker
        if (tp->ncpus_) {
            int ret;
            cpu_set_t cpuset;

            CPU_ZERO(&cpuset);
            CPU_SET(tp->cpu_ids_[id % tp->ncpus_], &cpuset);
            ret = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
            SDK_TRACE_VERBOSE("Setting cpu affinity for id %u to %u is %s", id,
                              tp->cpu_ids_[id % tp->ncpus_], ret == 0 ? "success" : "failure");
        } else if (tp->affinity_set_) {
            pthread_setaffinity_np(thread, sizeof(cpu_set_t), &tp->cpu_mask_);
        }
        tp->work_loop_(id);
        return nullptr;
    }

    // thread start and run the work given in argument
    static void *thread_run_(void *arg) {
        sdk_ret_t ret;
        work_entry_t *we = (work_entry_t *)arg;

        ret = we->cb(we->cb_arg);
        if (we->status_cb) {
            we->status_cb(we->id, ret);
        }
        SDK_FREE(SDK_MEM_ALLOC_LIB_THREAD_POOL, we);
        SDK_ATOMIC_FETCH_SUB(&active_nthreads_, 1);
        return nullptr;
    }

    lfq *lfq_;                              // lock free queue
    pthread_t tpool_[MAX_TPOOL_THREADS];    // pthread ids
    uint32_t run_count_[MAX_TPOOL_THREADS]; // works executed by each thread
    uint32_t nthreads_;                     // number of threads
    uint32_t tid_;                          // used for thread id allocation
    cpu_set_t cpu_mask_;                    // cpu core set to bind to
    sem_t sem_;                             // counting semaphore for wakeup
    bool stop_;                             // stop work signal
    uint32_t cpu_ids_[MAX_TPOOL_THREADS];   // cpu ids derived from given cpu mask
    uint32_t ncpus_;                        // number of valid cpus in the cpu_ids
    bool affinity_set_;                     // thread affinity to be set or not
    uint32_t pending_;                      // how many works are waiting to be completed
    static uint32_t active_nthreads_;       // how many threads are active now
};

}    // namespace lib
}    // namespace sdk

#endif    // __SDK_THREAD_POOL_HPP__
