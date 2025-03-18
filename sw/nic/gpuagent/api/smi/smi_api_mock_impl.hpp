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
/// smi layer mock impl APIs
///
//----------------------------------------------------------------------------

#ifndef __AGA_API_SMI_API_MOCK_IMPL_HPP__
#define __AGA_API_SMI_API_MOCK_IMPL_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/api/include/aga_event.hpp"
#include "nic/gpuagent/api/smi/smi.hpp"

#define AGA_MOCK_NUM_GPU    16

namespace aga {

/// \brief      function to get GPU handle from event buffer
/// \param[in]  event_buffer    event buffer
/// \param[in]  event_idx       event index
/// \return     GPU handle
aga_gpu_handle_t event_buffer_get_gpu_handle(void *event_buffer,
                                             uint32_t event_idx);

/// \brief      function to get event identifier from event buffer
/// \param[in]  event_buffer    event buffer
/// \param[in]  event_idx       event index
/// \return     event identifier
aga_event_id_t event_buffer_get_event_id(void *event_buffer,
                                         uint32_t event_idx);

/// \brief      function to get event message from event buffer
/// \param[in]  event_buffer    event buffer
/// \param[in]  event_idx       event index
/// \return     event identifier
char *event_buffer_get_message(void *event_buffer, uint32_t event_idx);

/// \brief      get GPU handle given linear GPU index
/// \param[in]  gpu_idx         linear GPU index
/// return      GPU handle
aga_gpu_handle_t gpu_get_handle(uint32_t gpu_idx);

/// \brief      get GPU unique identifier given linear GPU index
/// \param[in]  gpu_idx         linear GPU index
/// return      GPU unique identifier
uint64_t gpu_get_unique_id(uint32_t gpu_idx);

/// \brief      get GPU event
/// return      pointer to event
void *event_get(void);

}    // namespace aga

#endif    // __AGA_API_SMI_API_HPP__
