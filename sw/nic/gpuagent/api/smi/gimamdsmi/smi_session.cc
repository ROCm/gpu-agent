//
// Copyright(C) Advanced Micro Devices, Inc. All rights reserved.
//
// You may not use this software and documentation (if any) (collectively,
// the "Materials") except in compliance with the terms and conditions of
// the Software License Agreement included with the Materials or otherwise as
// set forth in writing and signed by you and an authorized signatory of AMD.
// If you do not have a copy of the Software License Agreement, contact your
// AMD representative for a copy.
//
// You agree that you will not reverse engineer or decompile the Materials,
// in whole or in part, except as allowed by applicable law.
//
// THE MATERIALS ARE DISTRIBUTED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OR
// REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//----------------------------------------------------------------------------
///
/// \file
/// implementation of smi_session - RAII helper around amdsmi_init/shut_down
///
//----------------------------------------------------------------------------

#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/smi/gimamdsmi/smi_session.hpp"
#include "nic/gpuagent/api/smi/gimamdsmi/smi_utils.hpp"

namespace aga {

std::mutex&
smi_session::init_mutex_ (void)
{
    // Meyers' singleton - thread-safe initialization in C++11+
    static std::mutex m;
    return m;
}

uint32_t&
smi_session::refcount_ (void)
{
    // protected by init_mutex_(); modified only while the lock is held
    static uint32_t n = 0;
    return n;
}

smi_session::smi_session ()
    : ok_(false),
      amdsmi_ret_(AMDSMI_STATUS_SUCCESS), ret_(SDK_RET_OK)
{
    // refcount-gated init; the lock is held ONLY for the brief window
    // around the refcount check and the amdsmi_init call (if any); after
    // the lock is released, this and other sessions execute their amdsmi
    // API calls UNLOCKED on a single shared amdsmi state - which is the
    // same concurrency model used by the existing persistent-mode
    // gimamdsmi backend and the amdsmi backend (amdsmi is assumed
    // thread-safe for concurrent reads).
    //
    // Invariants (all maintained under init_mutex_):
    //   refcount_() == 0   <=>  amdsmi is NOT initialized
    //   refcount_() >  0   <=>  amdsmi IS initialized; exactly ONE
    //                           successful amdsmi_init has occurred
    //                           without a matching amdsmi_shut_down
    //   amdsmi_init runs ONLY on the 0 -> 1 transition
    //   amdsmi_shut_down runs ONLY on the 1 -> 0 transition
    std::lock_guard<std::mutex> lk(init_mutex_());
    if (refcount_() == 0) {
        // 0 -> 1 transition: this session triggers the one amdsmi_init
        amdsmi_ret_ = amdsmi_init(AMDSMI_INIT_AMD_GPUS);
        if (amdsmi_ret_ != AMDSMI_STATUS_SUCCESS) {
            ret_ = amdsmi_ret_to_sdk_ret(amdsmi_ret_);
            AGA_TRACE_ERR("Failed to amdsmi_init for smi_session, err {}",
                          amdsmi_ret_);
            // refcount stays 0; the next session ctor will retry init
            return;
        }
    }
    // refcount > 0 reflects amdsmi-is-live; bump and proceed
    refcount_()++;
    ok_ = true;
}

smi_session::~smi_session ()
{
    // if ctor failed (ok_ == false) we never incremented the refcount and
    // we hold nothing to release; return without re-acquiring the lock
    if (!ok_) {
        return;
    }
    std::lock_guard<std::mutex> lk(init_mutex_());
    if (--refcount_() == 0) {
        // 1 -> 0 transition: this was the last live session, shut amdsmi
        // down so /dev/gim-smi0 is released back to other processes
        amdsmi_status_t s = amdsmi_shut_down();
        if (s != AMDSMI_STATUS_SUCCESS) {
            AGA_TRACE_ERR("amdsmi_shut_down failed for smi_session, err {}",
                          s);
        }
    }
}

}    // namespace aga
