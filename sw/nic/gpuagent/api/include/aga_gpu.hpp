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
/// GPU spec, status, stats and APIs
///
//----------------------------------------------------------------------------

#ifndef __API_INCLUDE_AGA_GPU_HPP__
#define __API_INCLUDE_AGA_GPU_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/api/include/base.hpp"
#include "nic/gpuagent/api/smi/smi.hpp"

#define AGA_GPU_MAX_CLOCK_FREQUENCY            6
#define AGA_GPU_MAX_HBM                        4
#define AGA_GPU_MAX_FIRMWARE_VERSION           85
#define AGA_GPU_MAX_KFD_PID                    8
#define AGA_GPU_MAX_VOLTAGE_CURVE_POINT        4
#define AGA_GPU_MIN_OVERDRIVE_LEVEL            0
#define AGA_GPU_MAX_OVERDRIVE_LEVEL            20
#define AGA_MAX_PEER_DEVICE                    16
#define AGA_GPU_GFX_MAX_CLOCK                  8
#define AGA_GPU_MEM_MAX_CLOCK                  1
#define AGA_GPU_VIDEO_MAX_CLOCK                4
#define AGA_GPU_DATA_MAX_CLOCK                 4
#define AGA_GPU_MAX_VCN                        4
#define AGA_GPU_MAX_JPEG                       32
#define AGA_GPU_MAX_XGMI_LINKS                 8
#define AGA_GPU_MAX_BAD_PAGE_RECORD            64
#define AGA_GPU_INVALID_PARTITION_ID           0xFFFFFFFF
#define AGA_GPU_MAX_PARTITION                  8
/// number of clocks that can not be configured - AGA_GPU_CLOCK_TYPE_FABRIC,
/// AGA_GPU_CLOCK_TYPE_SOC, AGA_GPU_CLOCK_TYPE_DCE, AGA_GPU_CLOCK_TYPE_PCIE
#define AGA_GPU_NUM_NON_CFG_CLOCK_TYPES        4
/// clock frequency range are per clock type; as of now it is only set for
/// clocks of type AGA_GPU_CLOCK_TYPE_SYSTEM, AGA_GPU_CLOCK_TYPE_MEMORY,
/// AGA_GPU_CLOCK_TYPE_VIDEO and AGA_GPU_CLOCK_TYPE_DATA
#define AGA_GPU_NUM_CFG_CLOCK_TYPES            4
/// total number of clocks; for non configurable clocks we assume 1 of each type
#define AGA_GPU_MAX_CLOCK                               \
    (AGA_GPU_GFX_MAX_CLOCK + AGA_GPU_MEM_MAX_CLOCK +    \
     AGA_GPU_VIDEO_MAX_CLOCK + AGA_GPU_DATA_MAX_CLOCK + \
     AGA_GPU_NUM_NON_CFG_CLOCK_TYPES)

/// \brief GPU admin state
typedef enum aga_gpu_admin_state_e {
    AGA_GPU_ADMIN_STATE_NONE = 0,
    /// admin UP
    AGA_GPU_ADMIN_STATE_UP   = 1,
    /// admin DOWN
    AGA_GPU_ADMIN_STATE_DOWN = 2,
} aga_gpu_admin_state_t;

/// \brief GPU clock types
typedef enum aga_gpu_clock_type_e {
    AGA_GPU_CLOCK_TYPE_NONE   = 0,
    /// fabric clock (aka. fclk)
    AGA_GPU_CLOCK_TYPE_FABRIC = 1,
    /// memory clock (aka. mclk)
    AGA_GPU_CLOCK_TYPE_MEMORY = 2,
    /// system clock (aka. sclk)
    AGA_GPU_CLOCK_TYPE_SYSTEM = 3,
    /// SoC clock (aka. socclk)
    AGA_GPU_CLOCK_TYPE_SOC    = 4,
    /// Display Controller Engine (DCE) clock
    AGA_GPU_CLOCK_TYPE_DCE    = 5,
    /// PCIe clock
    AGA_GPU_CLOCK_TYPE_PCIE   = 6,
    /// video clock
    AGA_GPU_CLOCK_TYPE_VIDEO  = 7,
    /// data clock
    AGA_GPU_CLOCK_TYPE_DATA   = 8,
    AGA_GPU_CLOCK_TYPE_MAX    = 9,
} aga_gpu_clock_type_t;

