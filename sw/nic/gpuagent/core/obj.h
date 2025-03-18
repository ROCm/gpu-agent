
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
/// object identifier definitions
//----------------------------------------------------------------------------

#ifndef __AGA_CORE_OBJ_H__
#define __AGA_CORE_OBJ_H__

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/// \brief    object identifiers
typedef enum obj_id_e {
    AGA_OBJ_ID_NONE      = 0,
    /// GPU object
    AGA_OBJ_ID_GPU       = 1,
    /// generic task object
    AGA_OBJ_ID_TASK      = 2,
    /// GPU watch object
    AGA_OBJ_ID_GPU_WATCH = 3,
    AGA_OBJ_ID_MAX
} obj_id_t;

/// \brief    max. size of API object name
#define AGA_OBJ_NAME_SIZE        32


#ifdef __cplusplus
}
#endif

#endif    // __AGA_CORE_OBJ_H__
