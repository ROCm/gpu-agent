//
// Copyright(C) Advanced Micro Devices, Inc. All rights reserved.
//
// You may not use this software and documentation (if any) (collectively,
// the "Materials") except in compliance with the terms and conditions of
// the Software License Agreement included with the Materials or otherwise as
// set forth in writing and signed by you and an authorized signatory of AMD.
// If you do not have a copy of the Software License Agreement, contact your
// AMD representative for a copy.
//
// You agree that you will not reverse engineer or decompile the Materials,
// in whole or in part, except as allowed by applicable law.
//
// THE MATERIALS ARE DISTRIBUTED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OR
// REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//----------------------------------------------------------------------------
///
/// \file
/// smi layer API definitions
///
//----------------------------------------------------------------------------

#include <sstream>
#include <iomanip>
extern "C" {
#include "nic/third-party/rocm/gim_amd_smi_lib/include/amd_smi/amdsmi.h"
}
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/api/gpu.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/smi/smi_api.hpp"
#include "nic/gpuagent/api/smi/smi_state.hpp"
#include "nic/gpuagent/api/smi/gimamdsmi/smi_utils.hpp"

// TODO:
// not using aga_ here for proper naming !!!

namespace aga {

#define AMDSMI_DEEP_SLEEP_THRESHOLD     140
#define AMDSMI_UINT32_INVALID_VAL       0xffffffff
#define AMDSMI_UINT64_INVALID_VAL       0xffffffffffffffff
#define CPER_BUF_SIZE                   (4 * 1024 * 1024) // 4 MB

/// \brief struct to be used as ctxt when walking GPU db to build topology
typedef struct gpu_topo_walk_ctxt_s {
    uint32_t count;
    gpu_entry *gpu;
    aga_device_topology_info_t *info;
} gpu_topo_walk_ctxt_t;

/// \brief    fill clock frequency ranges of the given GPU
/// \param[in] gpu_handle    GPU handle
/// \param[out] spec     spec to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_gpu_clock_frequency_spec_ (aga_gpu_handle_t gpu_handle,
                                    aga_gpu_spec_t *spec)
{
    uint32_t clk_cnt = 0;
    amdsmi_status_t amdsmi_ret;
    amdsmi_clk_info_t info = {};
    aga_gpu_clock_freq_range_t *clock_spec;

    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_GFX,
                                       &info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get system clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_spec = &spec->clock_freq[clk_cnt];
        clock_spec->clock_type = smi_to_aga_gpu_clock_type(AMDSMI_CLK_TYPE_GFX);
        // min and max frequencies are per clock type
        clock_spec->lo = info.min_clk;
        clock_spec->hi = info.max_clk;
        clk_cnt++;
    }
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_MEM,
                                       &info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get memory clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_spec = &spec->clock_freq[clk_cnt];
        clock_spec->clock_type = smi_to_aga_gpu_clock_type(AMDSMI_CLK_TYPE_MEM);
        // min and max frequencies are per clock type
        clock_spec->lo = info.min_clk;
        clock_spec->hi = info.max_clk;
        clk_cnt++;
    }
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_VCLK0,
                                       &info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get video clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_spec = &spec->clock_freq[clk_cnt];
        clock_spec->clock_type =
            smi_to_aga_gpu_clock_type(AMDSMI_CLK_TYPE_VCLK0);
        // min and max frequencies are per clock type
        clock_spec->lo = info.min_clk;
        clock_spec->hi = info.max_clk;
        clk_cnt++;
    }
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_DCLK0,
                                       &info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get data clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_spec = &spec->clock_freq[clk_cnt];
        clock_spec->clock_type =
            smi_to_aga_gpu_clock_type(AMDSMI_CLK_TYPE_DCLK0);
        // min and max frequencies are per clock type
        clock_spec->lo = info.min_clk;
        clock_spec->hi = info.max_clk;
        clk_cnt++;
    }
    spec->num_clock_freqs = clk_cnt;
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_spec (aga_gpu_handle_t gpu_handle, aga_gpu_spec_t *spec)
{
    amdsmi_status_t amdsmi_ret;
    amdsmi_power_cap_info_t power_cap_info = {};

    // fill fields not avaiable with gim amdsmi library
    spec->overdrive_level = AMDSMI_UINT32_INVALID_VAL;
    spec->perf_level = AGA_GPU_PERF_LEVEL_NONE;
    spec->fan_speed = AMDSMI_UINT64_INVALID_VAL;
    // fill the power cap
    amdsmi_ret = amdsmi_get_power_cap_info(gpu_handle, 0, &power_cap_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get power cap information for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        spec->num_gpu_power_cap = 1;
        spec->gpu_power_cap[0].type = AGA_GPU_POWER_CAP_TYPE_PPT0;
        spec->gpu_power_cap[0].power_cap = power_cap_info.power_cap/1000000;
    }
    // TODO: get admin_state
    // TODO: get RAS spec
    return SDK_RET_OK;
}