/// \brief GPU performance levels
typedef enum aga_gpu_perf_level_e {
    AGA_GPU_PERF_LEVEL_NONE             = 0,
    /// performance level auto
    AGA_GPU_PERF_LEVEL_AUTO             = 1,
    /// performance level low
    AGA_GPU_PERF_LEVEL_LOW              = 2,
    /// performance level high
    AGA_GPU_PERF_LEVEL_HIGH             = 3,
    /// minimum performance variation
    AGA_GPU_PERF_LEVEL_DETERMINISTIC    = 4,
    /// stable power state with minimum memory clock
    AGA_GPU_PERF_LEVEL_STABLE_WITH_MCLK = 5,
    /// stable power state with minimum system clock
    AGA_GPU_PERF_LEVEL_STABLE_WITH_SCLK = 6,
    /// manual performance level
    AGA_GPU_PERF_LEVEL_MANUAL           = 7,
} aga_gpu_perf_level_t;

/// \brief GPU throttling status
typedef enum aga_gpu_throttling_status_e {
    AGA_GPU_THROTTLING_STATUS_NONE = 0,
    /// throttling off
    AGA_GPU_THROTTLING_STATUS_OFF  = 1,
    /// throttling on
    AGA_GPU_THROTTLING_STATUS_ON   = 2,
} aga_gpu_throttling_status_t;

/// \brief GPU clock frequency range
/// NOTE:
/// values are in MHz
typedef struct aga_gpu_clock_freq_range_s {
    /// clock type
    aga_gpu_clock_type_t clock_type;
    /// low frequency value
    uint32_t lo;
    /// high frequency value
    uint32_t hi;
} aga_gpu_clock_freq_range_t;

/// \brief  RAS configuration per block
typedef struct aga_gpu_ras_spec_s {
    // TODO:
    // fill this
} aga_gpu_ras_spec_t;

/// GPU compute partition type
typedef enum aga_gpu_compute_partition_type_e {
    /// unknown/invalid partition type
    AGA_GPU_COMPUTE_PARTITION_TYPE_NONE = 0,
    /// single-partition GPU mode
    AGA_GPU_COMPUTE_PARTITION_TYPE_SPX  = 1,
    /// dual-partition GPU mode
    AGA_GPU_COMPUTE_PARTITION_TYPE_DPX  = 2,
    /// triple-partition GPU mode
    AGA_GPU_COMPUTE_PARTITION_TYPE_TPX  = 3,
    /// quad-partition GPU mode
    AGA_GPU_COMPUTE_PARTITION_TYPE_QPX  = 4,
    /// core-partition GPU mode
    AGA_GPU_COMPUTE_PARTITION_TYPE_CPX  = 5,
} aga_gpu_compute_partition_type_t;

/// \brief GPU specification
typedef struct aga_gpu_spec_s {
    /// uuid of gpu
    /// NOTE: in case of partition, all child GPUs share the UUID of the parent
    ///       GPU; to differentiate between them  we encode the partition ID in
    ///       the 4 bytes starting from byte 4 of the parent GPU UUID;
    ///       example : parent GPU - 1eff74a1-0000-1000-807e-1746627a9cd7
    ///                 child GPUs - 1eff74a1-0000-0000-807e-1746627a9cd7
    ///                              1eff74a1-0000-0001-807e-1746627a9cd7
    ///                              1eff74a1-0000-0002-807e-1746627a9cd7
    ///                              1eff74a1-0000-0003-807e-1746627a9cd7
    aga_obj_key_t key;
    /// uuid of parent gpu in case of partitioning
    aga_obj_key_t parent_gpu;
    /// admin state
    aga_gpu_admin_state_t admin_state;
    /// GPU clock overdrive level (as percentage)
    uint32_t overdrive_level;
    /// max GPU power in power overdrive (in Watts)
    uint64_t gpu_power_cap;
    /// GPU performance level
    aga_gpu_perf_level_t perf_level;
    /// number of clock frequencies
    uint32_t num_clock_freqs;
    /// clock frequency ranges
    aga_gpu_clock_freq_range_t clock_freq[AGA_GPU_NUM_CFG_CLOCK_TYPES];
    /// fan speed
    uint64_t fan_speed;
    /// GPU RAS configuration
    aga_gpu_ras_spec_t ras_spec;
    /// GPU compute partition type
    aga_gpu_compute_partition_type_t compute_partition_type;
} aga_gpu_spec_t;

