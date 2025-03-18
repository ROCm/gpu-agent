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


#ifndef __TWHEEL_HPP__
#define __TWHEEL_HPP__

#include <time.h>
#include "include/sdk/base.hpp"
#include "include/sdk/lock.hpp"
#include "lib/slab/slab.hpp"
#include "include/sdk/timestamp.hpp"

using sdk::lib::slab;

namespace sdk {
namespace lib {

#define TWHEEL_DEFAULT_SLICE_DURATION            250      // in msecs
#define TWHEEL_DEFAULT_SLICE_DURATION_IN_SECS    (0.25)   // in seconds
// time duration for one full rotation around the wheel is 2 hours
#define TWHEEL_DEFAULT_DURATION                  (2 * 60 * TIME_MSECS_PER_MIN)

typedef void (*twheel_cb_t)(void *timer, uint32_t timer_id, void *ctxt);
typedef struct twentry_s twentry_t;
struct twentry_s {
    uint32_t       timer_id_;    // unique (in app's space) timer id
    uint32_t       timeout_;     // timeout (in ms) of this timer
    uint8_t        periodic_:1;  // periodic timer
    uint8_t        valid_:1;     // timer is valid or not
    void           *ctxt_;       // user provided context
    twheel_cb_t    cb_;          // callback to invoke at timeout
    uint16_t       nspins_;      // age this out when nspins == 0
    uint32_t       slice_;       // slice this entry is sitting in
    twentry_t      *next_;       // next entry in the list
    twentry_t      *prev_;       // previous entry in the list
} __PACK__;

// one slice of the timer wheel
typedef struct tw_slice_s {
    sdk_spinlock_t    slock_;          // lock for thread safety
    twentry_t         *slice_head_;    // slice head
}  __PACK__ tw_slice_t;

//------------------------------------------------------------------------------
// NOTE: all intervals are expressed in milli seconds
//------------------------------------------------------------------------------
class twheel {
public:
    static twheel *factory(uint64_t slice_intvl=TWHEEL_DEFAULT_SLICE_DURATION,
                           uint32_t wheel_duration=TWHEEL_DEFAULT_DURATION,
                           bool thread_safe=false);
    static void destroy(twheel *twh);
    void tick(uint32_t msecs_elapsed);
    void *add_timer(uint32_t timer_id, uint64_t timeout, void *ctxt,
                    twheel_cb_t cb, bool periodic, uint64_t initial_delay);
    void *del_timer(void *timer);
    void *upd_timer(void *timer, uint64_t timeout, bool periodic, void *ctxt);
    void *upd_timer_ctxt(void *timer, void *ctxt);
    uint64_t get_timeout_remaining(void *timer);
    inline bool timer_valid(void *timer) {
        twentry_t    *twentry;

        if (timer == NULL) {
            return false;
        }
        twentry = static_cast<twentry_t *>(timer);
        if (twentry->valid_) {
            return true;
        }
        return false;
    }
    uint32_t num_entries(void) const { return num_entries_; }
    slab *get_slab(void) { return twentry_slab_; };

private:

    slab          *twentry_slab_;    // slab memory for timer wheel entry
    uint64_t      slice_intvl_;      // per slice interval in msecs
    bool          thread_safe_;      // TRUE if this is thread_safe instance
    uint32_t      nslices_;          // # of slices in this wheel
    tw_slice_t    *twheel_;          // timer wheel itself
    uint32_t      curr_slice_;       // current slice we are processing
    uint32_t      num_entries_;      // no. of entries in the wheel

private:
    twheel() {};
    ~twheel();
    sdk_ret_t init(uint64_t slice_intvl, uint32_t wheel_duration,
                   bool thread_safe);
    void init_twentry_(twentry_t *twentry, uint32_t timer_id, uint64_t timeout,
                       bool periodic, void *ctxt, twheel_cb_t cb, uint32_t slice);
    void free_to_slab_(void *timer) {
        twentry_slab_->free(timer);
    }
    uint32_t next_slice_(uint64_t timeout, uint32_t entry_slice, bool update);
    void delay_delete_(twentry_t *twentry);

    // NOTE: this internal API is called under twheel slice lock
    inline void insert_timer_(twentry_t *twentry) {
#if SDK_TWHEEL_DEBUG
        SDK_TRACE_ERR("insert timer id : %d, timeout : %d, valid : %d, "
                      "slice: %d, periodic: %d, twentry: %p",
                      twentry->timer_id_, twentry->timeout_, twentry->valid_,
                      twentry->slice_, twentry->periodic_, twentry);
#endif
	twentry_t *cur_entry = twheel_[twentry->slice_].slice_head_;
        twentry->next_ = cur_entry;
        if (cur_entry != NULL) {
            cur_entry->prev_ = twentry;
        }
        twheel_[twentry->slice_].slice_head_ = twentry;
        twentry->valid_ = TRUE;
        num_entries_++;
#if SDK_TWHEEL_DEBUG
        SDK_TRACE_ERR(" slice : %d, entry is : %p", twentry->slice_,
                      twheel_[twentry->slice_].slice_head_);
#endif
    }
    inline void unlink_timer_(twentry_t *twentry) {
        if (twentry == NULL) {
            SDK_TRACE_ERR("twentry null");
            return;
        }

        if (twentry->next_) {
#if SDK_TWHEEL_DEBUG
            SDK_TRACE_ERR("next is not null next : %p, next_prev : %p "
                          "prev : %p", twentry->next_, twentry->next_->prev_,
                          twentry->prev_);
#endif
            // removing last entry in the list
            twentry->next_->prev_ = twentry->prev_;
        }
        if (twentry->prev_ == NULL) {
            // removing the head of the list
#if SDK_TWHEEL_DEBUG
            SDK_TRACE_ERR("prev is null in slice %d  slice_head %p t->n : %p",
                          twentry->slice_, twheel_[twentry->slice_].slice_head_,
                          twentry->next_);
#endif
            twheel_[twentry->slice_].slice_head_ = twentry->next_;
        } else {
#if SDK_TWHEEL_DEBUG
            SDK_TRACE_ERR("next is not null t->p->n : %p, t->n : %p ",
                          twentry->prev_->next_, twentry->next_);
#endif
            twentry->prev_->next_ = twentry->next_;
        }
        num_entries_--;
    }

    // NOTE: this internal API is called under twheel slice lock
    inline void remove_timer_(twentry_t *twentry) {
#if SDK_TWHEEL_DEBUG
        SDK_TRACE_ERR("timer id: %d, timeout: %d, valid: %d, twentry: %p",
                      twentry->timer_id_, twentry->timeout_, twentry->valid_,
                      twentry);
#endif

        if (twentry->valid_ == FALSE) {
            return;
        }
        unlink_timer_(twentry);
        twentry->valid_ = FALSE;
    }

    inline twentry_t *last_timer_in_slice(tw_slice_t *slice) {
        twentry_t *last = slice->slice_head_;
        while(last != NULL && last->next_ != NULL) {
            last = last->next_;
        }
        return last;
    }



    void upd_timer_(twentry_t *twentry, uint64_t timeout, bool periodic);
};

}    // namespace lib
}    // namespace sdk

#endif    // __TWHEEL_HPP__