/// \brief     function to get name for amdsmi firmware block enum
/// \param[in] block    amdsmi firmware block enum
/// \return    firmware block name
static inline const char *
gpu_fw_block_name_str_ (amdsmi_fw_block_t block)
{
    switch (block) {
    case AMDSMI_FW_ID_SMU:
        return "SMU";
    case AMDSMI_FW_ID_CP_CE:
        return "CP_CE";
    case AMDSMI_FW_ID_CP_PFP:
        return "CP_PFP";
    case AMDSMI_FW_ID_CP_ME:
        return "CP_ME";
    case AMDSMI_FW_ID_CP_MEC_JT1:
        return "CP_MEC_JT1";
    case AMDSMI_FW_ID_CP_MEC_JT2:
        return "CP_MEC_JT2";
    case AMDSMI_FW_ID_CP_MEC1:
        return "CP_MEC1";
    case AMDSMI_FW_ID_CP_MEC2:
        return "CP_MEC2";
    case AMDSMI_FW_ID_RLC:
        return "RLC";
    case AMDSMI_FW_ID_SDMA0:
        return "SDMA0";
    case AMDSMI_FW_ID_SDMA1:
        return "SDMA1";
    case AMDSMI_FW_ID_SDMA2:
        return "SDMA2";
    case AMDSMI_FW_ID_SDMA3:
        return "SDMA3";
    case AMDSMI_FW_ID_SDMA4:
        return "SDMA4";
    case AMDSMI_FW_ID_SDMA5:
        return "SDMA5";
    case AMDSMI_FW_ID_SDMA6:
        return "SDMA6";
    case AMDSMI_FW_ID_SDMA7:
        return "SDMA7";
    case AMDSMI_FW_ID_VCN:
        return "VCN";
    case AMDSMI_FW_ID_UVD:
        return "UVD";
    case AMDSMI_FW_ID_VCE:
        return "VCE";
    case AMDSMI_FW_ID_ISP:
        return "ISP";
    case AMDSMI_FW_ID_DMCU_ERAM:
        return "DMCU_ERAM";
    case AMDSMI_FW_ID_DMCU_ISR:
        return "DMCU_ISR";
    case AMDSMI_FW_ID_RLC_RESTORE_LIST_GPM_MEM:
        return "RLC_GPM_MEM";
    case AMDSMI_FW_ID_RLC_RESTORE_LIST_SRM_MEM:
        return "RLC_SRM_MEM";
    case AMDSMI_FW_ID_RLC_RESTORE_LIST_CNTL:
        return "RLC_CNTL";
    case AMDSMI_FW_ID_RLC_V:
        return "RLC_V";
    case AMDSMI_FW_ID_MMSCH:
        return "MMSCH";
    case AMDSMI_FW_ID_PSP_SYSDRV:
        return "PSP_SYSDRV";
    case AMDSMI_FW_ID_PSP_SOSDRV:
        return "PSP_SOSDRV";
    case AMDSMI_FW_ID_PSP_TOC:
        return "PSP_TOC";
    case AMDSMI_FW_ID_PSP_KEYDB:
        return "PSP_KEYDB";
    case AMDSMI_FW_ID_DFC:
        return "DFC";
    case AMDSMI_FW_ID_PSP_SPL:
        return "PSP_SPL";
    case AMDSMI_FW_ID_DRV_CAP:
        return "DRV_CAP";
    case AMDSMI_FW_ID_MC:
        return "MC";
    case AMDSMI_FW_ID_PSP_BL:
        return "PSP_BL";
    case AMDSMI_FW_ID_CP_PM4:
        return "CP_PM4";
    case AMDSMI_FW_ID_RLC_P:
        return "RLC_P";
    case AMDSMI_FW_ID_SEC_POLICY_STAGE2:
        return "SEC_POL_STG2";
    case AMDSMI_FW_ID_REG_ACCESS_WHITELIST:
        return "REG_ACCESS_WL";
    case AMDSMI_FW_ID_IMU_DRAM:
        return "IMU_DRAM";
    case AMDSMI_FW_ID_IMU_IRAM:
        return "IMU_IRAM";
    case AMDSMI_FW_ID_SDMA_TH0:
        return "SDMA_TH0";
    case AMDSMI_FW_ID_SDMA_TH1:
        return "SDMA_TH1";
    case AMDSMI_FW_ID_CP_MES:
        return "CP_MES";
    case AMDSMI_FW_ID_MES_STACK:
        return "MES_STACK";
    case AMDSMI_FW_ID_MES_THREAD1:
        return "MES_THREAD1";
    case AMDSMI_FW_ID_MES_THREAD1_STACK:
        return "MES_THREAD1_STACK";
    case AMDSMI_FW_ID_RLX6:
        return "RLX6";
    case AMDSMI_FW_ID_RLX6_DRAM_BOOT:
        return "RLX6_DRAM_BOOT";
    case AMDSMI_FW_ID_RS64_ME:
        return "RS64_ME";
    case AMDSMI_FW_ID_RS64_ME_P0_DATA:
        return "RS64_ME_P0_DATA";
    case AMDSMI_FW_ID_RS64_ME_P1_DATA:
        return "RS64_ME_P1_DATA";
    case AMDSMI_FW_ID_RS64_PFP:
        return "RS64_PFP";
    case AMDSMI_FW_ID_RS64_PFP_P0_DATA:
        return "RS64_PFP_P0_DATA";
    case AMDSMI_FW_ID_RS64_PFP_P1_DATA:
        return "RS64_PFP_P1_DATA";
    case AMDSMI_FW_ID_RS64_MEC:
        return "RS64_MEC";
    case AMDSMI_FW_ID_RS64_MEC_P0_DATA:
        return "RS64_MEC_P0_DATA";
    case AMDSMI_FW_ID_RS64_MEC_P1_DATA:
        return "RS64_MEC_P1_DATA";
    case AMDSMI_FW_ID_RS64_MEC_P2_DATA:
        return "RS64_MEC_P2_DATA";
    case AMDSMI_FW_ID_RS64_MEC_P3_DATA:
        return "RS64_MEC_P3_DATA";
    case AMDSMI_FW_ID_PPTABLE:
        return "PPTABLE";
    case AMDSMI_FW_ID_PSP_SOC:
        return "PSP_SOC";
    case AMDSMI_FW_ID_PSP_DBG:
        return "PSP_DBG";
    case AMDSMI_FW_ID_PSP_INTF:
        return "PSP_INTF";
    case AMDSMI_FW_ID_RLX6_CORE1:
        return "RLX6_CORE1";
    case AMDSMI_FW_ID_RLX6_DRAM_BOOT_CORE1:
        return "RLX6_DRAM_BOOT_CORE1";
    case AMDSMI_FW_ID_RLCV_LX7:
        return "RLCV_LX7";
    case AMDSMI_FW_ID_RLC_SAVE_RESTORE_LIST:
        return "RLC_SAVE_RL";
    case AMDSMI_FW_ID_ASD:
        return "ASD";
    case AMDSMI_FW_ID_TA_RAS:
        return "TA_RAS";
    case AMDSMI_FW_ID_XGMI:
        return "XGMI";
    case AMDSMI_FW_ID_RLC_SRLG:
        return "RLC_SRLG";
    case AMDSMI_FW_ID_RLC_SRLS:
        return "RLC_SRLS";
    case AMDSMI_FW_ID_SMC:
        return "SMC";
    case AMDSMI_FW_ID_DMCU:
        return "DMCU";
    case AMDSMI_FW_ID_PSP_RAS:
        return "PSP_RAS";
    case AMDSMI_FW_ID_P2S_TABLE:
        return "P2S_TABLE";
    default:
        return (std::string("FW_ID_")+ std::to_string(block)).c_str();
    }
}