/// \brief GPU operational state
typedef enum aga_gpu_oper_state_e {
    AGA_GPU_OPER_STATE_NONE = 0,
    /// operationally up
    AGA_GPU_OPER_STATE_UP   = 1,
    /// operationall down
    AGA_GPU_OPER_STATE_DOWN = 2,
} aga_gpu_oper_state_t;

/// GPU temperature sensor type
typedef enum aga_gpu_temp_sensor_type_e {
    AGA_GPU_TEMP_TYPE_EDGE     = 0,
    AGA_GPU_TEMP_TYPE_JUNCTION = 1,
    AGA_GPU_TEMP_TYPE_MEMORY   = 2,
    AGA_GPU_TEMP_TYPE_HBM_0    = 3,
    AGA_GPU_TEMP_TYPE_HBM_1    = 4,
    AGA_GPU_TEMP_TYPE_HBM_2    = 5,
    AGA_GPU_TEMP_TYPE_HBM_3    = 6,
    AGA_GPU_TEMP_TYPE_INVALID  = 7,
} aga_gpu_temp_sensor_type_t;

/// \brief GPU temperature information
/// NOTE:
/// all temperatures are in centigrade
typedef struct aga_gpu_temperature_stats_s {
    /// edge temperature
    float edge_temperature;
    /// junction temperature
    float junction_temperature;
    /// memory temperature
    float memory_temperature;
    /// HBM temperatures
    float hbm_temperature[AGA_GPU_MAX_HBM];
} aga_gpu_temperature_stats_t;

/// \brief GPU usage status as percentage of time GPU is busy
typedef struct aga_gpu_usage_s {
    uint32_t gfx_activity;
    uint32_t umc_activity;
    uint32_t mm_activity;
    uint16_t num_vcn;
    uint16_t vcn_activity[AGA_GPU_MAX_VCN];
    uint16_t num_jpeg;
    uint16_t jpeg_activity[AGA_GPU_MAX_JPEG];
} aga_gpu_usage_t;

/// \brief GPU current memory usage
typedef struct aga_gpu_memory_usage_s {
    /// percentage of available memory in use
    float memory_usage;
    float activity;
} aga_gpu_memory_usage_t;

/// \brief GPU clock information
typedef struct aga_gpu_clock_status_s {
    /// clock type
    aga_gpu_clock_type_t clock_type;
    /// clock frequency in MHz
    uint32_t frequency;
    /// low frequency value
    uint32_t low_frequency;
    /// high frequency value
    uint32_t high_frequency;
    /// clock is locked or not
    bool locked;
    /// clock is in deep sleep or not
    bool deep_sleep;
} aga_gpu_clock_status_t;

/// \brief GPU voltage curve point
typedef struct aga_gpu_voltage_curve_point_s {
    /// curve point
    uint32_t point;
    /// frequency in MHz
    uint32_t frequency;
    /// voltage
    uint32_t voltage;
} aga_gpu_voltage_curve_point_t;

/// \brief GPU reliability, availability & servicability status
typedef struct aga_gpu_ras_status_s {
    // TODO:
    // fill this
} aga_gpu_ras_status_t;


/// \brief GPU xgmi error status
typedef enum aga_gpu_xgmi_error_status_e {
    AGA_GPU_XGMI_STATUS_NONE           = 0,
    /// no errors since last read
    AGA_GPU_XGMI_STATUS_NO_ERROR       = 1,
    /// one error detected since last read
    AGA_GPU_XGMI_STATUS_ONE_ERROR      = 2,
    /// multiple errors detected since last read
    AGA_GPU_XGMI_STATUS_MULTIPLE_ERROR = 3,
} aga_gpu_xgmi_error_status_t;

/// \brief GPU XGMI status
typedef struct aga_gpu_xgmi_status_s {
    /// XGMI error status
    aga_gpu_xgmi_error_status_t error_status;
    /// XGMI link width in GB/s
    uint64_t width;
    /// XGMI link speed in GB/s
    uint64_t speed;
} aga_gpu_xgmi_status_t;

