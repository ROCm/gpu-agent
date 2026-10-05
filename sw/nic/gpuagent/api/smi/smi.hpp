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
/// common smi header file
///
//----------------------------------------------------------------------------

#ifndef __AGA_API_SMI_HPP__
#define __AGA_API_SMI_HPP__

#include <cstdint>
#include "nic/sdk/include/sdk/base.hpp"
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
#elif defined(WSL_AMD_SMI)
extern "C" {
#include "nic/third-party/rocm/wsl_amd_smi_lib/include/amd_smi/amdsmi.h"
}

typedef amdsmi_processor_handle aga_gpu_handle_t;
#endif

// widen a uint8 NA sentinel (0xFF) to _dst_max_, else pass _val_ through
#define AGA_WIDEN_UINT8_NA(_val_, _dst_max_)                                   \
            (((_val_) == UINT8_MAX) ? (_dst_max_) : (_val_))

// widen a uint16 NA sentinel (0xFFFF) to _dst_max_
#define AGA_WIDEN_UINT16_NA(_val_, _dst_max_)                                  \
            (((_val_) == UINT16_MAX) ? (_dst_max_) : (_val_))

// widen a uint32 NA sentinel (0xFFFFFFFF) to _dst_max_
#define AGA_WIDEN_UINT32_NA(_val_, _dst_max_)                                  \
            (((_val_) == UINT32_MAX) ? (_dst_max_) : (_val_))

// widen an engine-usage NA sentinel; 0xFFFF/INT32_MAX in amd-smi <=10.1,
// UINT32_MAX in 10.2+
#define AGA_WIDEN_INT32_NA(_val_, _dst_max_)                                   \
            ((((_val_) == UINT16_MAX) || ((_val_) == (uint32_t)INT32_MAX) ||   \
              ((_val_) == UINT32_MAX)) ? (_dst_max_) : (_val_))

// widen a uint16 NA sentinel (0xFFFF) to the float NA sentinel (UINT32_MAX)
#define AGA_WIDEN_FLOAT16_NA(_val_)                                            \
            (((_val_) == UINT16_MAX) ? (float)UINT32_MAX : (float)(_val_))

#endif    // __AGA_API_SMI_HPP__