/// \brief      function to format firmware version
/// \param[out] fw_version    firmware component/version after formatting
/// \param[in]  block         firmware component enum
/// \param[in]  version       firmware version
/// \return     none
static void
fill_gpu_fw_version_ (aga_gpu_fw_version_t *fw_version, amdsmi_fw_block_t block,
                      uint64_t version)
{
    char buf[AGA_MAX_STR_LEN + 1];
    std::string block_name = gpu_fw_block_name_str_(block);

    strncpy(fw_version->firmware, block_name.c_str(), AGA_MAX_STR_LEN);
    if ((block == AMDSMI_FW_ID_VCN) || (block == AMDSMI_FW_ID_UVD) ||
        (block == AMDSMI_FW_ID_VCE) ||
        (block == AMDSMI_FW_ID_ASD) || (block == AMDSMI_FW_ID_CP_MES) ||
        (block == AMDSMI_FW_ID_PSP_SOSDRV)) {
        // 'VCN', 'VCE', 'UVD', 'SOS', 'ASD', 'MES', 'MES KIQ' fw versions
        // needs to hexadecimal
        snprintf(buf, AGA_MAX_STR_LEN, "0x%08" PRIx64, version);
        strncpy(fw_version->version, buf, AGA_MAX_STR_LEN);
    } else if ((block == AMDSMI_FW_ID_XGMI) ||
               (block == AMDSMI_FW_ID_TA_RAS)) {
        // XGMI and TA RAS firmware's hex value looks like 0x12345678
        // however, they are parsed as: int(0x12).int(0x34).int(0x56).int(0x78)
        // which results in the following: 12.34.56.78
        unsigned char tmp[8];
        for (auto i = 0; i < 8; i++) {
            tmp[i] = version >> ((7-i)*8);
        }
        snprintf(buf, AGA_MAX_STR_LEN, "%02u.%02u.%02u.%02u",
                 tmp[4], tmp[5], tmp[6], tmp[7]);
        strncpy(fw_version->version, buf, AGA_MAX_STR_LEN);
    } else {
        strncpy(fw_version->version, std::to_string(version).c_str(),
                AGA_MAX_STR_LEN);
    }
}

/// \brief      get SKU from VBIOS version
/// \param[in]  vbios    VBIOS part number string
/// \param[out] sku          SKU string dervied from vbios version
/// \return     none
static void
gpu_get_sku_from_vbios_ (char *sku, char *vbios)
{
    char *buf;
    char *token;

    // middle portion in the VBIOS version is SKU XXX-<CARD_SKU>-XXX
    // get first token
    token = strtok_r(vbios, "-", &buf);
    if (token == NULL) {
        AGA_TRACE_ERR("SKU cannot be derived from vbios version {}", vbios);
        return;
    }
    // second token is the SKU
    token = strtok_r(NULL, "-", &buf);
    if (token == NULL) {
        AGA_TRACE_ERR("SKU cannot be derived from vbios version {}", vbios);
        return;
    }
    strncpy(sku, token, AGA_MAX_STR_LEN);
}

/// \brief    fill status of clocks
/// \param[in] gpu_handle    GPU handle
/// \param[out] status    operational status to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_clock_status_ (aga_gpu_handle_t gpu_handle,
                        aga_gpu_status_t *status)
{
    uint32_t clk_cnt = 0;
    amdsmi_status_t amdsmi_ret;
    amdsmi_clk_info_t clock_info;
    aga_gpu_clock_status_t *clock_status;

    // get clock status information from amdsmi_get_clock_info
    clk_cnt = 0;
    // gfx clocks
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_GFX,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get system clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_status = &status->clock_status[clk_cnt];
        clock_status->clock_type = AGA_GPU_CLOCK_TYPE_SYSTEM;
        clock_status->frequency = clock_info.clk;
        clock_status->low_frequency = clock_info.min_clk;
        clock_status->high_frequency = clock_info.max_clk;
        clock_status->locked = clock_info.clk_locked;
        clock_status->deep_sleep = clock_info.clk_deep_sleep;
        clk_cnt++;
    }
    // memory clock
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_MEM,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get memory clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_status = &status->clock_status[clk_cnt];
        clock_status->clock_type = AGA_GPU_CLOCK_TYPE_MEMORY;
        clock_status->frequency = clock_info.clk;
        clock_status->low_frequency = clock_info.min_clk;
        clock_status->high_frequency = clock_info.max_clk;
        clock_status->locked = clock_info.clk_locked;
        clock_status->deep_sleep = clock_info.clk_deep_sleep;
        clk_cnt++;
    }
    // video clocks
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_VCLK0,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get video clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_status = &status->clock_status[clk_cnt];
        clock_status->clock_type = AGA_GPU_CLOCK_TYPE_VIDEO;
        clock_status->frequency = clock_info.clk;
        clock_status->low_frequency = clock_info.min_clk;
        clock_status->high_frequency = clock_info.max_clk;
        clock_status->locked = clock_info.clk_locked;
        clock_status->deep_sleep = clock_info.clk_deep_sleep;
        clk_cnt++;
    }
    // data clocks
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_DCLK0,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get data clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_status = &status->clock_status[clk_cnt];
        clock_status->clock_type = AGA_GPU_CLOCK_TYPE_DATA;
        clock_status->frequency = clock_info.clk;
        clock_status->low_frequency = clock_info.min_clk;
        clock_status->high_frequency = clock_info.max_clk;
        clock_status->locked = clock_info.clk_locked;
        clock_status->deep_sleep = clock_info.clk_deep_sleep;
        clk_cnt++;
    }
    // data fabric clock
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_DF,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get fabric clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_status = &status->clock_status[clk_cnt];
        clock_status->clock_type = AGA_GPU_CLOCK_TYPE_FABRIC;
        clock_status->frequency = clock_info.clk;
        clock_status->low_frequency = clock_info.min_clk;
        clock_status->high_frequency = clock_info.max_clk;
        clock_status->locked = clock_info.clk_locked;
        clock_status->deep_sleep = clock_info.clk_deep_sleep;
        clk_cnt++;
    }
    // DCE clock
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_DCEF,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get DCE clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_status = &status->clock_status[clk_cnt];
        clock_status->clock_type = AGA_GPU_CLOCK_TYPE_DCE;
        clock_status->frequency = clock_info.clk;
        clock_status->low_frequency = clock_info.min_clk;
        clock_status->high_frequency = clock_info.max_clk;
        clock_status->locked = clock_info.clk_locked;
        clock_status->deep_sleep = clock_info.clk_deep_sleep;
        clk_cnt++;
    }
    // SOC clock
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_SOC,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get SOC clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_status = &status->clock_status[clk_cnt];
        clock_status->clock_type = AGA_GPU_CLOCK_TYPE_SOC;
        clock_status->frequency = clock_info.clk;
        clock_status->low_frequency = clock_info.min_clk;
        clock_status->high_frequency = clock_info.max_clk;
        clock_status->locked = clock_info.clk_locked;
        clock_status->deep_sleep = clock_info.clk_deep_sleep;
        clk_cnt++;
    }
    // PCIe clock
    amdsmi_ret = amdsmi_get_clock_info(gpu_handle, AMDSMI_CLK_TYPE_PCIE,
                                       &clock_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get PCIe clock info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        clock_status = &status->clock_status[clk_cnt];
        clock_status->clock_type = AGA_GPU_CLOCK_TYPE_PCIE;
        clock_status->frequency = clock_info.clk;
        clock_status->low_frequency = clock_info.min_clk;
        clock_status->high_frequency = clock_info.max_clk;
        clock_status->locked = clock_info.clk_locked;
        clock_status->deep_sleep = clock_info.clk_deep_sleep;
        clk_cnt++;
    }
    status->num_clock_status = clk_cnt;
    return SDK_RET_OK;
}

