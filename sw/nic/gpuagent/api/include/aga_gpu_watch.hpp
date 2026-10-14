
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
/// allowed range and default for the fast attribute sampling period, in seconds
#define AGA_GPU_WATCH_FAST_PERIOD_MIN     1
#define AGA_GPU_WATCH_FAST_PERIOD_MAX     60
#define AGA_GPU_WATCH_FAST_PERIOD_DFLT    1
/// allowed range and default for the slow attribute sampling period, in seconds
#define AGA_GPU_WATCH_SLOW_PERIOD_MIN     5
#define AGA_GPU_WATCH_SLOW_PERIOD_MAX     3600
#define AGA_GPU_WATCH_SLOW_PERIOD_DFLT    5

/// \brief    GPU attributes that are watchable
typedef enum aga_gpu_watch_attr_id_e {
    AGA_GPU_WATCH_ATTR_ID_INVALID                                        = 0,
    AGA_GPU_WATCH_ATTR_ID_GPU_CLOCK                                      = 1,
    AGA_GPU_WATCH_ATTR_ID_MEM_CLOCK                                      = 2,
    AGA_GPU_WATCH_ATTR_ID_MEMORY_TEMP                                    = 3,
    AGA_GPU_WATCH_ATTR_ID_GPU_TEMP                                       = 4,
    AGA_GPU_WATCH_ATTR_ID_POWER_USAGE                                    = 5,
    AGA_GPU_WATCH_ATTR_ID_PCIE_TX                                        = 6,
    AGA_GPU_WATCH_ATTR_ID_PCIE_RX                                        = 7,
    AGA_GPU_WATCH_ATTR_ID_GPU_UTIL                                       = 8,
    AGA_GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE                               = 9,
    AGA_GPU_WATCH_ATTR_ID_ECC_CORRECT_TOTAL                              = 10,
    AGA_GPU_WATCH_ATTR_ID_ECC_UNCORRECT_TOTAL                            = 11,
    AGA_GPU_WATCH_ATTR_ID_ECC_SDMA_CE                                    = 12,
    AGA_GPU_WATCH_ATTR_ID_ECC_SDMA_UE                                    = 13,
    AGA_GPU_WATCH_ATTR_ID_ECC_GFX_CE                                     = 14,
    AGA_GPU_WATCH_ATTR_ID_ECC_GFX_UE                                     = 15,
    AGA_GPU_WATCH_ATTR_ID_ECC_MMHUB_CE                                   = 16,
    AGA_GPU_WATCH_ATTR_ID_ECC_MMHUB_UE                                   = 17,
    AGA_GPU_WATCH_ATTR_ID_ECC_ATHUB_CE                                   = 18,
    AGA_GPU_WATCH_ATTR_ID_ECC_ATHUB_UE                                   = 19,
    AGA_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_CE                                = 20,
    AGA_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_UE                                = 21,
    AGA_GPU_WATCH_ATTR_ID_ECC_HDP_CE                                     = 22,
    AGA_GPU_WATCH_ATTR_ID_ECC_HDP_UE                                     = 23,
    AGA_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_CE                               = 24,
    AGA_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_UE                               = 25,
    AGA_GPU_WATCH_ATTR_ID_ECC_DF_CE                                      = 26,
    AGA_GPU_WATCH_ATTR_ID_ECC_DF_UE                                      = 27,
    AGA_GPU_WATCH_ATTR_ID_ECC_SMN_CE                                     = 28,
    AGA_GPU_WATCH_ATTR_ID_ECC_SMN_UE                                     = 29,
    AGA_GPU_WATCH_ATTR_ID_ECC_SEM_CE                                     = 30,
    AGA_GPU_WATCH_ATTR_ID_ECC_SEM_UE                                     = 31,
    AGA_GPU_WATCH_ATTR_ID_ECC_MP0_CE                                     = 32,
    AGA_GPU_WATCH_ATTR_ID_ECC_MP0_UE                                     = 33,
    AGA_GPU_WATCH_ATTR_ID_ECC_MP1_CE                                     = 34,
    AGA_GPU_WATCH_ATTR_ID_ECC_MP1_UE                                     = 35,
    AGA_GPU_WATCH_ATTR_ID_ECC_FUSE_CE                                    = 36,
    AGA_GPU_WATCH_ATTR_ID_ECC_FUSE_UE                                    = 37,
    AGA_GPU_WATCH_ATTR_ID_ECC_UMC_CE                                     = 38,
    AGA_GPU_WATCH_ATTR_ID_ECC_UMC_UE                                     = 39,
    AGA_GPU_WATCH_ATTR_ID_ECC_MCA_CE                                     = 40,
    AGA_GPU_WATCH_ATTR_ID_ECC_MCA_UE                                     = 41,
    AGA_GPU_WATCH_ATTR_ID_ECC_VCN_CE                                     = 42,
    AGA_GPU_WATCH_ATTR_ID_ECC_VCN_UE                                     = 43,
    AGA_GPU_WATCH_ATTR_ID_ECC_JPEG_CE                                    = 44,
    AGA_GPU_WATCH_ATTR_ID_ECC_JPEG_UE                                    = 45,
    AGA_GPU_WATCH_ATTR_ID_ECC_IH_CE                                      = 46,
    AGA_GPU_WATCH_ATTR_ID_ECC_IH_UE                                      = 47,
    AGA_GPU_WATCH_ATTR_ID_ECC_MPIO_CE                                    = 48,
    AGA_GPU_WATCH_ATTR_ID_ECC_MPIO_UE                                    = 49,
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_NOP_TX                                  = 50,
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_REQ_TX                                  = 51,
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_RESP_TX                                 = 52,
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_BEATS_TX                                = 53,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_NOP_TX                                  = 54,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_REQ_TX                                  = 55,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_RESP_TX                                 = 56,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_BEATS_TX                                = 57,
    AGA_GPU_WATCH_ATTR_ID_XGMI_0_THRPUT                                  = 58,
    AGA_GPU_WATCH_ATTR_ID_XGMI_1_THRPUT                                  = 59,
    AGA_GPU_WATCH_ATTR_ID_XGMI_2_THRPUT                                  = 60,
    AGA_GPU_WATCH_ATTR_ID_XGMI_3_THRPUT                                  = 61,
    AGA_GPU_WATCH_ATTR_ID_XGMI_4_THRPUT                                  = 62,
    AGA_GPU_WATCH_ATTR_ID_XGMI_5_THRPUT                                  = 63,
    AGA_GPU_WATCH_ATTR_ID_PCIE_BANDWIDTH                                 = 64,
    AGA_GPU_WATCH_ATTR_ID_UMC_ACTIVITY                                   = 65,
    AGA_GPU_WATCH_ATTR_ID_MM_ACTIVITY                                    = 66,
    AGA_GPU_WATCH_ATTR_ID_AVG_POWER                                      = 67,
    AGA_GPU_WATCH_ATTR_ID_GFX_VOLTAGE                                    = 68,
    AGA_GPU_WATCH_ATTR_ID_SOC_VOLTAGE                                    = 69,
    AGA_GPU_WATCH_ATTR_ID_MEM_VOLTAGE                                    = 70,
    // array-valued (indexed) attributes; value rides the LONG_ARRAY variant
    AGA_GPU_WATCH_ATTR_ID_HBM_TEMP                                       = 71,
    AGA_GPU_WATCH_ATTR_ID_VCN_ACTIVITY                                   = 72,
    AGA_GPU_WATCH_ATTR_ID_JPEG_ACTIVITY                                  = 73,
    AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_CURRENT                             = 74,
    AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_MIN                                 = 75,
    AGA_GPU_WATCH_ATTR_ID_CLOCK_FREQ_MAX                                 = 76,
    AGA_GPU_WATCH_ATTR_ID_XGMI_LINK_RX                                   = 77,
    AGA_GPU_WATCH_ATTR_ID_XGMI_LINK_TX                                   = 78,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_ACC   = 79,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_ACC = 80,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_ACC                         = 81,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_ACC   = 82,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_PCT   = 83,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_PCT = 84,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_PCT                         = 85,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_PCT   = 86,
    AGA_GPU_WATCH_ATTR_ID_GFX_BUSY_INST                                  = 87,
    AGA_GPU_WATCH_ATTR_ID_VCN_BUSY_INST                                  = 88,
    AGA_GPU_WATCH_ATTR_ID_JPEG_BUSY_INST                                 = 89,
    // flat scalar violation counters; value rides the LONG variant
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_CURRENT_ACC_COUNTER                  = 90,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_ACC               = 91,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_ACC                    = 92,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_ACC         = 93,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_ACC             = 94,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_ACC            = 95,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_PCT               = 96,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_PCT                    = 97,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_PCT         = 98,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_PCT             = 99,
    AGA_GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_PCT            = 100,
    // ECC deferred counters
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_TOTAL                                  = 101,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SDMA                                   = 102,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_GFX                                    = 103,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MMHUB                                  = 104,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_ATHUB                                  = 105,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_PCIE_BIF                               = 106,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_HDP                                    = 107,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_XGMI_WAFL                              = 108,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_DF                                     = 109,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SMN                                    = 110,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_SEM                                    = 111,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MP0                                    = 112,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MP1                                    = 113,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_FUSE                                   = 114,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_UMC                                    = 115,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MCA                                    = 116,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_VCN                                    = 117,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_JPEG                                   = 118,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_IH                                     = 119,
    AGA_GPU_WATCH_ATTR_ID_ECC_DEF_MPIO                                   = 120,
    // PCIe accumulated counters
    AGA_GPU_WATCH_ATTR_ID_PCIE_REPLAY_COUNT                              = 121,
    AGA_GPU_WATCH_ATTR_ID_PCIE_RECOVERY_COUNT                            = 122,
    AGA_GPU_WATCH_ATTR_ID_PCIE_REPLAY_ROLLOVER_COUNT                     = 123,
    AGA_GPU_WATCH_ATTR_ID_PCIE_NACK_SENT_COUNT                           = 124,
    AGA_GPU_WATCH_ATTR_ID_PCIE_NACK_RECEIVED_COUNT                       = 125,
    // additional bulk scalars
    AGA_GPU_WATCH_ATTR_ID_GPU_POWER_USAGE                                = 126,
    AGA_GPU_WATCH_ATTR_ID_VRAM_MAX_BANDWIDTH                             = 127,
    AGA_GPU_WATCH_ATTR_ID_EDGE_TEMP                                      = 128,
    // memory totals/used/free and PCIe link speed
    AGA_GPU_WATCH_ATTR_ID_TOTAL_VRAM                                     = 129,
    AGA_GPU_WATCH_ATTR_ID_TOTAL_VISIBLE_VRAM                             = 130,
    AGA_GPU_WATCH_ATTR_ID_TOTAL_GTT                                      = 131,
    AGA_GPU_WATCH_ATTR_ID_USED_VISIBLE_VRAM                              = 132,
    AGA_GPU_WATCH_ATTR_ID_USED_GTT                                       = 133,
    AGA_GPU_WATCH_ATTR_ID_FREE_VRAM                                      = 134,
    AGA_GPU_WATCH_ATTR_ID_FREE_VISIBLE_VRAM                              = 135,
    AGA_GPU_WATCH_ATTR_ID_FREE_GTT                                       = 136,
    AGA_GPU_WATCH_ATTR_ID_PCIE_SPEED                                     = 137,
    AGA_GPU_WATCH_ATTR_ID_PCIE_MAX_SPEED                                 = 138,
    AGA_GPU_WATCH_ATTR_ID_ENERGY_CONSUMED                                = 139,
    AGA_GPU_WATCH_ATTRS_MAX                                              = 140,
} aga_gpu_watch_attr_id_t;

