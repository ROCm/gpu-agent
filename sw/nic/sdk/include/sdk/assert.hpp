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
/// assert definitions
///
//----------------------------------------------------------------------------

#ifndef __SDK_ASSERT_HPP__
#define __SDK_ASSERT_HPP__

#include <assert.h>
#include "lib/logger/logger.h"

#ifdef __cplusplus
namespace sdk {
#endif

#define __ASSERT__(x)       assert(x)

#define SDK_ASSERT_RETURN(cond, rv)                        \
do {                                                       \
    if (unlikely(!(cond))) {                               \
        SDK_TRACE_ERR("ASSERT FAILURE(" #cond ")");        \
        __ASSERT__(FALSE);                                 \
        return rv;                                         \
    }                                                      \
} while (FALSE)

#define SDK_ASSERT_TRACE_RETURN(cond, rv, args...)         \
do {                                                       \
    if (unlikely(!(cond))) {                               \
        SDK_TRACE_ERR("ASSERT FAILURE(" #cond ")");        \
        SDK_TRACE_ERR(args);                               \
        __ASSERT__(FALSE);                                 \
        return rv;                                         \
    }                                                      \
} while (FALSE)


#define SDK_ASSERT_RETURN_VOID(cond)                       \
do {                                                       \
    if (unlikely(!(cond))) {                               \
        SDK_TRACE_ERR("ASSERT FAILURE(" #cond ")");        \
        __ASSERT__(FALSE);                                 \
        return;                                            \
    }                                                      \
} while (FALSE)

#define SDK_ASSERT_GOTO(cond, label)                       \
do {                                                       \
    if (unlikely(!(cond))) {                               \
        SDK_TRACE_ERR("ASSERT FAILURE(" #cond ")");        \
        goto label;                                        \
    }                                                      \
} while (FALSE)

#define SDK_ASSERT_TRACE_GOTO(cond, label, args...)        \
do {                                                       \
    if (unlikely(!(cond))) {                               \
        SDK_TRACE_ERR("ASSERT FAILURE(" #cond ")");        \
        SDK_TRACE_ERR(args);                               \
        __ASSERT__(FALSE);                                 \
        goto label;                                         \
    }                                                      \
} while (FALSE)

#define SDK_ASSERT(cond)        __ASSERT__(cond)

#define SDK_ABORT(cond)                                    \
do {                                                       \
    if (unlikely(!(cond))) {                               \
        abort();                                           \
    }                                                      \
} while (FALSE)

#define SDK_ABORT_TRACE(cond, args...)                     \
do {                                                       \
    if (unlikely(!(cond))) {                               \
        SDK_TRACE_ERR(args);                               \
        abort();                                           \
    }                                                      \
} while (FALSE)

#ifdef __cplusplus
}    // namespace sdk
#endif   // __cplusplus

#endif    // __SDK_ASSERT_HPP__