/// \brief    fill PCIe status
/// \param[in] gpu_handle    GPU handle
/// \param[out] status    operational status to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_pcie_status_ (aga_gpu_handle_t gpu_handle,
                       aga_gpu_status_t *status)
{
    amdsmi_bdf_t bdf;
    amdsmi_pcie_info_t info;
    amdsmi_status_t amdsmi_ret;
    aga_gpu_pcie_status_t *pcie_status = &status->pcie_status;

    amdsmi_ret = amdsmi_get_pcie_info(gpu_handle, &info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get PCIe info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        pcie_status->slot_type =
            smi_to_aga_pcie_slot_type(info.pcie_static.slot_type);
        pcie_status->max_width = info.pcie_static.max_pcie_width;
        pcie_status->max_speed = info.pcie_static.max_pcie_speed/1000;
        pcie_status->version = info.pcie_static.pcie_interface_version;
        pcie_status->width = info.pcie_metric.pcie_width;
        pcie_status->speed = info.pcie_metric.pcie_speed/1000;
        pcie_status->bandwidth = info.pcie_metric.pcie_bandwidth;
    }
    amdsmi_ret = amdsmi_get_gpu_device_bdf(gpu_handle, &bdf);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get PCIe bus id for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
        return amdsmi_ret_to_sdk_ret(amdsmi_ret);
    } else {
        // convert PCIe bus to XXXX.XX.XX.X format
        snprintf(pcie_status->pcie_bus_id, AGA_MAX_STR_LEN, "%04X:%02X:%02X.%X",
                ((uint32_t)((bdf.as_uint >> 32) & 0xffffffff)),
                ((uint32_t)((bdf.as_uint >> 8) & 0xff)),
                ((uint32_t)((bdf.as_uint >> 3) & 0x1f)),
                ((uint32_t)(bdf.as_uint & 0x7)));
    }
    return SDK_RET_OK;
}

/// \brief    fill VRAM status
/// \param[in] gpu_handle    GPU handle
/// \param[out] status    operational status to be filled
/// \return SDK_RET_OK or error code in case of failure
static sdk_ret_t
smi_fill_vram_status_ (aga_gpu_handle_t gpu_handle,
                       aga_gpu_status_t *status)
{
    amdsmi_vram_info_t info;
    amdsmi_status_t amdsmi_ret;
    aga_gpu_vram_status_t *vram_status = &status->vram_status;

    amdsmi_ret = amdsmi_get_gpu_vram_info(gpu_handle, &info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get VRAM info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        vram_status->type = smi_to_aga_vram_type(info.vram_type);
        smi_vram_vendor_to_string(info.vram_vendor, vram_status->vendor,
                                  AGA_MAX_STR_LEN);
        smi_vram_vendor_to_string(info.vram_vendor, status->memory_vendor,
                                  AGA_MAX_STR_LEN);
        vram_status->size = info.vram_size;
    }
    return SDK_RET_OK;
}

static sdk_ret_t
smi_fill_ecc_stats_ (aga_gpu_handle_t gpu_handle,
                     aga_gpu_stats_t *stats)
{
    amdsmi_error_count_t ec;
    amdsmi_status_t amdsmi_ret;
    uint64_t total_deferred_count = 0;
    uint64_t total_correctable_count = 0;
    uint64_t total_uncorrectable_count = 0;
    // get correctable and uncorrectable total error count beforehand
    for (uint32_t b = AMDSMI_GPU_BLOCK_FIRST; b <= AMDSMI_GPU_BLOCK_LAST;
         b = b * 2) {
        // initialize ec to all 0s
        ec = { 0 };
        amdsmi_ret = amdsmi_get_gpu_ecc_count(gpu_handle,
                                              (amdsmi_gpu_block_t)(b), &ec);
        if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
            total_correctable_count += ec.correctable_count;
            total_uncorrectable_count += ec.uncorrectable_count;
            total_deferred_count += ec.deferred_count;
            switch (b) {
            case AMDSMI_GPU_BLOCK_UMC:
                stats->umc_correctable_errors =
                    ec.correctable_count;
                stats->umc_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->umc_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_SDMA:
                stats->sdma_correctable_errors =
                    ec.correctable_count;
                stats->sdma_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->sdma_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_GFX:
                stats->gfx_correctable_errors =
                    ec.correctable_count;
                stats->gfx_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->gfx_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_MMHUB:
                stats->mmhub_correctable_errors =
                    ec.correctable_count;
                stats->mmhub_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->mmhub_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_ATHUB:
                stats->athub_correctable_errors =
                    ec.correctable_count;
                stats->athub_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->athub_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_PCIE_BIF:
                stats->bif_correctable_errors =
                    ec.correctable_count;
                stats->bif_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->bif_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_HDP:
                stats->hdp_correctable_errors =
                    ec.correctable_count;
                stats->hdp_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->hdp_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_XGMI_WAFL:
                stats->xgmi_wafl_correctable_errors =
                    ec.correctable_count;
                stats->xgmi_wafl_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->xgmi_wafl_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_DF:
                stats->df_correctable_errors =
                    ec.correctable_count;
                stats->df_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->df_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_SMN:
                stats->smn_correctable_errors =
                    ec.correctable_count;
                stats->smn_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->smn_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_SEM:
                stats->sem_correctable_errors =
                    ec.correctable_count;
                stats->sem_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->sem_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_MP0:
                stats->mp0_correctable_errors =
                    ec.correctable_count;
                stats->mp0_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->mp0_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_MP1:
                stats->mp1_correctable_errors =
                    ec.correctable_count;
                stats->mp1_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->mp1_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_FUSE:
                stats->fuse_correctable_errors =
                    ec.correctable_count;
                stats->fuse_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->fuse_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_MCA:
                stats->mca_correctable_errors =
                    ec.correctable_count;
                stats->mca_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->mca_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_VCN:
                stats->vcn_correctable_errors =
                    ec.correctable_count;
                stats->vcn_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->vcn_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_JPEG:
                stats->jpeg_correctable_errors =
                    ec.correctable_count;
                stats->jpeg_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->jpeg_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_IH:
                stats->ih_correctable_errors =
                    ec.correctable_count;
                stats->ih_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->ih_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_MPIO:
                stats->mpio_correctable_errors =
                    ec.correctable_count;
                stats->mpio_uncorrectable_errors =
                    ec.uncorrectable_count;
                stats->mpio_deferred_errors =
                    ec.deferred_count;
                break;
            default:
                break;
            }
        }
    }
    stats->total_correctable_errors = total_correctable_count;
    stats->total_uncorrectable_errors = total_uncorrectable_count;
    stats->total_deferred_errors = total_deferred_count;
}

