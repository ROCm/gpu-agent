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
/// smi events header
///
//----------------------------------------------------------------------------

#ifndef __AGA_SMI_EVENTS_HPP__
#define __AGA_SMI_EVENTS_HPP__

#include <unordered_map>
#include <set>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/thread/thread.hpp"
#include "nic/sdk/include/sdk/timestamp.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/sdk/include/sdk/lock.hpp"
#include "nic/gpuagent/api/include/aga_init.hpp"
#include "nic/gpuagent/api/include/aga_event.hpp"
#include "nic/gpuagent/api/internal/aga_event.hpp"

using std::set;
using std::unordered_map;

/// \defgroup AGA_SMI_EVENTS global state for smi interactions
/// \ingroup AGA
/// @{

namespace aga {

/// \brief per client info maintained for each GPU event
typedef struct gpu_event_client_info_s {
    /// set of client contexts
    set<aga_event_client_ctxt_t *> client_set;
    /// last notified timestamp
    timespec_t last_ntfn_ts;
} gpu_event_client_info_t;

/// \brief    per event information
typedef struct gpu_event_record_s {
    /// time when the event happened
    timespec_t timestamp;
    /// event description
    char message[AGA_MAX_EVENT_STR + 1];
    /// clients interested in this event and associated state
    gpu_event_client_info_t client_info;

    /// constructor
    gpu_event_record_s() {
        memset(&timestamp, 0, sizeof(timestamp));
        memset(message, 0, sizeof(message));
    }
} gpu_event_record_t;

/// \brief event map with event id as the key
typedef unordered_map<aga_event_id_t, gpu_event_record_t> gpu_event_map_t;

/// \brief    per GPU current event information
typedef struct gpu_event_db_entry_s {
    // TODO:
    // this lock is not needed if we handle everything in the backend thread,
    // revisit later !!
    /// spinlock to avoid race conditions
    sdk_spinlock_t slock;
    /// event map indexed/keyed by event id
    gpu_event_map_t event_map;
} gpu_event_db_entry_t;

/// \brief    internal structure to hold event listener information
typedef struct aga_event_listener_info_s {
    /// GPU id
    uint32_t gpu_id;
    /// event identifier
    aga_event_id_t event;
    /// client context representing the listener
    aga_event_client_ctxt_t *client_ctxt;
} aga_event_listener_info_t;

/// \@}

}    // namespace aga

#endif    // __AGA_SMI_EVENTS_HPP__