/// \brief    GPU watch attribute value type
typedef enum aga_gpu_watch_attr_value_type_e {
    AGA_GPU_WATCH_ATTR_VALUE_TYPE_NONE       = 0,
    // attribute value type float
    AGA_GPU_WATCH_ATTR_VALUE_TYPE_FLOAT      = 1,
    // attribute value type long
    AGA_GPU_WATCH_ATTR_VALUE_TYPE_LONG       = 2,
    // attribute value type string
    AGA_GPU_WATCH_ATTR_VALUE_TYPE_STRING     = 3,
    // attribute value type long array (array-valued/indexed attributes)
    AGA_GPU_WATCH_ATTR_VALUE_TYPE_LONG_ARRAY = 4,
} aga_gpu_watch_attr_value_type_t;

/// max elements in an array-valued watch attribute; 40 covers the largest
/// array (JPEG, AGA_GPU_MAX_JPEG_ENG engines), also the multi-XCD clock set
#define AGA_GPU_WATCH_MAX_INDEXED_VALS    40

/// \brief    one element of an array-valued watch attribute
typedef struct aga_gpu_watch_indexed_val_s {
    /// 0-based element index, becomes the *_index label on the exporter
    uint32_t index;
    /// optional qualifier enum (e.g. clock_type); 0 means none, dropped by
    /// the exporter. Fills struct alignment padding, so size stays 16 bytes
    uint32_t sub_type;
    /// element value
    uint64_t val;
} aga_gpu_watch_indexed_val_t;

/// \brief    watch GPU attribute value
typedef struct aga_gpu_watch_attr_value_s {
    /// attribute value type
    aga_gpu_watch_attr_value_type_t type;
    /// attribute value
    union {
        float float_val;
        uint64_t long_val;
        char str_val[AGA_MAX_WATCH_ATTR_STR + 1];
        /// set when type is LONG_ARRAY
        struct {
            uint32_t num_vals;
            aga_gpu_watch_indexed_val_t vals[AGA_GPU_WATCH_MAX_INDEXED_VALS];
        } long_array;
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
    /// sampling period of fast attributes, also the subscriber notify period
    uint32_t fast_period_secs;
    /// sampling period of slow attributes, always >= fast_period_secs
    uint32_t slow_period_secs;
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