sdk_ret_t
smi_get_gpu_partition_id (aga_gpu_handle_t gpu_handle, uint32_t *partition_id)
{
    // for now set partition id as 0
    *partition_id = 0;
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_fill_status (aga_gpu_handle_t gpu_handle, uint32_t gpu_id,
                     aga_gpu_spec_t *spec, aga_gpu_status_t *status)
{
    // fill fields not avaiable with gim amdsmi library
    status->xgmi_status.width = AMDSMI_UINT64_INVALID_VAL;
    status->xgmi_status.speed = AMDSMI_UINT64_INVALID_VAL;
    status->xgmi_status.error_status = AGA_GPU_XGMI_STATUS_NONE;

    // fill the clock status without metrics info
    smi_fill_clock_status_(gpu_handle, status);
    // fill the PCIe status
    smi_fill_pcie_status_(gpu_handle, status);
    // TODO: oper status
    // TODO: RAS status
    return SDK_RET_OK;
}

/// \brief function to get number of bad pages for GPU
/// \param[in]  gpu             GPU object
/// \param[out] num_bad_pages   number of bad pages
/// \return SDK_RET_OK or error code in case of failure
sdk_ret_t
smi_gpu_get_bad_page_count (void *gpu_obj,
                            uint32_t *num_bad_pages)
{
    return SDK_RET_INVALID_OP;
}

/// \brief function to get GPU bad page records
/// \param[in]  gpu           GPU object
/// \param[in]  num_bad_pages number of bad pages
/// \param[out] records       GPU bad page records
/// \return SDK_RET_OK or error code in case of failure
sdk_ret_t
smi_gpu_get_bad_page_records (void *gpu_obj,
                              uint32_t num_bad_pages,
                              aga_gpu_bad_page_record_t *records)
{
    return SDK_RET_INVALID_OP;
}

sdk_ret_t
smi_gpu_fill_stats (aga_gpu_handle_t gpu_handle,
                    bool is_partitioned,
                    uint32_t partition_id,
                    aga_gpu_handle_t first_partition_handle,
                    aga_gpu_stats_t *stats)
{
    sdk_ret_t ret;
    int64_t temperature;
    amdsmi_status_t amdsmi_ret;
    amdsmi_pcie_info_t pcie_info = {};
    amdsmi_power_info_t power_info = {};
    amdsmi_engine_usage_t usage_info = {};

    // fill the power and voltage info
    amdsmi_ret = amdsmi_get_power_info(gpu_handle, 0, &power_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get power information for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->package_power = power_info.socket_power;
        stats->voltage.voltage = power_info.soc_voltage;
        stats->voltage.gfx_voltage = power_info.gfx_voltage;
        stats->voltage.memory_voltage = power_info.mem_voltage;
    }
    // fill the GPU usage
    amdsmi_ret = amdsmi_get_gpu_activity(gpu_handle, &usage_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get GPU activity for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->usage.umc_activity = usage_info.umc_activity;
        stats->usage.mm_activity = usage_info.mm_activity;
        stats->usage.gfx_activity = usage_info.gfx_activity;
    }
    // fill the PCIe stats
    amdsmi_ret = amdsmi_get_pcie_info(gpu_handle, &pcie_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get PCIe info for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->pcie_stats.replay_count =
            pcie_info.pcie_metric.pcie_replay_count;
        stats->pcie_stats.recovery_count =
            pcie_info.pcie_metric.pcie_l0_to_recovery_count;
        stats->pcie_stats.replay_rollover_count =
            pcie_info.pcie_metric.pcie_replay_roll_over_count;
        stats->pcie_stats.nack_sent_count =
            pcie_info.pcie_metric.pcie_nak_sent_count;
        stats->pcie_stats.nack_received_count =
            pcie_info.pcie_metric.pcie_nak_received_count;
    }
    // fill the edge temperature
    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle,
                                        AMDSMI_TEMPERATURE_TYPE_EDGE,
                                        AMDSMI_TEMP_CURRENT, &temperature);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get edge temperature for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->temperature.edge_temperature = (float)temperature;
    }
    // fill the junction temperature
    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle,
                                        AMDSMI_TEMPERATURE_TYPE_JUNCTION,
                                        AMDSMI_TEMP_CURRENT, &temperature);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get junction temperature for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->temperature.junction_temperature = (float)temperature;
    }
    // fill the memory temperature
    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle,
                                        AMDSMI_TEMPERATURE_TYPE_VRAM,
                                        AMDSMI_TEMP_CURRENT, &temperature);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get VRAM temperature for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->temperature.memory_temperature = (float)temperature;
    }
    // fill the HBM0 temperature
    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle,
                                        AMDSMI_TEMPERATURE_TYPE_HBM_0,
                                        AMDSMI_TEMP_CURRENT, &temperature);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get HBM0 temperature for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->temperature.hbm_temperature[0] = (float)temperature;
    }
    // fill the HBM1 temperature
    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle,
                                        AMDSMI_TEMPERATURE_TYPE_HBM_1,
                                        AMDSMI_TEMP_CURRENT, &temperature);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get HBM1 temperature for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->temperature.hbm_temperature[1] = (float)temperature;
    }
    // fill the HBM2 temperature
    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle,
                                        AMDSMI_TEMPERATURE_TYPE_HBM_2,
                                        AMDSMI_TEMP_CURRENT, &temperature);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get HBM2 temperature for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->temperature.hbm_temperature[2] = (float)temperature;
    }
    // fill the HBM3 temperature
    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle,
                                        AMDSMI_TEMPERATURE_TYPE_HBM_3,
                                        AMDSMI_TEMP_CURRENT, &temperature);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get HBM3 temperature for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        stats->temperature.hbm_temperature[3] = (float)temperature;
    }
    // fill ECC block stats
    smi_fill_ecc_stats_(gpu_handle, stats);
    return SDK_RET_OK;
}

sdk_ret_t
smi_event_read_all (aga_event_read_cb_t cb, void *ctxt)
{
    return SDK_RET_INVALID_OP;
}

sdk_ret_t
smi_gpu_reset (aga_gpu_handle_t gpu_handle,
               aga_gpu_reset_type_t reset_type)
{
    return SDK_RET_INVALID_OP;
}