/// \brief GPU firmware version
typedef struct aga_gpu_fw_version_s {
    /// name of the component
    char firmware[AGA_MAX_STR_LEN + 1];
    /// firmware version of component
    char version[AGA_MAX_STR_LEN + 1];
} aga_gpu_fw_version_t;

/// \brief PCIe slot type
typedef enum aga_pcie_slot_type_e {
    AGA_PCIE_SLOT_TYPE_NONE    = 0,
    AGA_PCIE_SLOT_TYPE_PCIE    = 1,
    AGA_PCIE_SLOT_TYPE_OAM     = 2,
    AGA_PCIE_SLOT_TYPE_CEM     = 3,
    AGA_PCIE_SLOT_TYPE_UNKNOWN = 4,
} aga_pcie_slot_type_t;

/// \brief PCIe status
typedef struct aga_gpu_pcie_status_s {
    /// PCIe card form factor
    aga_pcie_slot_type_t slot_type;
    /// pcie bus id
    char pcie_bus_id[AGA_MAX_STR_LEN + 1];
    /// maximum number of PCIe lanes
    uint32_t max_width;
    /// maximum PCIe speed
    uint32_t max_speed;
    /// PCIe interface version
    uint32_t version;
    /// current PCIe lanes
    uint32_t width;
    /// current PCIe speed (in GT/s)
    uint32_t speed;
    /// current PCIe bandwidth (in MB/s)
    uint64_t bandwidth;
} aga_gpu_pcie_status_t;

/// \brief VRAM type
typedef enum aga_vram_type_e {
    AGA_VRAM_TYPE_NONE    = 0,
    AGA_VRAM_TYPE_HBM     = 1,
    AGA_VRAM_TYPE_HBM2    = 2,
    AGA_VRAM_TYPE_HBM2E   = 3,
    AGA_VRAM_TYPE_HBM3    = 4,
    AGA_VRAM_TYPE_DDR2    = 5,
    AGA_VRAM_TYPE_DDR3    = 6,
    AGA_VRAM_TYPE_DDR4    = 7,
    AGA_VRAM_TYPE_GDDR1   = 8,
    AGA_VRAM_TYPE_GDDR2   = 9,
    AGA_VRAM_TYPE_GDDR3   = 10,
    AGA_VRAM_TYPE_GDDR4   = 11,
    AGA_VRAM_TYPE_GDDR5   = 12,
    AGA_VRAM_TYPE_GDDR6   = 13,
    AGA_VRAM_TYPE_GDDR7   = 14,
    AGA_VRAM_TYPE_UNKNOWN = 15,
} aga_vram_type_t;

/// \brief VRAM vendor
typedef enum aga_vram_vendor_e {
    AGA_VRAM_VENDOR_NONE     = 0,
    AGA_VRAM_VENDOR_SAMSUNG  = 1,
    AGA_VRAM_VENDOR_INFINEON = 2,
    AGA_VRAM_VENDOR_ELPIDA   = 3,
    AGA_VRAM_VENDOR_ETRON    = 4,
    AGA_VRAM_VENDOR_NANYA    = 5,
    AGA_VRAM_VENDOR_HYNIX    = 6,
    AGA_VRAM_VENDOR_MOSEL    = 7,
    AGA_VRAM_VENDOR_WINBOND  = 8,
    AGA_VRAM_VENDOR_ESMT     = 9,
    AGA_VRAM_VENDOR_MICRON   = 10,
    AGA_VRAM_VENDOR_UNKNOWN  = 11,
} aga_vram_vendor_t;

/// \brief VRAM status
typedef struct aga_gpu_vram_status_s {
    /// VRAM type
    aga_vram_type_t type;
    /// VRAM vendor
    aga_vram_vendor_t vendor;
    /// VRAM size (in MB)
    uint64_t size;
} aga_gpu_vram_status_t;

/// \brief GPU page status
typedef enum aga_gpu_page_status_e {
    AGA_GPU_PAGE_STATUS_NONE         = 0,
    AGA_GPU_PAGE_STATUS_RESERVED     = 1,
    AGA_GPU_PAGE_STATUS_PENDING      = 2,
    AGA_GPU_PAGE_STATUS_UNRESERVABLE = 3,
} aga_gpu_page_status_t;

