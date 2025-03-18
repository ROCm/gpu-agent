
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
/// GPU watch definitions
///
//----------------------------------------------------------------------------

#ifndef __API_INCLUDE_AGA_GPU_WATCH_HPP__
#define __API_INCLUDE_AGA_GPU_WATCH_HPP__

#include "nic/sdk/include/sdk/timestamp.hpp"
#include "nic/gpuagent/api/include/base.hpp"

/// max. number of GPU watch objects
#define AGA_MAX_GPU_WATCH                 128
/// max. string length of string watch attribute
#define AGA_MAX_WATCH_ATTR_STR            256
#define AGA_MAX_GPU_WATCH_CLIENT_STR      128

/// \brief    GPU attributes that are watchable
typedef enum aga_gpu_watch_attr_id_e {
    AGA_GPU_WATCH_ATTR_ID_INVALID             = 0,
    AGA_GPU_WATCH_ATTR_ID_GPU_CLOCK           = 1,
    AGA_GPU_WATCH_ATTR_ID_MEM_CLOCK           = 2,
    AGA_GPU_WATCH_ATTR_ID_MEMORY_TEMP         = 3,
    AGA_GPU_WATCH_ATTR_ID_GPU_TEMP            = 4,
    AGA_GPU_WATCH_ATTR_ID_POWER_USAGE         = 5,
    AGA_GPU_WATCH_ATTR_ID_PCIE_TX             = 6,
    AGA_GPU_WATCH_ATTR_ID_PCIE_RX             = 7,
    AGA_GPU_WATCH_ATTR_ID_GPU_UTIL            = 8,
    AGA_GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE    = 9,
    AGA_GPU_WATCH_ATTR_ID_ECC_CORRECT_TOTAL   = 10,
    AGA_GPU_WATCH_ATTR_ID_ECC_UNCORRECT_TOTAL = 11,
    AGA_GPU_WATCH_ATTR_ID_ECC_SDMA_CE         = 12,
    AGA_GPU_WATCH_ATTR_ID_ECC_SDMA_UE         = 13,
    AGA_GPU_WATCH_ATTR_ID_ECC_GFX_CE          = 14,
    AGA_GPU_WATCH_ATTR_ID_ECC_GFX_UE          = 15,
    AGA_GPU_WATCH_ATTR_ID_ECC_MMHUB_CE        = 16,
    AGA_GPU_WATCH_ATTR_ID_ECC_MMHUB_UE        = 17,
    AGA_GPU_WATCH_ATTR_ID_ECC_ATHUB_CE        = 18,
    AGA_GPU_WATCH_ATTR_ID_ECC_ATHUB_UE        = 19,
    AGA_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_CE     = 20,
    AGA_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_UE     = 21,
    AGA_GPU_WATCH_ATTR_ID_ECC_HDP_CE          = 22,
    AGA_GPU_WATCH_ATTR_ID_ECC_HDP_UE          = 23,
    AGA_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_CE    = 24,
    AGA_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_UE    = 25,
    AGA_GPU_WATCH_ATTR_ID_ECC_DF_CE           = 26,
    AGA_GPU_WATCH_ATTR_ID_ECC_DF_UE           = 27,
    AGA_GPU_WATCH_ATTR_ID_ECC_SMN_CE          = 28,
    AGA_GPU_WATCH_ATTR_ID_ECC_SMN_UE          = 29,
    AGA_GPU_WATCH_ATTR_ID_ECC_SEM_CE          = 30,
    AGA_GPU_WATCH_ATTR_ID_ECC_SEM_UE          = 31,
    AGA_GPU_WATCH_ATTR_ID_ECC_MP0_CE          = 32,
    AGA_GPU_WATCH_ATTR_ID_ECC_MP0_UE          = 33,
    AGA_GPU_WATCH_ATTR_ID_ECC_MP1_CE          = 34,
    AGA_GPU_WATCH_ATTR_ID_ECC_MP1_UE          = 35,
    AGA_GPU_WATCH_ATTR_ID_ECC_FUSE_CE         = 36,
    AGA_GPU_WATCH_ATTR_ID_ECC_FUSE_UE         = 37,
    AGA_GPU_WATCH_ATTR_ID_ECC_UMC_CE          = 38,
    AGA_GPU_WATCH_ATTR_ID_ECC_UMC_UE          = 39,
    AGA_GPU_WATCH_ATTR_ID_ECC_MCA_CE          = 40,
    AGA_GPU_WATCH_ATTR_ID_ECC_MCA_UE          = 41,
    AGA_GPU_WATCH_ATTR_ID_ECC_VCN_CE          = 42,
    AGA_GPU_WATCH_ATTR_ID_ECC_VCN_UE          = 43,
    AGA_GPU_WATCH_ATTR_ID_ECC_JPEG_CE         = 44,
    AGA_GPU_WATCH_ATTR_ID_ECC_JPEG_UE         = 45,
    AGA_GPU_WATCH_ATTR_ID_ECC_IH_CE           = 46,
    AGA_GPU_WATCH_ATTR_ID_ECC_IH_UE           = 47,
    AGA_GPU_WATCH_ATTR_ID_ECC_MPIO_CE         = 48,
    AGA_GPU_WATCH_ATTR_ID_ECC_MPIO_UE         = 49,
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_NOP_TX       = 50,
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_REQ_TX       = 51,
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_RESP_TX      = 52,
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_BEATS_TX     = 53,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_NOP_TX       = 54,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_REQ_TX       = 55,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_RESP_TX      = 56,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_BEATS_TX     = 57,
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_THRPUT       = 58,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_THRPUT       = 59,
    AGA_GPU_WATCH_ATTR_ID_XGMI_2_THRPUT       = 60,
    AGA_GPU_WATCH_ATTR_ID_XGMI_3_THRPUT       = 61,
    AGA_GPU_WATCH_ATTR_ID_XGMI_4_THRPUT       = 62,
    AGA_GPU_WATCH_ATTR_ID_XGMI_5_THRPUT       = 63,
    AGA_GPU_WATCH_ATTR_ID_PCIE_BANDWIDTH      = 64,
    AGA_GPU_WATCH_ATTRS_MAX                   = 65,
} aga_gpu_watch_attr_id_t;