static sdk_ret_t
smi_gpu_power_cap_update_ (aga_gpu_handle_t gpu_handle,
                           aga_gpu_spec_t *spec)
{
    amdsmi_status_t amdsmi_ret;
    amdsmi_power_cap_info_t power_cap_info;

    // 1. get power cap range
    // 2. validate the power cap is within the range
    // 3. set power cap
    // NOTE: power cap 0 indicates reset to default

    // step1: get power cap range
    amdsmi_ret = amdsmi_get_power_cap_info(gpu_handle, 0, &power_cap_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get power cap, GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
        return (amdsmi_ret_to_sdk_ret(amdsmi_ret));
    }
    // step2: validate power cap
    power_cap_info.min_power_cap /= 1000000;
    power_cap_info.max_power_cap /= 1000000;
    if ((spec->gpu_power_cap[0].power_cap < power_cap_info.min_power_cap) ||
        (spec->gpu_power_cap[0].power_cap > power_cap_info.max_power_cap)) {
        AGA_TRACE_ERR("Power cap {} is out of supported range, GPU {}, "
                      "allowed range {}-{}", spec->gpu_power_cap[0].power_cap,
                      gpu_handle, power_cap_info.min_power_cap,
                      power_cap_info.max_power_cap);
        return sdk_ret_t(SDK_RET_INVALID_ARG,
                         ERR_CODE_SMI_GPU_POWER_CAP_OUT_OF_RANGE);
    }
    // step3: set power cap
    amdsmi_ret = amdsmi_set_power_cap(gpu_handle, 0,
                     (spec->gpu_power_cap[0].power_cap * 1000000));
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to set power cap, GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
        return (amdsmi_ret_to_sdk_ret(amdsmi_ret));
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_update (aga_gpu_handle_t gpu_handle, aga_gpu_spec_t *spec,
                uint64_t upd_mask)
{
    sdk_ret_t ret;
    std::ofstream of;
    std::string dev_path;
    amdsmi_status_t amdsmi_ret;

    // performance level has to be set to manual (default is auto) to configure
    // the following list of attributes to non default values
    // 1. GPU overdrive level
    // 2. memory overdirve level

    // set memory partition type; we return after this operation as it doesn't
    // make sense to update other fields along with memory partition type
    if (upd_mask & AGA_GPU_UPD_MEMORY_PARTITION_TYPE) {
        amdsmi_ret = amdsmi_set_gpu_memory_partition_mode(gpu_handle,
                         aga_to_smi_gpu_memory_partition_type(
                             spec->memory_partition_type));
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to set GPU memory partition type to {}, "
                          "GPU {}, err {}", spec->memory_partition_type,
                          gpu_handle, amdsmi_ret);
        }
        return (amdsmi_ret_to_sdk_ret(amdsmi_ret));
    }
    // power cap update
    if (upd_mask & AGA_GPU_UPD_POWER_CAP) {
        ret = smi_gpu_power_cap_update_(gpu_handle, spec);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    return SDK_RET_OK;
}

/// \brief  callback function to be used to fill topology information between
///         two GPUS
/// \param[in]  obj     GPU object returned by walk function
/// \param[in]  ctxt    opaque context passed to the callback function
/// \return false in case walk should continue or true otherwise
static inline bool
gpu_topo_walk_cb (void *obj, void *ctxt)
{
    gpu_entry *gpu1, *gpu2;
    amdsmi_status_t amdsmi_ret;
    static std::string name = "GPU";
    gpu_topo_walk_ctxt_t *walk_ctxt;
    aga_device_topology_info_t *info;
    amdsmi_link_topology_t topo = {};

    gpu2 = (gpu_entry *)obj;
    walk_ctxt = (gpu_topo_walk_ctxt_t *)ctxt;
    gpu1 = walk_ctxt->gpu;
    info = walk_ctxt->info;

    if (gpu1->handle() != gpu2->handle()) {
        info->peer_device[walk_ctxt->count].peer_device.type =
            AGA_DEVICE_TYPE_GPU;
        strcpy(info->peer_device[walk_ctxt->count].peer_device.name,
               (name + std::to_string(gpu1->id())).c_str());
        amdsmi_ret =
            amdsmi_get_link_topology(gpu1->handle(), gpu2->handle(), &topo);
        if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to get link topology between gpus {} and {}, "
                          "err {}", gpu1->handle(), gpu2->handle(), amdsmi_ret);
            // in case of error set num hops and link weight to 0xffff and IO
            // link type to none
            info->peer_device[walk_ctxt->count].num_hops = 0xffff;
            info->peer_device[walk_ctxt->count].connection.type =
                AGA_IO_LINK_TYPE_NONE;
            info->peer_device[walk_ctxt->count].link_weight = 0xffff;
        } else {
            info->peer_device[walk_ctxt->count].num_hops = topo.num_hops;
            info->peer_device[walk_ctxt->count].link_weight = topo.weight;
            switch (topo.link_type) {
            case AMDSMI_LINK_TYPE_XGMI:
                info->peer_device[walk_ctxt->count].connection.type =
                    AGA_IO_LINK_TYPE_XGMI;
                break;
            case AMDSMI_LINK_TYPE_PCIE:
                info->peer_device[walk_ctxt->count].connection.type =
                    AGA_IO_LINK_TYPE_PCIE;
                break;
            default:
                info->peer_device[walk_ctxt->count].connection.type =
                    AGA_IO_LINK_TYPE_NONE;
                break;
            }
        }
        info->peer_device[walk_ctxt->count].valid = true;
        walk_ctxt->count++;
    }
    return false;
}

sdk_ret_t
smi_gpu_fill_device_topology (aga_gpu_handle_t gpu_handle,
                              aga_device_topology_info_t *info)
{
    gpu_entry *gpu;
    gpu_topo_walk_ctxt_t ctxt;

    gpu = gpu_db()->find(gpu_handle);
    if (gpu == NULL) {
        AGA_TRACE_ERR("Failed to find GPU {}", gpu_handle);
        return SDK_RET_ENTRY_NOT_FOUND;
    }

    ctxt.count = 0;
    ctxt.info = info;
    ctxt.gpu = gpu;

    // walk gpu db and fill device topology
    gpu_db()->walk_handle_db(gpu_topo_walk_cb, &ctxt);
    return SDK_RET_OK;
}

/// \brief function to get parent aga_obj_key_t for a given GPU
/// \param[in]  gpu_handle  GPU handle
/// \param[out] key         aga_obj_key_t of the parent GPU
sdk_ret_t
smi_get_parent_gpu_uuid (aga_gpu_handle_t gpu_handle, aga_obj_key_t *parent_key)
{
    amdsmi_bdf_t pcie_bdf;
    amdsmi_status_t status;
    amdsmi_asic_info_t asic_info;

    // get device BDF
    status = amdsmi_get_gpu_device_bdf(gpu_handle, &pcie_bdf);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get PCIe BDF of GPU {}, err {}",
                      gpu_handle, status);
        return amdsmi_ret_to_sdk_ret(status);
    }
    // get GPU asic info
    status = amdsmi_get_gpu_asic_info(gpu_handle, &asic_info);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get ASIC information of GPU {}, err {}",
                      gpu_handle, status);
        return amdsmi_ret_to_sdk_ret(status);
    }
    // compute the parent GPU uuid
    parent_key->reset();
    memcpy(&parent_key->id[0], asic_info.asic_serial, 4);
    *(uint16_t *)(&parent_key->id[6]) = 0x1000;
    *(uint64_t *)(&parent_key->id[8]) = pcie_bdf.as_uint;

    return SDK_RET_OK;
}

