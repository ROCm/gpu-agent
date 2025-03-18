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

#include "nic/sdk/include/sdk/base.hpp"
#ifndef ROCM_SMI
#include "nic/third-party/rocm/amd_smi_lib/include/amd_smi/amdsmi.h"

typedef amdsmi_processor_handle aga_gpu_handle_t;
#else
typedef uint32_t aga_gpu_handle_t;
#endif

#endif    // __AGA_API_SMI_HPP__