/// \brief    GPU watch attribute value type
typedef enum aga_gpu_watch_attr_value_type_e {
    AGA_GPU_WATCH_ATTR_VALUE_TYPE_NONE   = 0,
    // attribute value type float
    AGA_GPU_WATCH_ATTR_VALUE_TYPE_FLOAT  = 1,
    // attribute value type long
    AGA_GPU_WATCH_ATTR_VALUE_TYPE_LONG   = 2,
    // attribute value type string
    AGA_GPU_WATCH_ATTR_VALUE_TYPE_STRING = 3,
} aga_gpu_watch_attr_value_type_t;

/// \brief    watch GPU attribute value
typedef struct aga_gpu_watch_attr_value_s {
    /// attribute value type
    aga_gpu_watch_attr_value_type_t type;
    /// attribute value
    union {
        float float_val;
        uint64_t long_val;
        char str_val[AGA_MAX_WATCH_ATTR_STR + 1];
    };
} aga_gpu_watch_attr_value_t;

/// \brief    watch GPU attribute record
typedef struct aga_gpu_watch_attr_s {
    /// watch GPU attribute identifier
    aga_gpu_watch_attr_id_t id;
    /// timestamp indicating when the attribute read
    timespec_t timestamp;
    /// uuid of the GPU device
    aga_obj_key_t gpu;
    /// attribute value
    aga_gpu_watch_attr_value_t value;
} aga_gpu_watch_attr_t;

/// \brief    GPU watch attributes
typedef struct aga_gpu_watch_attrs_s {
    /// uuid of GPU
    aga_obj_key_t gpu;
    /// list of GPU watch attributes
    uint16_t num_attrs;
    aga_gpu_watch_attr_t attr[AGA_GPU_WATCH_ATTRS_MAX];
} aga_gpu_watch_attrs_t;

/// \brief GPU watch specification
typedef struct aga_gpu_watch_spec_s {
    /// uuid of GPU watch object
    aga_obj_key_t key;
    /// list of GPUs to be monitored
    uint8_t num_gpu;
    aga_obj_key_t gpu[AGA_MAX_GPU];
    /// list of attributes to be monitor
    uint16_t num_attrs;
    aga_gpu_watch_attr_id_t attr_id[AGA_GPU_WATCH_ATTRS_MAX];
} aga_gpu_watch_spec_t;

/// \brief GPU watch operational information
typedef struct aga_gpu_watch_status_s {
    /// number of subscribers to this GPU watch object
    uint32_t num_subscribers;
} aga_gpu_watch_status_t;

/// \brief GPU watch statistics
typedef struct aga_gpu_watch_stats_s {
    /// number of GPUs being monitored
    uint8_t num_gpu;
    aga_gpu_watch_attrs_t gpu_watch_attr[AGA_MAX_GPU];
} aga_gpu_watch_stats_t;

/// \brief GPU watch info
typedef struct aga_gpu_watch_info_s {
    aga_gpu_watch_spec_t spec;
    aga_gpu_watch_status_t status;
    aga_gpu_watch_stats_t stats;
} aga_gpu_watch_info_t;

typedef sdk_ret_t (*aga_gpu_watch_cb_t)(_In_ const aga_gpu_watch_info_t *info,
                                        _Out_ void *ctxt);

/// \brief    GPU watch subscribe request from a gRPC client
typedef struct aga_gpu_watch_subscribe_req_s {
    /// number of gpu-watch ids
    uint8_t num_gpu_watch_ids;
    /// list of gpu watch uuids
    aga_obj_key_t gpu_watch_ids[AGA_MAX_GPU_WATCH];
    /// gRPC client IP address and port
    char client[AGA_MAX_GPU_WATCH_CLIENT_STR + 1];
    /// opaque context sent from gRPC thread to backend
    /// NOTE:
    /// gRPC response stream to periodically publish events to
    void *stream;
    /// callback API to write gpu watch info to the client stream
    aga_gpu_watch_cb_t write_cb;
} aga_gpu_watch_subscribe_req_t;

/// \brief     create gpu watch object
/// \param[in] spec config specification
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_gpu_watch_create(_In_ aga_gpu_watch_spec_t *spec);

/// \brief     delete gpu watch object
/// \param[in] key key
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_gpu_watch_delete(_In_ aga_obj_key_t *key);

/// \brief      read gpu watch
/// \param[in]  key  key of the gpu object
/// \param[out] info information
/// \return     #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_gpu_watch_read(_In_ aga_obj_key_t *key,
                             _Out_ aga_gpu_watch_info_t *info);

typedef void (*gpu_watch_read_cb_t)(void *ctxt,
                                    const aga_gpu_watch_info_t *info);

/// \brief    read all gpu watch information
/// \param[in]  cb      callback function
/// \param[in]  ctxt    opaque context passed to cb
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_gpu_watch_read_all(_In_ gpu_watch_read_cb_t gpu_watch_read_cb,
                                 _In_ void *ctxt);

/// \brief    gpu watch subscribe
/// \param[in] req    pointer to gpu watch subscribe request
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_gpu_watch_subscribe(aga_gpu_watch_subscribe_req_t *req);

#endif    // __API_INCLUDE_AGA_GPU_WATCH_HPP__