/// \brief function to get aga_obj_key_t for a given GPU
/// \param[in]  gpu_handle  GPU handle
/// \param[out] key         aga_obj_key_t of the GPU
static sdk_ret_t
smi_gpu_uuid_get (aga_gpu_handle_t gpu_handle, aga_obj_key_t *key)
{
    amdsmi_status_t status;
    char uuid_rem[20];
    char uuid[AMDSMI_GPU_UUID_SIZE];
    uint32_t uuid_len = AMDSMI_GPU_UUID_SIZE;

    // get uuid from amdsmi
    status = amdsmi_get_gpu_device_uuid(gpu_handle, &uuid_len, uuid);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get uuid of GPU {}, err {}",
                      gpu_handle, status);
        return amdsmi_ret_to_sdk_ret(status);
    }
    // amdsmi returns a string containing the uuid of the GPU (ex:
    // 2eff74a1-0000-1000-80fe-9cea14a6b148); to derive the aga_obj_key_t from
    // it we scan the string and construct our aga_obj_key_t
    sscanf(uuid, "%x-%hx-%hx-%hx-%s", (uint32_t *)&key->id[0],
           (uint16_t *)&key->id[4], (uint16_t *)&key->id[6],
           (uint16_t *)&key->id[8], uuid_rem);
    *(uint32_t *)&key->id[0] = htonl(*(uint32_t *)&key->id[0]);
    *(uint16_t *)&key->id[4] = htons(*(uint16_t *)&key->id[4]);
    *(uint16_t *)&key->id[6] = htons(*(uint16_t *)&key->id[6]);
    *(uint16_t *)&key->id[8] = htons(*(uint16_t *)&key->id[8]);
    sscanf(uuid_rem, "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx", &key->id[10],
           &key->id[11], &key->id[12], &key->id[13], &key->id[14],
           &key->id[15]);
    return SDK_RET_OK;
}

static sdk_ret_t
smi_fill_gpu_profile_ (uint32_t id, aga_gpu_handle_t handle,
                       aga_gpu_profile_t& gpu)
{
    sdk_ret_t ret;
    amdsmi_status_t status;
    amdsmi_accelerator_partition_profile_t profile;
    amdsmi_memory_partition_config_t mem_part_config = {};
    uint32_t partition_id[AMDSMI_MAX_ACCELERATOR_PARTITIONS] = { 0 };

    gpu.id = id;
    gpu.handle = handle;
    // get GPU uuid
    ret = smi_gpu_uuid_get(gpu.handle, &gpu.key);
    if (ret != SDK_RET_OK) {
        AGA_TRACE_ERR("GPU discovery failed due to error in getting UUID of "
                      "GPU {}", gpu.handle);
        return ret;
    }
    // set partition information to default values
    gpu.partition_capable = false;
    gpu.partition_id = 0;
    gpu.compute_partition = AGA_GPU_COMPUTE_PARTITION_TYPE_NONE;
    gpu.memory_partition = AGA_GPU_MEMORY_PARTITION_TYPE_NONE;
    // get accelerator partition profile
    status = amdsmi_get_gpu_accelerator_partition_profile(gpu.handle, &profile,
                                                          partition_id);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get accelerator partition config for GPU {}, "
                      "err {}", gpu.key.str(), status);
    } else {
        gpu.compute_partition =
            smi_to_aga_gpu_compute_partition_type(profile.profile_type);
        gpu.partition_id = partition_id[0];
        if (profile.num_partitions == std::numeric_limits<uint32_t>::max()) {
            // num partitions is set to the max uint32_t value for GPUs which
            // don't support partitions
            gpu.partition_capable = false;
        } else {
            gpu.partition_capable = true;
        }
    }
    // set memory partition type
    status = amdsmi_get_gpu_memory_partition_config(gpu.handle,
                                                    &mem_part_config);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get memory partition config for GPU {}, "
                      "err {}", gpu.key.str(), status);
    } else {
        gpu.memory_partition =
            smi_to_aga_gpu_memory_partition_type(mem_part_config.mp_mode);
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_discover_gpus (uint32_t *num_gpu, aga_gpu_profile_t *gpu)
{
    sdk_ret_t ret;
    uint32_t num_procs;
    amdsmi_status_t status;
    aga_gpu_handle_t proc_handles[AGA_MAX_GPU];

    if (!num_gpu) {
        return SDK_RET_ERR;
    }
    *num_gpu = 0;
    // for each socket get the number of processors
    status = amdsmi_get_processor_handles(NULL, &num_procs, NULL);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get number of processors, err {}", status);
        return amdsmi_ret_to_sdk_ret(status);
    }
    // for each socket get the processor handles
    status = amdsmi_get_processor_handles(NULL, &num_procs, &proc_handles[0]);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get processor handles from amd smi library, "
                      "err {}", status);
        return amdsmi_ret_to_sdk_ret(status);
    }
    // get uuids of each GPU
    for (uint32_t j = 0; j < num_procs; j++) {
        ret = smi_fill_gpu_profile_(*num_gpu, proc_handles[j],
                                    gpu[*num_gpu]);
        if (ret != SDK_RET_OK) {
            AGA_TRACE_ERR("GPU discovery failed when processing GPU {}",
                          proc_handles[j]);
            return ret;
        }
        (*num_gpu)++;
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_gpu_init_immutable_attrs (aga_gpu_handle_t gpu_handle, aga_gpu_spec_t *spec,
                              aga_gpu_status_t *status)
{
    amdsmi_fw_info_t fw_info;
    amdsmi_status_t amdsmi_ret;
    amdsmi_vbios_info_t vbios_info;
    amdsmi_board_info_t board_info;
    amdsmi_driver_info_t driver_info;
    amdsmi_virtualization_mode_t mode;

    // fill immutable attributes in spec
    // fill gpu and memory clock frequencies
    smi_fill_gpu_clock_frequency_spec_(gpu_handle, spec);

    // fill immutable attributes in status
    // fill the GPU serial number
    amdsmi_ret = amdsmi_get_gpu_board_info(gpu_handle, &board_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
       AGA_TRACE_ERR("Failed to get serial number for GPU {}, err {}",
                     gpu_handle, amdsmi_ret);
    }

    // fill the virtualization mode
    amdsmi_ret = amdsmi_get_gpu_virtualization_mode(gpu_handle, &mode);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get virtualization mode for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        status->virtualization_mode = smi_to_aga_virtualization_mode(mode);
    }

    memcpy(status->serial_num, board_info.product_serial, AGA_MAX_STR_LEN);
    // fill the GPU card series
    memcpy(status->card_series, board_info.product_name, AGA_MAX_STR_LEN);
    // fill the GPU vendor information
    memcpy(status->card_vendor, board_info.manufacturer_name, AGA_MAX_STR_LEN);
    // fill the GPU card model
    memcpy(status->card_model, board_info.model_number, AGA_MAX_STR_LEN);
    // fill the driver version
    amdsmi_ret = amdsmi_get_gpu_driver_info(gpu_handle, &driver_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get system driver information, GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    }
    memcpy(status->driver_version, driver_info.driver_version, AGA_MAX_STR_LEN);

    // fill the vbios version
    amdsmi_ret = amdsmi_get_gpu_vbios_info(gpu_handle, &vbios_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get vbios version for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        strncpy(status->vbios_version, vbios_info.version, AGA_MAX_STR_LEN);
        strncpy(status->vbios_part_number, vbios_info.part_number,
                AGA_MAX_STR_LEN);
        // sku should be retrieved from vbios version
        gpu_get_sku_from_vbios_(status->card_sku, vbios_info.part_number);
    }
    // fill the firmware version
    amdsmi_ret = amdsmi_get_fw_info(gpu_handle, &fw_info);
    if (unlikely(amdsmi_ret != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get firmware version for GPU {}, err {}",
                      gpu_handle, amdsmi_ret);
    } else {
        memset(status->fw_version, 0,
               sizeof(aga_gpu_fw_version_t) * AGA_GPU_MAX_FIRMWARE_VERSION);
        for (uint32_t i = 0; i < fw_info.num_fw_info; i++) {
            fill_gpu_fw_version_(&status->fw_version[i],
                                 fw_info.fw_info_list[i].fw_id,
                                 fw_info.fw_info_list[i].fw_version);
        }
        status->num_fw_versions = fw_info.num_fw_info;
    }
    // fill VRAM status
    smi_fill_vram_status_(gpu_handle, status);
    return SDK_RET_OK;
}

