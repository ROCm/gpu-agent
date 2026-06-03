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
/// RAII helper that owns one amdsmi_init/amdsmi_shut_down lifetime
///
/// Refcount-gated: amdsmi_init runs only when the first session opens,
/// amdsmi_shut_down only when the last session closes. The lock is held
/// for the brief refcount transition, NOT for the session lifetime, so
/// many concurrent sessions on different threads run their amdsmi API
/// calls in parallel.
///
//----------------------------------------------------------------------------

#ifndef __AGA_API_SMI_GIMAMDSMI_SESSION_HPP__
#define __AGA_API_SMI_GIMAMDSMI_SESSION_HPP__

#include <mutex>
extern "C" {
#include "nic/third-party/rocm/gim_amd_smi_lib/include/amd_smi/amdsmi.h"
}
#include "nic/sdk/include/sdk/base.hpp"

namespace aga {

/// \defgroup AGA_SMI_SESSION - gim amdsmi RAII session helper
/// \ingroup AGA
/// @{

/// \brief RAII wrapper around one amdsmi_init / amdsmi_shut_down lifetime
///
/// On construction, briefly acquires a process-wide mutex to increment a
/// refcount; the first session to enter (refcount 0->1) actually calls
/// amdsmi_init. The lock is then released. On destruction the mutex is
/// briefly re-acquired to decrement the refcount; the last session out
/// (refcount 1->0) calls amdsmi_shut_down.
///
/// While a session is "open" (after ctor, before dtor) the caller holds
/// NO lock - amdsmi API calls from many concurrent sessions execute in
/// parallel.
///
/// Typical use (per-request):
/// \code
///     smi_session s;
///     if (!s.ok()) {
///         return s.ret();
///     }
///     // amdsmi calls here run unlocked
/// \endcode
class smi_session {
public:
    /// \brief ctor - increments refcount; first session also runs amdsmi_init
    smi_session();

    /// \brief dtor - decrements refcount; last session also runs amdsmi_shut_down
    ~smi_session();

    /// non-copyable / non-movable - sessions are tied to a scope
    smi_session(const smi_session &) = delete;
    smi_session &operator=(const smi_session &) = delete;
    smi_session(smi_session &&) = delete;
    smi_session &operator=(smi_session &&) = delete;

    /// \brief    true iff this session is valid (amdsmi is live)
    bool ok(void) const { return ok_; }

    /// \brief    sdk_ret_t form of the underlying amdsmi_init status
    sdk_ret_t ret(void) const { return ret_; }

    /// \brief    raw amdsmi_init status (for logging / mapping)
    amdsmi_status_t amdsmi_ret(void) const { return amdsmi_ret_; }

private:
    /// process-wide guard, held only during refcount transitions and the
    /// amdsmi_init / amdsmi_shut_down calls themselves. NOT held while a
    /// session is open, so concurrent sessions run their queries in parallel.
    static std::mutex& init_mutex_(void);

    /// process-wide active session count; protected by init_mutex_().
    /// amdsmi_init fires on 0->1, amdsmi_shut_down on 1->0.
    static uint32_t& refcount_(void);

    /// true iff this session was opened against a live amdsmi
    bool ok_;
    /// raw amdsmi status from amdsmi_init (set only if this session
    /// triggered the init call)
    amdsmi_status_t amdsmi_ret_;
    /// sdk_ret_t form of amdsmi_ret_
    sdk_ret_t ret_;
};

/// \@}

}    // namespace aga

#endif    // __AGA_API_SMI_GIMAMDSMI_SESSION_HPP__