/// \brief GPU bad page record
typedef struct aga_gpu_bad_page_record_s {
    /// GPU key
    aga_obj_key_t key;
    /// page start address
    uint64_t page_address;
    /// page size
    uint64_t page_size;
    /// page status
    aga_gpu_page_status_t page_status;
} aga_gpu_bad_page_record_t;

/// \brief operational information of a physical GPU
typedef struct aga_gpu_status_s {
    /// assigned GPU index local to compute node
    uint32_t index;
    /// handle of gpu
    aga_gpu_handle_t handle;
    /// serial number
    char serial_num[AGA_MAX_STR_LEN + 1];
    /// GPU product series
    char card_series[AGA_MAX_STR_LEN + 1];
    /// GPU model
    char card_model[AGA_MAX_STR_LEN + 1];
    /// GPU vendor
    char card_vendor[AGA_MAX_STR_LEN + 1];
    /// GPU sku
    char card_sku[AGA_MAX_STR_LEN + 1];
    /// driver version
    char driver_version[AGA_MAX_STR_LEN + 1];
    /// vbios part number
    char vbios_part_number[AGA_MAX_STR_LEN + 1];
    /// vbios version
    char vbios_version[AGA_MAX_STR_LEN + 1];
    /// number of fw versions
    uint32_t num_fw_versions;
    /// firmware versions of various components
    aga_gpu_fw_version_t fw_version[AGA_GPU_MAX_FIRMWARE_VERSION];
    /// memory component vendor
    char memory_vendor[AGA_MAX_STR_LEN + 1];
    /// operational status of the device
    aga_gpu_oper_state_t oper_status;
    /// number of clock status
    uint32_t num_clock_status;
    /// GPU clock status
    aga_gpu_clock_status_t clock_status[AGA_GPU_MAX_CLOCK];
    /// number of Kenral Fusion Driver process ids using the GPU
    uint32_t num_kfd_process_id;
    /// Kernel Fusion Driver (KFD) process ids using the GPU
    uint32_t kfd_process_id[AGA_GPU_MAX_KFD_PID];
    /// GPU RAS status
    aga_gpu_ras_status_t ras_status;
    /// xgmi status
    aga_gpu_xgmi_status_t xgmi_status;
    /// PCIe status
    aga_gpu_pcie_status_t pcie_status;
    /// VRAM status
    aga_gpu_vram_status_t vram_status;
    /// voltage curve points
    aga_gpu_voltage_curve_point_t voltage_curve_point[AGA_GPU_MAX_VOLTAGE_CURVE_POINT];
    /// GPU throttling status
    aga_gpu_throttling_status_t throttling_status;
    /// firmware timestamp
    uint64_t fw_timestamp;
    /// GPU partition id
    uint32_t partition_id;
    /// GPU partitions (aka. child GPUs)
    /// NOTE:
    /// only valid for physical GPUs which have been partitioned
    uint32_t num_gpu_partition;
    aga_obj_key_t gpu_partition[AGA_GPU_MAX_PARTITION];
    /// physical GPU (aka. parent GPU)
    /// NOTE:
    /// only valid for GPU partitions (child GPUs)
    aga_obj_key_t physical_gpu;
} aga_gpu_status_t;

/// \brief GPU PCIe statistics
typedef struct aga_gpu_pcie_stats_s {
    /// total number of the replays issued on the PCIe link
    uint64_t replay_count;
    /// total number of times PCIe link transitioned from L0 to recovery state
    uint64_t recovery_count;
    /// total number of replay rollovers issued on the PCIe link
    uint64_t replay_rollover_count;
    /// total number of NACKs issued on the PCIe link by the device
    uint64_t nack_sent_count;
    /// total number of NACKs issued on the PCIe link by the receiver
    uint64_t nack_received_count;
} aga_gpu_pcie_stats_t;

/// \brief GPU voltage statistics
typedef struct aga_gpu_voltage_s {
    /// current voltage (in mV)
    uint64_t voltage;
    /// current graphics voltage (in mV)
    uint64_t gfx_voltage;
    /// current memory voltage (in mV)
    uint64_t memory_voltage;
} aga_gpu_voltage_t;

