
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
/// this file captures memory related helpers
///
//----------------------------------------------------------------------------

#ifndef __AGA_API_MEM_HPP__
#define __AGA_API_MEM_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/timestamp.hpp"

#define AGA_DELAY_DELETE_SECS       2

namespace aga {

/// \brief wrapper function to delay delete elements
///
/// \param[in] obj_id       object identifier
/// \param[in] elem         element to free
/// \param[in] timeout_secs optional timeout in seconds to delay delete the
///                         element
/// \return #SDK_RET_OK on success, failure status code on error
///
/// \remark
///   - currently delay delete timeout is AGA_DELAY_DELETE_MSECS, it is
///     expected that other thread(s) using (a pointer to) this object should
///     be done using this object within this timeout or else this memory can
///     be freed and allocated for other objects and can result in corruptions.
///     essentially, AGA_DELAY_DELETE is assumed to be infinite
sdk_ret_t delay_delete(uint32_t obj_id, void *elem,
                       uint64_t timeout_secs = AGA_DELAY_DELETE_SECS);

}    // namespace aga

#endif    // __AGA_CORE_MEM_HPP__
