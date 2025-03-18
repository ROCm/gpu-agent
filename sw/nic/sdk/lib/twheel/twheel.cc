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
#include <unistd.h>
#include "include/sdk/mem.hpp"
#include "lib/twheel/twheel.hpp"

// #define SDK_TWHEEL_DEBUG 1

namespace sdk {
namespace lib {

#define TWHEEL_LOCK_SLICE(slice)                                       \
{                                                                      \
    if (thread_safe_) {                                                \
        SDK_SPINLOCK_LOCK(&twheel_[(slice)].slock_);                   \
    }                                                                  \
}

#define TWHEEL_UNLOCK_SLICE(slice)                                     \
{                                                                      \
    if (thread_safe_) {                                                \
        SDK_SPINLOCK_UNLOCK(&twheel_[(slice)].slock_);                 \
    }                                                                  \
}

#define TWHEEL_DELAY_DELETE    2000    // 2 sec delay delete timeout

//------------------------------------------------------------------------------
// init function for the timer wheel
//------------------------------------------------------------------------------
sdk_ret_t
twheel::init(uint64_t slice_intvl, uint32_t wheel_duration, bool thread_safe)
{
    uint32_t    i;

    twentry_slab_ = slab::factory("twheel", SDK_SLAB_ID_TWHEEL,
                                  sizeof(twentry_t), 256,
                                  thread_safe, true, false);
    if (twentry_slab_ == NULL) {
        return SDK_RET_OOM;
    }
    slice_intvl_ = slice_intvl;
    thread_safe_ = thread_safe;
    nslices_ = wheel_duration/slice_intvl;
    twheel_ = (tw_slice_t *)SDK_CALLOC(SDK_MEM_ALLOC_LIB_TWHEEL,
                                       nslices_ * sizeof(tw_slice_t));
    if (twheel_ == NULL) {
        slab::destroy(twentry_slab_);
        return SDK_RET_OOM;
    }
    if (thread_safe_) {
        for (i = 0; i < nslices_; i++) {
            SDK_SPINLOCK_INIT(&twheel_[i].slock_, PTHREAD_PROCESS_PRIVATE);
        }
    }
    curr_slice_ = 0;
    num_entries_ = 0;
    return SDK_RET_OK;
}

//------------------------------------------------------------------------------
// factory method
//------------------------------------------------------------------------------
twheel *
twheel::factory(uint64_t slice_intvl, uint32_t wheel_duration,
                bool thread_safe)
{
    void      *mem;
    twheel    *new_twheel = NULL;

    if ((slice_intvl == 0) || (wheel_duration == 0) ||
        (wheel_duration <= slice_intvl)) {
        return NULL;
    }
    mem = SDK_CALLOC(SDK_MEM_ALLOC_LIB_TWHEEL, sizeof(twheel));
    if (!mem) {
        return NULL;
    }
    new_twheel = new (mem) twheel();
    if (new_twheel->init(slice_intvl, wheel_duration, thread_safe) !=
            SDK_RET_OK) {
        new_twheel->~twheel();
        SDK_FREE(SDK_MEM_ALLOC_LIB_TWHEEL, mem);
        return NULL;
    }
    return new_twheel;
}

//------------------------------------------------------------------------------
// destructor
//------------------------------------------------------------------------------
twheel::~twheel()
{
    uint32_t    i;

    if (twentry_slab_) {
        slab::destroy(twentry_slab_);
    }
    if (thread_safe_) {
        for (i = 0; i < nslices_; i++) {
            SDK_SPINLOCK_DESTROY(&twheel_[i].slock_);
        }
    }
    SDK_FREE(SDK_MEM_ALLOC_LIB_TWHEEL, twheel_);
}

void
twheel::destroy(twheel *twh)
{
    if (!twh) {
        return;
    }
    twh->~twheel();
    SDK_FREE(SDK_MEM_ALLOC_LIB_TWHEEL, twh);
}

uint32_t
twheel::next_slice_(uint64_t timeout, uint32_t entry_slice, bool update) {
    uint32_t rem, num_slices, slice;

    rem  = timeout % (nslices_ * slice_intvl_);
    num_slices = rem / slice_intvl_;
    if (num_slices == 0) {
        num_slices = 1;
    }
    slice = ((curr_slice_ +  num_slices) % nslices_);
    // if new slice and current slice are equal, requires recursive locks as the
    // update are called with current slice lock held. so incrementing the slice
    // once. there will be one slice difference in timeout.
    if (update && (slice == entry_slice)) {
        slice = ((slice +  1) % nslices_);
    }
    return slice;
}

//------------------------------------------------------------------------------
// initialize a given timer wheel entry
//------------------------------------------------------------------------------
void
twheel::init_twentry_(twentry_t *twentry, uint32_t timer_id, uint64_t timeout,
                      bool periodic, void *ctxt, twheel_cb_t cb, uint32_t slice)
{

#if SDK_TWHEEL_DEBUG
    SDK_TRACE_VERBOSE("init timer id : %u, timeout : %u, periodic : %d, "
                      "twentry : %p, slice : %u", timer_id, timeout, periodic,
                      twentry, slice);
#endif
    twentry->timer_id_ = timer_id;
    twentry->timeout_ = timeout;
    twentry->periodic_ = periodic;
    twentry->ctxt_ = ctxt;
    twentry->cb_ = cb;
    twentry->valid_ = FALSE;
    twentry->nspins_ = timeout/(nslices_ * slice_intvl_);
    twentry->slice_ = slice;
    twentry->next_ = twentry->prev_ = NULL;
}

//------------------------------------------------------------------------------
// NOTE: this internal API is called under twheel slice lock
//------------------------------------------------------------------------------
void
twheel::upd_timer_(twentry_t *twentry, uint64_t timeout, bool periodic) {
    uint32_t slice;

    remove_timer_(twentry);
    slice = next_slice_(timeout, twentry->slice_, true);
    TWHEEL_LOCK_SLICE(slice);
    init_twentry_(twentry, twentry->timer_id_, timeout,
                  periodic, twentry->ctxt_, twentry->cb_, slice);
    insert_timer_(twentry);
    TWHEEL_UNLOCK_SLICE(slice);
}

//------------------------------------------------------------------------------
// enqueue the timer for delay delete
// NOTE: assumption here is that the timer entry is already removed from the
//       timer wheel
//       the lock taken by this function is corresponding to the slice that is
//       TWHEEL_DELAY_DELETE msecs from now
//------------------------------------------------------------------------------
void
twheel::delay_delete_(twentry_t *twentry)
{
    uint32_t slice;

#if SDK_TWHEEL_DEBUG
    SDK_TRACE_VERBOSE("timer id : %d, timeout : %d, twentry : %p",
                      twentry->timer_id_, twentry->timeout_, twentry);
#endif
    slice = next_slice_(TWHEEL_DELAY_DELETE, twentry->slice_, true);
    TWHEEL_LOCK_SLICE(slice);
    init_twentry_(twentry, twentry->timer_id_,
                  TWHEEL_DELAY_DELETE, false, NULL, NULL, slice);
    insert_timer_(twentry);
    twentry->valid_ = FALSE;
    TWHEEL_UNLOCK_SLICE(slice);
}

//------------------------------------------------------------------------------
// add a timer entry to the wheel
//------------------------------------------------------------------------------
void *
twheel::add_timer(uint32_t timer_id, uint64_t timeout, void *ctxt,
                  twheel_cb_t cb, bool periodic, uint64_t initial_delay)
{
    twentry_t    *twentry;
    uint32_t     slice = next_slice_(initial_delay + timeout, 0, false);

    twentry = static_cast<twentry_t *>(this->twentry_slab_->alloc());
    if (twentry == NULL) {
        return NULL;
    }
#if SDK_TWHEEL_DEBUG
    SDK_TRACE_VERBOSE("added timer id : %d, timeout : %d, periodic : %d, "
                      "twentry : %p", timer_id, timeout, periodic, twentry);
#endif

    init_twentry_(twentry, timer_id, timeout, periodic, ctxt, cb, slice);

    TWHEEL_LOCK_SLICE(twentry->slice_);
    insert_timer_(twentry);
    TWHEEL_UNLOCK_SLICE(twentry->slice_);

    return twentry;
}

//------------------------------------------------------------------------------
// delete timer entry from the timer wheel and return the application context
//------------------------------------------------------------------------------
void *
twheel::del_timer(void *timer)
{
    twentry_t    *twentry;
    void         *ctxt;
    uint32_t      slice = 0;

    if (timer == NULL) {
        return NULL;
    }
    twentry = static_cast<twentry_t *>(timer);
    ctxt = twentry->ctxt_;
#if THWEEL_DEBUG
    SDK_TRACE_VERBOSE("del timer id : %u, timeout : %u, periodic : %u, "
                      "twentry: %p", twentry->timer_id_, twentry->timeout_,
                      twentry->periodic_, twentry);
#endif
    while(1) {
        slice = twentry->slice_;
        TWHEEL_LOCK_SLICE(slice);
        // slice might have moved by upd_timer_ if the timer is periodic
        if (twentry->slice_ == slice) {
            break;
        }
        TWHEEL_UNLOCK_SLICE(slice);
    }
    if (twentry->valid_ == FALSE) {
        SDK_TRACE_ERR("Timer has not been added yet, timer 0x%lx",
                      (long)twentry);
        TWHEEL_UNLOCK_SLICE(slice);
        return ctxt;
    }
    remove_timer_(twentry);
    TWHEEL_UNLOCK_SLICE(slice);
    delay_delete_(twentry);
    return ctxt;
}

//------------------------------------------------------------------------------
// Get how much timeout remaining for this timer.
//------------------------------------------------------------------------------
uint64_t
twheel::get_timeout_remaining(void *timer)
{
    twentry_t    *twentry;
    uint64_t      timeout;

    if (timer == NULL) {
        return 0;
    }
    twentry = static_cast<twentry_t *>(timer);

    timeout = twentry->nspins_ * (nslices_ * slice_intvl_) +
            ((twentry->slice_ - curr_slice_ + nslices_) % nslices_) * slice_intvl_;

    return timeout;
}

//------------------------------------------------------------------------------
// update timer's caontext
//------------------------------------------------------------------------------
void *
twheel::upd_timer_ctxt(void *timer, void *ctxt)
{
    twentry_t        *twentry;

    if (timer == NULL) {
        return NULL;
    }
    twentry = static_cast<twentry_t *>(timer);
    twentry->ctxt_ = ctxt;

    return twentry;
}

//------------------------------------------------------------------------------
// update a given timer wheel entry
//------------------------------------------------------------------------------
void *
twheel::upd_timer(void *timer, uint64_t timeout, bool periodic, void *ctxt)
{
    twentry_t        *twentry;
    uint32_t         slice;
    uint32_t         entry_slice;

    if (timer == NULL) {
        return NULL;
    }
    twentry = static_cast<twentry_t *>(timer);

    // remove this entry from current slice
    while(1) {
        entry_slice = twentry->slice_;
        TWHEEL_LOCK_SLICE(entry_slice);
        // this entry slice might have been moved by upd_timer when the timer
        // is periodic
        if (twentry->slice_ == entry_slice) {
            break;
        }
        TWHEEL_UNLOCK_SLICE(entry_slice);
    }
    if (twentry->valid_ == FALSE) {
        SDK_TRACE_ERR("Timer has not been added yet, timer 0x%lx", (long)twentry);
        TWHEEL_UNLOCK_SLICE(entry_slice);
        return twentry;
    }
    remove_timer_(twentry);

    // re-init with updated params
    slice = next_slice_(timeout, entry_slice, true);
    TWHEEL_LOCK_SLICE(slice);
    init_twentry_(twentry, twentry->timer_id_, timeout,
                  periodic, ctxt, twentry->cb_, slice);
    // re-insert in the right slice
    insert_timer_(twentry);
    TWHEEL_UNLOCK_SLICE(slice);
    TWHEEL_UNLOCK_SLICE(entry_slice);

    return twentry;
}

//------------------------------------------------------------------------------
// timer wheel tick routine that drives the wheel, expected to be called by user
// of the timer wheel instance (ideally once every tick), tick is assumed  to be
// same as timer wheel slice interval
//------------------------------------------------------------------------------
void
twheel::tick(uint32_t msecs_elapsed)
{
    uint32_t     nslices = 0;
    twentry_t    *twentry, *prev_entry;

    // check if full slice interval has elapsed since last invokation
    if (msecs_elapsed < slice_intvl_) {
        return;
    }

    // compute the number of slices to walk from current slice
    nslices = msecs_elapsed/slice_intvl_;
    SDK_ASSERT(nslices >= 1);

    // process all the timer events from current slice
    do {
        TWHEEL_LOCK_SLICE(curr_slice_);
        twentry = last_timer_in_slice(&twheel_[curr_slice_]);
#if SDK_TWHEEL_DEBUG
        SDK_TRACE_DEBUG("curr_slice_ : %d", curr_slice_);
#endif
        while (twentry) {
            if (twentry->valid_ == FALSE) {
                // delay deleting memory for already freed timer
                prev_entry = twentry->prev_;
#if SDK_TWHEEL_DEBUG
                SDK_TRACE_VERBOSE("free to slab timer id : %d, timeout : %d, "
                                  "periodic : %d, twentry : %p",
                                  twentry->timer_id_, twentry->timeout_,
                                  twentry->periodic_, twentry);
#endif
                unlink_timer_(twentry);
                free_to_slab_(twentry);
                twentry = prev_entry;
            } else {
                if (twentry->nspins_) {
#if SDK_TWHEEL_DEBUG
                    SDK_TRACE_VERBOSE("spin timer for timer id : %d, "
                                      "twentry : %p", twentry->timer_id_,
                                      twentry);
#endif
                    // revisit this after one more full spin
                    twentry->nspins_ -= 1;
                    twentry = twentry->prev_;
                } else {
                    // cache the next entry, in case callback function does
                    // something to this timer (it shouldn't ideally)
                    prev_entry = twentry->prev_;
#if SDK_TWHEEL_DEBUG
                    SDK_TRACE_VERBOSE("calling the callback for timer id : %d, "
                                      "timeout : %d, periodic : %d, "
                                      "twentry : %p", twentry->timer_id_,
                                      twentry->timeout_, twentry->periodic_,
                                      twentry);
#endif
                    twentry->cb_(twentry, twentry->timer_id_,
                                 twentry->ctxt_);
                    if (twentry->periodic_) {
                        // re-insert this timer
                        if (twentry->valid_) {
                            // in the unlock-lock window, this timer got
                            // deleted, no need to reinsert (this is mostly
                            // sitting in the delay delete state)
#if SDK_TWHEEL_DEBUG
                            SDK_TRACE_VERBOSE("upd timer for timer id : %d, "
                                              "twentry : %p",
                                              twentry->timer_id_, twentry);
#endif
                            upd_timer_(twentry, twentry->timeout_, true);
                        }
                    } else {
                        if (twentry->valid_) {
                            // delete this timer, if its not already deleted
#if SDK_TWHEEL_DEBUG
                            SDK_TRACE_VERBOSE("remove non periodic timer id : "
                                              "%u, timeout : %u, periodic : "
                                              "%u, twentry : %p",
                                              twentry->timer_id_,
                                              twentry->timeout_,
                                              twentry->periodic_, twentry);
#endif
                            remove_timer_(twentry);
#if SDK_TWHEEL_DEBUG
                            SDK_TRACE_VERBOSE("add to delay del timer id : %u, "
                                              "timeout : %u, periodic : %u, "
                                              "twentry : %p prev entry : %p",
                                              twentry->timer_id_,
                                              twentry->timeout_,
                                              twentry->periodic_, twentry,
                                              prev_entry);
#endif
                            delay_delete_(twentry);
                        }
                    }
                    twentry = prev_entry;
                }
            }
        }
        TWHEEL_UNLOCK_SLICE(curr_slice_);
        curr_slice_ = (curr_slice_ + 1)%nslices_;
    } while (--nslices);
}

}    // namespace lib
}    // namespace sdk