/// \brief GPU VRAM usage statistics
typedef struct aga_gpu_vram_usage_s {
    /// total VRAM (in MB)
    uint64_t total_vram;
    /// used VRAM (in MB)
    uint64_t used_vram;
    /// free VRAM (in MB)
    uint64_t free_vram;
    /// total visible VRAM (in MB)
    uint64_t total_visible_vram;
    /// used visible VRAM (in MB)
    uint64_t used_visible_vram;
    /// free visible VRAM (in MB)
    uint64_t free_visible_vram;
    /// total Graphic Translation Table (GTT) (in MB)
    uint64_t total_gtt;
    /// used GTT (in MB)
    uint64_t used_gtt;
    /// free GTT (in MB)
    uint64_t free_gtt;
} aga_gpu_vram_usage_t;

/// \brief GPU XGMI link statistics
typedef struct aga_gpu_xgmi_link_stats_s {
    /// data read in KB
    uint64_t data_read;
    /// data written in KB
    uint64_t data_write;
} aga_gpu_xgmi_link_stats_t;

/// \brief GPU violation statistics
typedef struct aga_gpu_violation_stats_s {
    /// current acummulated counter
    uint64_t current_accumulated_counter;
    /// processor hot residency accumulated
    uint64_t processor_hot_residency_accumulated;
    /// Package Power Tracking (PPT) residency accumulated
    uint64_t ppt_residency_accumulated;
    /// socket thermal residency accumulated
    uint64_t socket_thermal_residency_accumulated;
    /// Voltage Rail (VR) thermal residency accumulated
    uint64_t vr_thermal_residency_accumulated;
    /// High Bandwidth Memory (HBM) thermal residency accumulated
    uint64_t hbm_thermal_residency_accumulated;
} aga_gpu_violation_stats_t;

