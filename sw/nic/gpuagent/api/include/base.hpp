
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
/// agent header file
///
//----------------------------------------------------------------------------

#ifndef _AGA_API_INCLUDE_BASE_HPP__
#define _AGA_API_INCLUDE_BASE_HPP__

#include <stdio.h>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

// TODO:
// 1. rename this to aga_base.hpp ??
// 2. move this to core/

// 16B keys for the objects
#define OBJ_MAX_KEY_LEN                 ((uint32_t)16)
#define OBJ_MAX_KEY_STR_LEN             36

#define AGA_MAX_STR_LEN                 128
// max. number of GPUs in a compute node
#define AGA_MAX_GPU                     16
// max. number of numa nodes (sockets) in a compute node
#define AGA_MAX_SOCKET                  16
// max. processors per num node (socket)
#define AGA_MAX_PROCESSORS_PER_SOCKET   4

// error codes for objects
// NOTE:
// 1. error code partitioning is as below:
//    1-4095            : reserved for generic ones that can be used across
//                        objects
//    4096-(62K-1)      : PI object specific error codes, per object 128 codes
//                        are reserved
//    62K-(64K-1)       : impl layer generic error codes
//    64K-(64K+64K-1)   : 2^16 - (2^17 - 1) for SMI impl specific error codes
//    2^17-(2^17+64K-1) : for RDC impl specific error codes
typedef enum err_code_e {
    ERR_CODE_NONE                                                 = 0,

    // generic error codes that can be used across objects
    ERR_CODE_GENERIC_MAX                                          = 4095,

    // GPU object specific error codes
    ERR_CODE_GPU_CLOCK_FREQ_RANGE_INVALID                         = 4096,
    ERR_CODE_GPU_OVERDRIVE_OUT_OF_RANGE                           = 4097,
    ERR_CODE_GPU_NUM_CLOCK_FREQ_RANGE_EXCEEDED                    = 4098,
    ERR_CODE_GPU_DUPLICATE_CLOCK_FREQ_RANGE                       = 4099,
    ERR_CODE_GPU_CLOCK_TYPE_FREQ_RANGE_UPDATE_NOT_SUPPORTED       = 4100,
    ERR_CODE_GPU_MAX                                              = 4223,

    // impl layer generic error codes
    ERR_CODE_GENERIC_IMPL_MAX                                     = 65535,

    // smi layer object specific error codes
    ERR_CODE_SMI_GPU_POWER_CAP_OUT_OF_RANGE                       = 65536,
    ERR_CODE_SMI_MAX                                              = 131071,

    // rdc layer object specific error codes
    ERR_CODE_RDC_MAX                                              = 196607,

    // catch all error code
    ERR_CODE_UNKNOWN                                              = 0x1FFFFFFF,
} err_code_t;

// forward declaration for generic API object key
typedef struct aga_obj_key_s aga_obj_key_t;

// generic API object key
struct aga_obj_key_s {
    char id[OBJ_MAX_KEY_LEN + 1];

