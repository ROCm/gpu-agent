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
/// smi layer APIs
///
//----------------------------------------------------------------------------

#ifndef __AGA_API_SMI_API_HPP__
#define __AGA_API_SMI_API_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/api/include/aga_init.hpp"
#include "nic/gpuagent/api/include/aga_gpu.hpp"
#include "nic/gpuagent/api/include/aga_task.hpp"
#ifdef ROCM_SMI
typedef uint32_t aga_gpu_handle_t;
#elif defined(GIM_AMD_SMI)
extern "C" {
#include "nic/third-party/rocm/gim_amd_smi_lib/include/amd_smi/amdsmi.h"
}

typedef amdsmi_processor_handle aga_gpu_handle_t;
#elif defined(AMD_SMI)
extern "C" {
#include "nic/third-party/rocm/amd_smi_lib/include/amd_smi/amdsmi.h"
}

typedef amdsmi_processor_handle aga_gpu_handle_t;
#endif

namespace aga {

/// \defgroup AGA_SMI - smi module APIs
/// \ingroup AGA
/// @{

/// \brief    initialize smi layer
/// \param[in] init_params    initialization parameters
/// \return     SDK_RET_OK or error code in case of failure
sdk_ret_t smi_init(aga_api_init_params_t *init_params);

/// \brief    teardown smi layer
/// \return     SDK_RET_OK or error code in case of failure
sdk_ret_t smi_teardown(void);

/// \brief    fill gpu object config specification
/// \param[in] handle    GPU handle (may be stale across amdsmi re-init in
///                      lazy mode; uuid is used to refresh inside)
/// \param[in] uuid      stable GPU UUID
/// \param[out] spec    operational status to be filled
/// \return     SDK_RET_OK or error code in case of failure
sdk_ret_t smi_gpu_fill_spec(aga_gpu_handle_t handle,
                            const aga_obj_key_t *gpu_key,
                            aga_gpu_spec_t *spec);

/// \brief    fill gpu object operational status
/// \param[in] handle    GPU handle (may be stale; uuid refreshes inside)
/// \param[in] uuid      stable GPU UUID
/// \param[in] spec      GPU operational spec
/// \param[out] status    operational status to be filled
/// \param[in] filter     attributes to skip (NULL fetches all attributes)
/// \return     SDK_RET_OK or error code in case of failure
sdk_ret_t smi_gpu_fill_status(aga_gpu_handle_t handle,
                              const aga_obj_key_t *gpu_key,
                              uint32_t id,
                              aga_gpu_spec_t *spec, aga_gpu_status_t *status,
                              const aga_gpu_get_filter_t *filter);

/// \brief    fill gpu object statistics
/// \param[in] handle                   GPU handle (uuid refreshes inside)
/// \param[in] uuid                     stable GPU UUID
/// \param[in] is_partitioned           GPU is partitioned or not
/// \param[in] partition_id             partition id (or 0 in case of
///                                     non-partitioned GPU)
/// \param[in] main_partition_handle    in case of GPU partitions, handle of the
///                                     first partition, else, GPU handle
/// \param[out] stats    gpu object stats to be filled
/// \param[in] filter       attributes to skip (NULL fetches all attributes)
/// \return     SDK_RET_OK or error code in case of failure
sdk_ret_t smi_gpu_fill_stats(aga_gpu_handle_t handle,
                             const aga_obj_key_t *gpu_key,
                             bool is_partitioned,
                             uint32_t partition_id,
                             aga_gpu_handle_t first_partition_handle,
                             aga_gpu_stats_t *stats,
                             const aga_gpu_get_filter_t *filter);

/// \brief    read all the events and invokve the callback provided for each
/// \param[in] cb    callback function pointer
/// \param[in] ctxt  opaque context passed back to the callback
/// \return     SDK_RET_OK or error code in case of failure
sdk_ret_t smi_event_read_all(aga_event_read_cb_t cb, void *ctxt);

/// \brief     reset gpu or a specific gpu setting
/// \param[in] handle    GPU handle
/// \param[in] reset_type    type of the reset
/// \return    SDK_RET_OK or error code in case of failure
sdk_ret_t smi_gpu_reset(aga_gpu_handle_t handle,
                        aga_gpu_reset_type_t reset_type);

/// \brief     update gpu object
/// \param[in] handle       GPU handle (uuid refreshes inside)
/// \param[in] uuid         stable GPU UUID
/// \param[in] spec     spec with updated attributes
/// \param[in] upd_mask updated attributes bitmask
/// \return    SDK_RET_OK or error code in case of failure
sdk_ret_t smi_gpu_update(aga_gpu_handle_t handle,
                         const aga_obj_key_t *gpu_key,
                         aga_gpu_spec_t *spec,
                         uint64_t upd_mask);

/// \brief     fill gpu device topology
/// \param[in] gpu_handle   GPU handle (uuid refreshes inside)
/// \param[in] uuid         stable GPU UUID
/// \param[out] info    GPU topology information
/// \return    SDK_RET_OK or error code in case of failure
sdk_ret_t smi_gpu_fill_device_topology(aga_gpu_handle_t handle,
                                       const aga_obj_key_t *gpu_key,
                                       aga_device_topology_info_t *info);

/// \brief     discover gpu devices
/// \param[out] num_gpus    number of GPU devices discovered
/// \param[out] gpu         profile of GPU devices discovered
/// \return    SDK_RET_OK or error code in case of failure
sdk_ret_t smi_discover_gpus(uint32_t *num_gpu, aga_gpu_profile_t *gpu);

/// \brief     function to compute parent GPU's uuid
/// \param[in]  gpu_handle    handle of GPU device (uuid refreshes inside)
/// \param[in]  uuid          stable UUID of the child GPU; used to refresh
///                           gpu_handle in lazy mode
/// \param[out] parent_key    computed uuid of parent GPU
sdk_ret_t smi_get_parent_gpu_uuid(aga_gpu_handle_t gpu_handle,
                                  const aga_obj_key_t *gpu_key,
                                  aga_obj_key_t *parent_key);

/// \brief function to get number of bad pages for GPU
/// \param[in]  gpu             GPU object
/// \param[out] num_bad_pages   number of bad pages
/// \return SDK_RET_OK or error code in case of failure
sdk_ret_t smi_gpu_get_bad_page_count(void *gpu_obj,
                                     uint32_t *num_bad_pages);

/// \brief function to get GPU bad page records
/// \param[in]  gpu           GPU object
/// \param[in]  num_bad_pages number of bad pages
/// \param[out] records       GPU bad page records
/// \return SDK_RET_OK or error code in case of failure
sdk_ret_t smi_gpu_get_bad_page_records(void *gpu_obj,
                                       uint32_t num_bad_pages,
                                       aga_gpu_bad_page_record_t *records);

/// \brief function to fill immutable attributes in GPU spec and status
/// \param[in]  gpu_handle    handle of GPU device (uuid refreshes inside)
/// \param[in]  uuid          stable GPU UUID
/// \param[out] spec          GPU spec
/// \param[out] status        GPU status
/// \return SDK_RET_OK or error code in case of failure
sdk_ret_t smi_gpu_init_immutable_attrs(aga_gpu_handle_t gpu_handle,
                                       const aga_obj_key_t *gpu_key,
                                       aga_gpu_spec_t *spec,
                                       aga_gpu_status_t *status);

/// \brief    initialize the immutable attributes, the attributes also read on
///           every get, and the config attributes
/// \param[in]  gpu_handle    GPU handle
/// \param[in]  gpu_key       stable GPU UUID
/// \param[out] spec          GPU spec
/// \param[out] status        GPU status
/// \return     SDK_RET_OK or error code in case of failure
sdk_ret_t smi_gpu_init_attrs(aga_gpu_handle_t gpu_handle,
                             const aga_obj_key_t *gpu_key,
                             aga_gpu_spec_t *spec,
                             aga_gpu_status_t *status);

/// \brief function to get GPU CPER entries
/// \param[in]  gpu_handle    handle of GPU device (uuid refreshes inside)
/// \param[in]  uuid          stable GPU UUID
/// \param[in]  severity      severity of CPER entries to be retrieved
///                           AGA_CPER_SEVERITY_NONE implies all
/// \param[out] info          GPU CPER information
/// \return SDK_RET_OK or error code in case of failure
sdk_ret_t smi_gpu_get_cper_entries(aga_gpu_handle_t gpu_handle,
                                   const aga_obj_key_t *gpu_key,
                                   aga_cper_severity_t severity,
                                   aga_cper_info_t *info);

/// \@}

}    // namespace aga

#endif    // __AGA_API_SMI_API_HPP__