/// \brief GPU statistics
typedef struct aga_gpu_stats_s {
    /// current graphics package power (in Watts)
    uint64_t package_power;
    /// average package power (in Watts)
    uint64_t avg_package_power;
    /// current temperature
    aga_gpu_temperature_stats_t temperature;
    /// current GPU usage
    aga_gpu_usage_t usage;
    /// current voltage (in mV)
    aga_gpu_voltage_t voltage;
    /// GPU PCIe stats
    aga_gpu_pcie_stats_t pcie_stats;
    /// GPU VRAM usage stats
    aga_gpu_vram_usage_t vram_usage;
    /// accumulated energy consumed (in uJ)
    double energy_consumed;
    /// power usage (in Watts)
    uint32_t power_usage;
    /// total correctable errors
    uint64_t total_correctable_errors;
    /// total uncorrectable errors
    uint64_t total_uncorrectable_errors;
    /// SDMA correctable errors
    uint64_t sdma_correctable_errors;
    /// SDMA uncorrectable errors
    uint64_t sdma_uncorrectable_errors;
    /// GFX correctable errors
    uint64_t gfx_correctable_errors;
    /// GFX uncorrectable errors
    uint64_t gfx_uncorrectable_errors;
    /// MMHUB correctable errors
    uint64_t mmhub_correctable_errors;
    /// MMHUB uncorrectable errors
    uint64_t mmhub_uncorrectable_errors;
    /// ATHUB correctable errors
    uint64_t athub_correctable_errors;
    /// ATHUB uncorrectable errors
    uint64_t athub_uncorrectable_errors;
    /// BIF correctable errors
    uint64_t bif_correctable_errors;
    /// BIF uncorrectable errors
    uint64_t bif_uncorrectable_errors;
    /// HDP correctable errors
    uint64_t hdp_correctable_errors;
    /// HDP uncorrectable errors
    uint64_t hdp_uncorrectable_errors;
    /// XGMI WAFL correctable errors
    uint64_t xgmi_wafl_correctable_errors;
    /// XGMI WAFL uncorrectable errors
    uint64_t xgmi_wafl_uncorrectable_errors;
    /// DF correctable errors
    uint64_t df_correctable_errors;
    /// DF uncorrectable errors
    uint64_t df_uncorrectable_errors;
    /// SMN correctable errors
    uint64_t smn_correctable_errors;
    /// SMN uncorrectable errors
    uint64_t smn_uncorrectable_errors;
    /// SEM correctable errors
    uint64_t sem_correctable_errors;
    /// SEM uncorrectable errors
    uint64_t sem_uncorrectable_errors;
    /// MP0 correctable errors
    uint64_t mp0_correctable_errors;
    /// MP0 uncorrectable errors
    uint64_t mp0_uncorrectable_errors;
    /// MP1 correctable errors
    uint64_t mp1_correctable_errors;
    /// MP1 uncorrectable errors
    uint64_t mp1_uncorrectable_errors;
    /// FUSE correctable errors
    uint64_t fuse_correctable_errors;
    /// FUSE uncorrectable errors
    uint64_t fuse_uncorrectable_errors;
    /// UMC correctable errors
    uint64_t umc_correctable_errors;
    /// UMC uncorrectable errors
    uint64_t umc_uncorrectable_errors;
    /// MCA correctable errors
    uint64_t mca_correctable_errors;
    /// MCA uncorrectable errors
    uint64_t mca_uncorrectable_errors;
    /// VCN correctable errors
    uint64_t vcn_correctable_errors;
    /// VCN uncorrectable errors
    uint64_t vcn_uncorrectable_errors;
    /// JPEG correctable errors
    uint64_t jpeg_correctable_errors;
    /// JPEG uncorrectable errors
    uint64_t jpeg_uncorrectable_errors;
    /// IH correctable errors
    uint64_t ih_correctable_errors;
    /// IH uncorrectable errors
    uint64_t ih_uncorrectable_errors;
    /// MPIO correctable errors
    uint64_t mpio_correctable_errors;
    /// MPIO uncorrectable errors
    uint64_t mpio_uncorrectable_errors;
    /// XGMI counters
    /// NOPs sent to neighbor0
    uint64_t xgmi_neighbor0_tx_nops;
    /// outgoing requests to neighbor0
    uint64_t xgmi_neighbor0_tx_requests;
    /// outgoing responses to neighbor0
    uint64_t xgmi_neighbor0_tx_responses;
    /// data beats sent to neighbor0 (each beat = 32 Bytes)
    uint64_t xgmi_neighbor0_tx_beats;
    /// NOPs sent to neighbor1
    uint64_t xgmi_neighbor1_tx_nops;
    /// outgoing requests to neighbor1
    uint64_t xgmi_neighbor1_tx_requests;
    /// outgoing responses to neighbor1
    uint64_t xgmi_neighbor1_tx_responses;
    /// data beats sent to neighbor1 (each beat = 32 Bytes)
    uint64_t xgmi_neighbor1_tx_beats;
    /// transmit throughput to XGMI neighbor 0 (in Bytes per second)
    uint64_t xgmi_neighbor0_tx_throughput;
    /// transmit throughput to XGMI neighbor 1 (in Bytes per second)
    uint64_t xgmi_neighbor1_tx_throughput;
    /// transmit throughput to XGMI neighbor 2 (in Bytes per second)
    uint64_t xgmi_neighbor2_tx_throughput;
    /// transmit throughput to XGMI neighbor 3 (in Bytes per second)
    uint64_t xgmi_neighbor3_tx_throughput;
    /// transmit throughput to XGMI neighbor 4 (in Bytes per second)
    uint64_t xgmi_neighbor4_tx_throughput;
    /// transmit throughput to XGMI neighbor 5 (in Bytes per second)
    uint64_t xgmi_neighbor5_tx_throughput;
    /// fan speed in RPMs
    uint64_t fan_speed;
    /// graphics activity accumulated in %
    uint64_t gfx_activity_accumulated;
    /// memory activity accumulated in %
    uint64_t mem_activity_accumulated;
    /// XGMI link statistics
    aga_gpu_xgmi_link_stats_t xgmi_link_stats[AGA_GPU_MAX_XGMI_LINKS];
    /// GPU violation statistics
    aga_gpu_violation_stats_t violation_stats;
} aga_gpu_stats_t;

/// GPU info
typedef struct aga_gpu_info_s {
    aga_gpu_spec_t spec;
    aga_gpu_status_t status;
    aga_gpu_stats_t stats;
} aga_gpu_info_t;

/// device type enum
typedef enum aga_device_type_e {
    AGA_DEVICE_TYPE_NONE = 0,
    /// GPU device
    AGA_DEVICE_TYPE_GPU  = 1,
} aga_device_type_t;

/// IO link type enum
typedef enum aga_io_link_type_e {
    AGA_IO_LINK_TYPE_NONE = 0,
    /// PCIe connection to the device
    AGA_IO_LINK_TYPE_PCIE = 1,
    /// XGMI connection to the device
    AGA_IO_LINK_TYPE_XGMI = 2,
} aga_io_link_type_t;