    void reset(void) {
        memset(id, 0, sizeof(id));
    }
    char *str(void) const {
        char *buf;
        static thread_local uint8_t next_str = 0;
        static thread_local char key_str[4][OBJ_MAX_KEY_STR_LEN + 1];

        buf = key_str[next_str++ & 0x3];
        snprintf(buf, OBJ_MAX_KEY_STR_LEN + 1, "%08x",
                 htonl((*(uint32_t *)&id[0]) & 0xFFFFFFFF));
        buf[8] = '-';
        snprintf(&buf[9], OBJ_MAX_KEY_STR_LEN - 8, "%04x",
                 htons((*(uint16_t *)&id[4]) & 0xFFFF));
        buf[13] = '-';
        snprintf(&buf[14], OBJ_MAX_KEY_STR_LEN - 13, "%04x",
                 htons((*(uint16_t *)&id[6]) & 0xFFFF));
        buf[18] = '-';
        snprintf(&buf[19], OBJ_MAX_KEY_STR_LEN - 18, "%04x",
                 htons((*(uint16_t *)&id[8]) & 0xFFFF));
        buf[23] = '-';
        for (uint32_t i = 0; i < 6; i++) {
            uint32_t off = 24 + (i << 1);
            snprintf(&buf[off], sizeof(buf) - off,"%02x", id[10 + i] & 0xFF);
        }
        buf[OBJ_MAX_KEY_STR_LEN] = '\0';
        return buf;
    }
    bool operator==(const aga_obj_key_t& other) const {
        if (!memcmp(id, other.id, OBJ_MAX_KEY_LEN)) {
            return true;
        }
        return false;
    }
    bool operator!=(const aga_obj_key_t& other) const {
        return !operator==(other);
    }
    bool valid(void) const {
        static char zero_id[OBJ_MAX_KEY_LEN + 1] = { 0 };

        if (memcmp(id, zero_id, OBJ_MAX_KEY_LEN)) {
            return true;
        }
        return false;
    }
};

// invalid object key (all 0s)
static const aga_obj_key_t k_aga_obj_key_invalid = { 0 };

// fnv hash function
static uint32_t
fnv_hash (void *key, uint32_t keylen)
{
    uint8_t     *ptr = (uint8_t *)key;
    uint32_t    i, hv = 2166136261;

    for (i = 0; i < keylen; i++) {
        hv = (hv * 16777619) ^ ptr[i];
    }

    return hv;
}

// helper class for hash computation of the object key
class aga_obj_key_hash {
public:
    std::size_t operator()(const aga_obj_key_t& key) const {
        return fnv_hash((void *)&key, OBJ_MAX_KEY_LEN);
    }
};

/// /brief function to form uuid of GPU given linear GPU index and GPU unique id
///        the uuid is constructed as follows
///        0th byte - 3rd byte  -> device host identifier
///        4th byte             -> 0x42
///        5th byte             -> linear GPU index
///        6th byte - 7th byte  -> 0x4242
///        8th byte - 15th byte -> GPU unique identifier
/// /param[in] gpu_idx          linear GPU index
/// /param[in] gpu_unique_id    unique GPU identifier
/// /return constructed aga_obj_key_t
static inline aga_obj_key_t
gpu_uuid (uint8_t gpu_idx, uint64_t gpu_unique_id)
{
    aga_obj_key_t key = { 0 };
    static uint32_t host_id = gethostid();

    memcpy(&key.id[0], &host_id, sizeof(host_id));
    memset(&key.id[4], 0x42, 1);
    memset(&key.id[5], gpu_idx, 1);
    memset(&key.id[6], 0x42, 1);
    memset(&key.id[7], 0x42, 1);
    memcpy(&key.id[8], &gpu_unique_id, sizeof(gpu_unique_id));
    return key;
}

// given GPU uuid, return its GPU index
static inline uint8_t
gpu_idx_from_uuid (aga_obj_key_t key)
{
    return (key.id[5] & 0xFF);
}

/// type of READ/GET operation
typedef enum obj_get_type_e {
    OBJ_GET_TYPE_NONE          = 0,
    OBJ_GET_TYPE_BATCHED       = 1,
    OBJ_GET_TYPE_STREAMING     = 2,
    OBJ_GET_TYPE_UDS_STREAMING = 3,
} obj_get_type_t;

/// context used during get operations to communicate
/// requirements between svc and api layers
typedef struct obj_get_ctxt_s {
    /// get operation type
    obj_get_type_t type;
    /// proto msg ctxt
    void *msg_ctxt;
    /// streaming writer ctxt
    void *writer_ctxt;
    /// uds streaming fd
    int uds_fd;
    /// filter ctxt
    void *filter_ctxt;
    /// count of objects matching the get request
    bool count;
} obj_get_ctxt_t;

#endif    // __AGA_API_INCLUDE_BASE_HPP__