static inline std::string
timestamp_string_from_cper_timestamp (amdsmi_cper_timestamp_t *ts)
{
    uint32_t full_year;
    std::ostringstream oss;

    // assuming year is offset from 2000
    full_year = 2000 + ts->year;

    oss << std::setfill('0') << std::setw(4) << full_year << "-"
        << std::setw(2) << static_cast<int>(ts->month) << "-"
        << std::setw(2) << static_cast<int>(ts->day) << " "
        << std::setw(2) << static_cast<int>(ts->hours) << ":"
        << std::setw(2) << static_cast<int>(ts->minutes) << ":"
        << std::setw(2) << static_cast<int>(ts->seconds);

    return oss.str();
}

sdk_ret_t
smi_gpu_get_cper_entries (aga_gpu_handle_t gpu_handle,
                          aga_cper_severity_t severity, aga_cper_info_t *info)
{
    char *cper_data;
    char *cper_buffer;
    uint64_t cursor = 0;
    uint32_t severity_mask;
    amdsmi_status_t afid_status;
    uint64_t total_cper_entries = 0;
    uint64_t buf_size = CPER_BUF_SIZE;
    uint32_t prev_cper_record_size = 0;
    uint64_t num_cper_hdr = AGA_GPU_MAX_CPER_ENTRY;
    amdsmi_status_t status = AMDSMI_STATUS_MORE_DATA;
    amdsmi_cper_hdr_t *cper_hdrs[AGA_GPU_MAX_CPER_ENTRY];

    // set severity mask
    switch (severity) {
    case AGA_CPER_SEVERITY_NON_FATAL_UNCORRECTED:
        severity_mask = (1 << AMDSMI_CPER_SEV_NON_FATAL_UNCORRECTED);
        break;
    case AGA_CPER_SEVERITY_FATAL:
        severity_mask = (1 << AMDSMI_CPER_SEV_FATAL);
        break;
    case AGA_CPER_SEVERITY_NON_FATAL_CORRECTED:
        severity_mask = (1 << AMDSMI_CPER_SEV_NON_FATAL_CORRECTED);
        break;
    default:
        severity_mask = (1 << AMDSMI_CPER_SEV_NON_FATAL_UNCORRECTED) |
                        (1 << AMDSMI_CPER_SEV_FATAL)                 |
                        (1 << AMDSMI_CPER_SEV_NON_FATAL_CORRECTED);
        break;
    }
    // allocate memory for CPER data
    cper_data = (char *)malloc(buf_size);
    // cper_buffer is used to keep track of each individual record
    cper_buffer = cper_data;
    while (status == AMDSMI_STATUS_MORE_DATA) {
        // get CPER entries
        status = amdsmi_get_gpu_cper_entries(gpu_handle, severity_mask,
                     cper_data, &buf_size, cper_hdrs, &num_cper_hdr, &cursor);
        if ((status != AMDSMI_STATUS_SUCCESS) &&
            (status != AMDSMI_STATUS_MORE_DATA)) {
            AGA_TRACE_ERR("Failed to get CPER entries for GPU {}, err {}",
                          gpu_handle, status);
            // free allocated memory
            free(cper_data);
            return amdsmi_ret_to_sdk_ret(status);
        }
        for (uint64_t i = 0;
             i < num_cper_hdr && total_cper_entries < AGA_GPU_MAX_CPER_ENTRY ;
             i++, total_cper_entries++) {
            auto cper_entry = &info->cper_entry[info->num_cper_entry++];

            cper_entry->record_id = std::string(cper_hdrs[i]->record_id);
            cper_entry->severity =
                smi_to_aga_cper_severity(cper_hdrs[i]->error_severity);
            cper_entry->revision = cper_hdrs[i]->revision;
            if (cper_hdrs[i]->cper_valid_bits.valid_bits.timestamp) {
                cper_entry->timestamp =
					timestamp_string_from_cper_timestamp(
						&cper_hdrs[i]->timestamp);
            }
            cper_entry->creator_id = std::string(cper_hdrs[i]->creator_id);
            cper_entry->notification_type =
                smi_to_aga_cper_notification_type(cper_hdrs[i]->notify_type);
            // get AMD field ids from the cper record
            cper_buffer += prev_cper_record_size;
            // initialize num_af_id to be the size of the array
            cper_entry->num_af_id = AGA_GPU_MAX_AF_ID_PER_CPER;
            afid_status = amdsmi_get_afids_from_cper(cper_buffer,
                              cper_hdrs[i]->record_length, cper_entry->af_id,
                              &cper_entry->num_af_id);
            if (afid_status != AMDSMI_STATUS_SUCCESS) {
                cper_entry->num_af_id = 0;
                AGA_TRACE_ERR("Failed to get AMD field id for CPER entry for "
                              "GPU {}, err {}", gpu_handle, status);
            }
            // update prev_cper_record_size
            prev_cper_record_size = cper_hdrs[i]->record_length;
        }
    }

    // free allocated memory
    free(cper_data);
    return SDK_RET_OK;
}

}    // namespace aga