typedef struct aga_device_connection_s {
    /// IO link type of connection
    aga_io_link_type_t type;
} aga_device_connection_t;

/// device structure
typedef struct aga_device_s {
    /// device type
    aga_device_type_t type;
    /// device name
    char name[AGA_MAX_STR_LEN + 1];
} aga_device_t;

/// peer device info
typedef struct aga_peer_device_s {
    /// peer device valid
    bool valid;
    /// peer device
    aga_device_t peer_device;
    /// connection details to the peer device
    aga_device_connection_t connection;
    /// distance in terms of no. of hops to the peer device
    uint64_t num_hops;
    /// weight assigned to the connection to peer device
    uint64_t link_weight;
} aga_peer_device_t;

/// device topology info
typedef struct aga_device_topology_info_s {
    /// device under consideration
    aga_device_t device;
    /// list of peer devices and corresponding inter-connection details
    aga_peer_device_t peer_device[AGA_MAX_PEER_DEVICE];
} aga_device_topology_info_t;

/// GPU compute partition info
typedef struct aga_gpu_compute_partition_info_s {
    /// physical GPU
    aga_obj_key_t physical_gpu;
    /// compute partition type
    aga_gpu_compute_partition_type_t compute_partition_type;
    /// GPU partitions (child GPUs)
    uint32_t num_gpu_partition;
    aga_obj_key_t gpu_partition[AGA_GPU_MAX_PARTITION];
} aga_gpu_compute_partition_info_t;

/// \brief     create gpu
/// \param[in] spec config specification
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_gpu_create(_In_ aga_gpu_spec_t *spec);

/// \brief      read gpu
/// \param[in]  key  key of the gpu object
/// \param[out] info information
/// \return     #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_gpu_read(_In_ aga_obj_key_t *key, _Out_ aga_gpu_info_t *info);

typedef void (*gpu_read_cb_t)(aga_gpu_info_t *info, void *ctxt);

/// \brief    read all gpu information
/// \param[in]  cb      callback function
/// \param[in]  ctxt    opaque context passed to cb
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_gpu_read_all(_In_ gpu_read_cb_t gpu_read_cb, _In_ void *ctxt);

/// \brief      function to get compute partition info of a given physical gpu
///             which has been partitioned
/// \param[in]  key  key of the physical gpu object which has been partitioned
/// \param[out] info information
/// \return     #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_gpu_compute_partition_read(_In_ aga_obj_key_t *key,
              _Out_ aga_gpu_compute_partition_info_t *info);

typedef void (*gpu_compute_partition_read_cb_t)
    (aga_gpu_compute_partition_info_t *info, void *ctxt);

/// \brief      read compute partition info of all physical gpus which have been
///             partitioned
/// \param[in]  cb      callback function
/// \param[in]  ctxt    opaque context passed to cb
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_gpu_compute_partition_read_all(
              _In_ gpu_compute_partition_read_cb_t gpu_read_cb,
              _In_ void *ctxt);

typedef void (*gpu_bad_page_read_cb_t)(uint32_t num_bad_pages,
                                       aga_gpu_bad_page_record_t *records,
                                       void *ctxt);

/// \brief    read gpu bad page records
/// \param[in]  key     key of the gpu object, if k_aga_obj_key_invalid we read
///                     bad page records of all gpu
/// \param[in]  cb      callback function
/// \param[in]  ctxt    opaque context passed to cb
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_gpu_bad_page_read(_In_ aga_obj_key_t *key,
              _In_ gpu_bad_page_read_cb_t gpu_bad_page_read_cb,
              _In_ void *ctxt);

typedef void (*device_topology_read_cb_t)(aga_device_topology_info_t *info,
                                          void *ctxt);

/// \brief    read all gpu topology information
/// \param[in]  cb      callback function
/// \param[in]  ctxt    opaque context passed to cb
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_gpu_read_topology_all(device_topology_read_cb_t gpu_read_cb,
                                    void *ctxt);

/// \brief     update gpu
/// \param[in] spec specification
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_gpu_update(_In_ aga_gpu_spec_t *spec);

/// \brief     delete gpu object
/// \param[in] key key
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t aga_gpu_delete(_In_ aga_obj_key_t *key);

#endif    /// __API_INCLUDE_AGA_GPU_HPP__
