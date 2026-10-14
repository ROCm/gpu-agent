
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
/// rdc watch thread header
///
//----------------------------------------------------------------------------

#ifndef __AGA_SMI_WATCH_HPP__
#define __AGA_SMI_WATCH_HPP__

#include <set>
#include <unordered_map>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/thread/thread.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/gpuagent/api/internal/aga_gpu_watch.hpp"
#include "nic/gpuagent/api/include/aga_init.hpp"

using std::set;
using std::unordered_map;

/// \defgroup AGA_SMI_WATCH global state for rdc interactions
/// \ingroup AGA
/// @{

namespace aga {

/// \brief per clients interested in each gpu watch
typedef struct gpu_watch_client_info_s {
    /// set of client contexts
    set<aga_gpu_watch_client_ctxt_t *> client_set;
} gpu_watch_client_info_t;

/// \brief gpu watch and client context map with gpu watch id as the key
typedef unordered_map<aga_obj_key_t, gpu_watch_client_info_t,
                      aga_obj_key_hash> gpu_watch_map_t;

/// \brief    per gpu watch subscriber information
typedef struct gpu_watch_subscriber_db_s {
    /// gpu watch map indexed/keyed by gpu watch key
    gpu_watch_map_t gpu_watch_map;
} gpu_watch_subscriber_db_t;

/// \brief    internal structure to hold gpu watch subscriber information
typedef struct gpu_watch_subscriber_info_s {
    /// gpu watch id
    aga_obj_key_t gpu_watch_id;
    /// client context representing the subscriber
    aga_gpu_watch_client_ctxt_t *client_ctxt;
} gpu_watch_subscriber_info_t;

/// \brief    sampling tier of a watch attribute
typedef enum gpu_watch_tier_e {
    /// operator facing signals, sampled every fast period
    GPU_WATCH_TIER_FAST = 0,
    /// counters, sampled every slow period
    GPU_WATCH_TIER_SLOW = 1,
} gpu_watch_tier_t;

/// \brief    a watchable GPU attr and the tier it is sampled at
/// NOTE:
/// the tier is carried per entry because it does not follow attr id order,
/// e.g. GPU_MEMORY_USAGE sits among the fast ids but needs its own smi call
typedef struct gpu_watch_field_s {
    /// watch attribute id
    aga_gpu_watch_attr_id_t id;
    /// sampling tier the attribute belongs to
    gpu_watch_tier_t tier;
} gpu_watch_field_t;

/// \brief    list of all watchable GPU attrs, shared by all SMI backends
extern vector<gpu_watch_field_t> g_watch_field_list;

/// \brief    populate the watchable GPU attr list, called once at watcher init
void smi_watch_field_list_init(void);

/// \brief    set the fast and slow tier sampling periods
/// \param[in] fast_secs    fast tier period in seconds, 0 selects the default
/// \param[in] slow_secs    slow tier period in seconds, 0 selects the default
void smi_watch_set_periods(uint32_t fast_secs, uint32_t slow_secs);

/// \brief    current fast tier sampling period, in seconds
uint32_t smi_watch_fast_period(void);

/// \brief    current slow tier sampling period, in seconds
uint32_t smi_watch_slow_period(void);

/// \@}

}    // namespace aga

#endif    // __AGA_SMI_WATCH_HPP__
