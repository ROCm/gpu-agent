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
/// smi library state implementation
///
//----------------------------------------------------------------------------

#include <vector>
#include <memory>
#include <sys/stat.h>
extern "C" {
#include "nic/third-party/rocm/wsl_amd_smi_lib/include/amd_smi/amdsmi.h"
}
#include "nic/gpuagent/core/trace.hpp"
#include "nic/gpuagent/core/aga_core.hpp"
#include "nic/gpuagent/core/ipc_msg.hpp"
#include "nic/gpuagent/api/aga_state.hpp"
#include "nic/gpuagent/api/smi/smi.hpp"
#include "nic/gpuagent/api/smi/smi_state.hpp"
#include "nic/gpuagent/api/smi/smi_watch.hpp"
#include "nic/gpuagent/api/smi/wslamdsmi/smi_utils.hpp"

using std::vector;

/// \defgroup AGA_SMI_STATE global state for smi interactions
/// @{

namespace event = sdk::event_thread;

/// initial delay after which event monitoring starts (in seconds)
#define AGA_SMI_EVENT_MONITOR_START_DELAY    10.0
/// event monitoring frequency (in seconds)
#define AGA_SMI_EVENT_MONITOR_INTERVAL       3.0
/// all amdsmi events of interest
#define AMDSMI_EVENT_MASK_ALL                  \
            ((1 << AMDSMI_EVT_NOTIF_VMFAULT)          |    \
             (1 << AMDSMI_EVT_NOTIF_THERMAL_THROTTLE) |    \
             (1 << AMDSMI_EVT_NOTIF_GPU_PRE_RESET)    |    \
             (1 << AMDSMI_EVT_NOTIF_GPU_POST_RESET))
/// timeout to wait to gather outstanding events (in milliseconds)
#define AMDSMI_EVENT_NTFN_TIMEOUT              0

/// initial delay after which watch field update starts
#define AGA_WATCHER_START_DELAY            10.0
/// watch field frequency (in seconds)
#define AGA_WATCHER_INTERVAL               1.0
/// watcher gpu group name
#define AGA_WATCHER_GPU_GROUP_NAME         "AGA_GPU_GROUP"
/// watcher field group name
#define AGA_WATCHER_FIELD_GROUP_NAME       "AGA_FIELD_GROUP"
/// update frequency of the watch fields in auto mode (micro seconds)
#define AGA_WATCHER_UPDATE_FREQUENCY_IN_MS 1000000
/// max age time in seconds for a field value after update
#define AGA_WATCHER_MAX_KEEP_AGE           60
/// max samples of a field value
#define AGA_WATCHER_MAX_KEEP_SAMPLES       10

namespace aga {

/// energy counter resolution (uJ/count), defined in smi_api.cc (namespace aga);
/// the watcher reuses it to scale energy_accumulator like GPUGet
extern float g_energy_counter_resolution;

/// global singleton smi state class instance
smi_state g_smi_state;

sdk_ret_t
smi_state::smi_watcher_update_all_watch_fields_(uint32_t gpu_id,
               amdsmi_processor_handle gpu_handle,
               aga_gpu_watch_db_t *watch_db, bool refresh_slow) {
    double time_sec;
    uint64_t mem64 = 0;
    uint64_t counter_key;
    int64_t int64_val = 0;
    amdsmi_error_count_t ec;
    uint64_t uint64_val = 0;
    uint64_t used_vram_mb = 0;
    amdsmi_clk_type_t clk_type;
    amdsmi_status_t amdsmi_ret;
    bool power_from_bulk = false;
    bool bulk_get_succeeded = false;
    bool ecc_read_succeeded = false;
    uint64_t pcie_tx = 0, pcie_rx = 0;
    uint64_t total_deferred_count = 0;
    bool violation_get_succeeded = false;
    amdsmi_pcie_info_t pcie_info = { 0 };
    uint64_t total_correctable_count = 0;
    amdsmi_temperature_type_t sensor_type;
    uint64_t total_uncorrectable_count = 0;
    amdsmi_frequencies_t freq_info = { 0 };
    amdsmi_power_info_t power_info = { 0 };
    amdsmi_engine_usage_t usage_info = { 0 };
    amdsmi_gpu_metrics_t gpu_metrics = { 0 };
    amdsmi_counter_value_t counter_value = { 0 };
    amdsmi_violation_status_t violation_status = {};

    // seed slow tier fields (ECC/XGMI/violation counters, clocks, memory) from
    // the last slow sample so they carry forward on ticks that skip the slow
    // reads, then reset fast tier fields to NA so a failed read this tick is
    // reported not-applicable rather than as a stale carried-forward value
    watch_db->watch_info[gpu_id] = slow_cache_[gpu_id];
    aga_gpu_watch_reset_fast_fields(&watch_db->watch_info[gpu_id]);

    // get GPU metrics, which can be used to bulk fill a few fields
    amdsmi_ret = amdsmi_get_gpu_metrics_info(gpu_handle, &gpu_metrics);
    if (amdsmi_ret == AMDSMI_STATUS_NOT_SUPPORTED) {
        // PMLog escape unsupported on this GPU/driver (common on WSL2);
        // synthesize from individual amdsmi calls.
        smi_wsl_fill_metrics_from_individual_(gpu_handle, &gpu_metrics);
        amdsmi_ret = AMDSMI_STATUS_SUCCESS;
    } else if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
        // librocdxg gpu_metrics_info may not populate all fields (e.g.
        // temperature); fill any zeros from the individual amdsmi calls.
        smi_wsl_fill_metrics_from_individual_(gpu_handle, &gpu_metrics);
    }
    if (amdsmi_ret == AMDSMI_STATUS_SUCCESS &&
            gpu_metrics.common_header.structure_size != 0) {
        // mark bulk get as succeeded only when the struct was actually populated
        bulk_get_succeeded = true;
    }

    // get correctable and uncorrectable total error count beforehand
    // ECC is slow tier, so the per block reads are skipped entirely on fast
    // only ticks and the seeded values stand
    for (uint64_t b = AMDSMI_GPU_BLOCK_FIRST;
         refresh_slow && (b <= AMDSMI_GPU_BLOCK_LAST); b = b * 2) {
        // initialize ec to all 0s
        ec = { 0 };
        amdsmi_ret = amdsmi_get_gpu_ecc_count(gpu_handle,
                                              (amdsmi_gpu_block_t)(b), &ec);
        if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
            ecc_read_succeeded = true;
            total_correctable_count += ec.correctable_count;
            total_uncorrectable_count += ec.uncorrectable_count;
            total_deferred_count += ec.deferred_count;
            switch (b) {
            case AMDSMI_GPU_BLOCK_UMC:
                watch_db->watch_info[gpu_id].umc_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].umc_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].umc_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_SDMA:
                watch_db->watch_info[gpu_id].sdma_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].sdma_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].sdma_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_GFX:
                watch_db->watch_info[gpu_id].gfx_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].gfx_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].gfx_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_MMHUB:
                watch_db->watch_info[gpu_id].mmhub_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].mmhub_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].mmhub_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_ATHUB:
                watch_db->watch_info[gpu_id].athub_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].athub_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].athub_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_PCIE_BIF:
                watch_db->watch_info[gpu_id].bif_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].bif_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].bif_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_HDP:
                watch_db->watch_info[gpu_id].hdp_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].hdp_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].hdp_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_XGMI_WAFL:
                watch_db->watch_info[gpu_id].xgmi_wafl_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].xgmi_wafl_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].xgmi_wafl_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_DF:
                watch_db->watch_info[gpu_id].df_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].df_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].df_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_SMN:
                watch_db->watch_info[gpu_id].smn_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].smn_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].smn_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_SEM:
                watch_db->watch_info[gpu_id].sem_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].sem_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].sem_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_MP0:
                watch_db->watch_info[gpu_id].mp0_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].mp0_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].mp0_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_MP1:
                watch_db->watch_info[gpu_id].mp1_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].mp1_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].mp1_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_FUSE:
                watch_db->watch_info[gpu_id].fuse_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].fuse_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].fuse_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_MCA:
                watch_db->watch_info[gpu_id].mca_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].mca_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].mca_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_VCN:
                watch_db->watch_info[gpu_id].vcn_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].vcn_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].vcn_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_JPEG:
                watch_db->watch_info[gpu_id].jpeg_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].jpeg_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].jpeg_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_IH:
                watch_db->watch_info[gpu_id].ih_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].ih_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].ih_deferred_errors =
                    ec.deferred_count;
                break;
            case AMDSMI_GPU_BLOCK_MPIO:
                watch_db->watch_info[gpu_id].mpio_correctable_errors =
                    ec.correctable_count;
                watch_db->watch_info[gpu_id].mpio_uncorrectable_errors =
                    ec.uncorrectable_count;
                watch_db->watch_info[gpu_id].mpio_deferred_errors =
                    ec.deferred_count;
                break;
            default:
                break;
            }
        }
    }

    // violations are slow tier; fetch the status once per slow refresh so all
    // violation attrs below fill from a single call rather than one call each
    if (refresh_slow) {
        amdsmi_ret = amdsmi_get_violation_status(gpu_handle, &violation_status);
        if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
            violation_get_succeeded = true;
        }
    }

    // memory totals/used/free and PCIe link speed are dedicated amdsmi calls;
    // fetch once per slow refresh
    if (refresh_slow) {
        if (amdsmi_get_gpu_memory_total(gpu_handle, AMDSMI_MEM_TYPE_VRAM,
                                        &mem64) == AMDSMI_STATUS_SUCCESS) {
            watch_db->watch_info[gpu_id].total_vram = mem64 / 1024 / 1024;
        }
        if (amdsmi_get_gpu_memory_total(gpu_handle, AMDSMI_MEM_TYPE_VIS_VRAM,
                                        &mem64) == AMDSMI_STATUS_SUCCESS) {
            watch_db->watch_info[gpu_id].total_visible_vram = mem64 / 1024 / 1024;
        }
        if (amdsmi_get_gpu_memory_total(gpu_handle, AMDSMI_MEM_TYPE_GTT,
                                        &mem64) == AMDSMI_STATUS_SUCCESS) {
            watch_db->watch_info[gpu_id].total_gtt = mem64 / 1024 / 1024;
        }
        if (amdsmi_get_gpu_memory_usage(gpu_handle, AMDSMI_MEM_TYPE_VRAM,
                                        &mem64) == AMDSMI_STATUS_SUCCESS) {
            used_vram_mb = mem64 / 1024 / 1024;
        }
        if (amdsmi_get_gpu_memory_usage(gpu_handle, AMDSMI_MEM_TYPE_VIS_VRAM,
                                        &mem64) == AMDSMI_STATUS_SUCCESS) {
            watch_db->watch_info[gpu_id].used_visible_vram = mem64 / 1024 / 1024;
        }
        if (amdsmi_get_gpu_memory_usage(gpu_handle, AMDSMI_MEM_TYPE_GTT,
                                        &mem64) == AMDSMI_STATUS_SUCCESS) {
            watch_db->watch_info[gpu_id].used_gtt = mem64 / 1024 / 1024;
        }
        watch_db->watch_info[gpu_id].free_vram =
            watch_db->watch_info[gpu_id].total_vram - used_vram_mb;
        watch_db->watch_info[gpu_id].free_visible_vram =
            watch_db->watch_info[gpu_id].total_visible_vram -
            watch_db->watch_info[gpu_id].used_visible_vram;
        watch_db->watch_info[gpu_id].free_gtt =
            watch_db->watch_info[gpu_id].total_gtt -
            watch_db->watch_info[gpu_id].used_gtt;
        pcie_info = { 0 };
        if (amdsmi_get_pcie_info(gpu_handle, &pcie_info) ==
                AMDSMI_STATUS_SUCCESS) {
            if (pcie_info.pcie_metric.pcie_speed != UINT32_MAX) {
                watch_db->watch_info[gpu_id].pcie_speed =
                    pcie_info.pcie_metric.pcie_speed / 1000;
            }
            if (pcie_info.pcie_static.max_pcie_speed != UINT32_MAX) {
                watch_db->watch_info[gpu_id].pcie_max_speed =
                    pcie_info.pcie_static.max_pcie_speed / 1000;
            }
        }
    }

    // clock status: assemble like the pull path (smi_fill_clock_status_), slow
    // tier. One slot per (clock_type, instance) in the same gfx/mem/video/data/
    // soc order; clock_index carries the global slot position so the exporter
    // clock_index label matches the pull path 1:1 (NA slots still consume an
    // index). Only slots with at least one applicable value are stored
    // (compacted), so the indexed-vals cap holds while indices stay sparse.
    // NA uses UINT64_MAX to match the exporter's uint64 applicability check.
    if (refresh_slow && bulk_get_succeeded) {
        uint32_t global_idx = 0, cc = 0;
        amdsmi_clk_info_t clk_info = {};
        amdsmi_frequencies_t soc_freq = {};
        auto store_clock = [&](uint32_t ctype, uint64_t cur, uint64_t lo,
                               uint64_t hi) {
            if (((cur != UINT64_MAX) || (lo != UINT64_MAX) ||
                 (hi != UINT64_MAX)) && (cc < AGA_GPU_WATCH_MAX_INDEXED_VALS)) {
                watch_db->watch_info[gpu_id].clock_index[cc] = global_idx;
                watch_db->watch_info[gpu_id].clock_type[cc] = ctype;
                watch_db->watch_info[gpu_id].clock_current[cc] = cur;
                watch_db->watch_info[gpu_id].clock_min[cc] = lo;
                watch_db->watch_info[gpu_id].clock_max[cc] = hi;
                cc++;
            }
            global_idx++;
        };
        auto clk_lo_hi = [&](amdsmi_clk_type_t t, uint64_t *lo, uint64_t *hi) {
            clk_info = {};
            *lo = *hi = UINT64_MAX;
            if (amdsmi_get_clock_info(gpu_handle, t, &clk_info) ==
                    AMDSMI_STATUS_SUCCESS) {
                *lo = clk_info.min_clk;
                *hi = clk_info.max_clk;
            }
        };
        uint64_t lo = UINT64_MAX, hi = UINT64_MAX;
        // gfx -> SYSTEM
        clk_lo_hi(AMDSMI_CLK_TYPE_GFX, &lo, &hi);
        for (uint32_t i = 0; i < AMDSMI_MAX_NUM_GFX_CLKS; i++) {
            store_clock(AGA_GPU_CLOCK_TYPE_SYSTEM,
                AGA_WIDEN_UINT16_NA(gpu_metrics.current_gfxclks[i], UINT64_MAX),
                lo, hi);
        }
        // memory -> MEMORY (single instance)
        clk_lo_hi(AMDSMI_CLK_TYPE_MEM, &lo, &hi);
        store_clock(AGA_GPU_CLOCK_TYPE_MEMORY,
            AGA_WIDEN_UINT16_NA(gpu_metrics.current_uclk, UINT64_MAX), lo, hi);
        // video -> VIDEO
        clk_lo_hi(AMDSMI_CLK_TYPE_VCLK0, &lo, &hi);
        for (uint32_t i = 0; i < AMDSMI_MAX_NUM_CLKS; i++) {
            store_clock(AGA_GPU_CLOCK_TYPE_VIDEO,
                AGA_WIDEN_UINT16_NA(gpu_metrics.current_vclk0s[i], UINT64_MAX),
                lo, hi);
        }
        // data -> DATA
        clk_lo_hi(AMDSMI_CLK_TYPE_DCLK0, &lo, &hi);
        for (uint32_t i = 0; i < AMDSMI_MAX_NUM_CLKS; i++) {
            store_clock(AGA_GPU_CLOCK_TYPE_DATA,
                AGA_WIDEN_UINT16_NA(gpu_metrics.current_dclk0s[i], UINT64_MAX),
                lo, hi);
        }
        // soc -> SOC, only when the freq read succeeds (matches the pull path,
        // which does not advance the clock index for SOC on failure)
        if (amdsmi_get_clk_freq(gpu_handle, AMDSMI_CLK_TYPE_SOC, &soc_freq) ==
                AMDSMI_STATUS_SUCCESS) {
            uint32_t slo = 0, shi = 0;
            find_low_high_frequency(&soc_freq, &slo, &shi);
            for (uint32_t i = 0; i < AMDSMI_MAX_NUM_CLKS; i++) {
                store_clock(AGA_GPU_CLOCK_TYPE_SOC,
                    AGA_WIDEN_UINT16_NA(gpu_metrics.current_socclks[i],
                                        UINT64_MAX), slo, shi);
            }
        }
        // fabric (DF), DCE, PCIe clocks: single instance each, current from
        // the freq table, added only when the current freq is applicable (the
        // pull path advances the clock index only on add for these types)
        struct { amdsmi_clk_type_t t; uint32_t ctype; } tail_clocks[] = {
            { AMDSMI_CLK_TYPE_DF,   AGA_GPU_CLOCK_TYPE_FABRIC },
            { AMDSMI_CLK_TYPE_DCEF, AGA_GPU_CLOCK_TYPE_DCE },
            { AMDSMI_CLK_TYPE_PCIE, AGA_GPU_CLOCK_TYPE_PCIE },
        };
        for (auto &tc : tail_clocks) {
            amdsmi_frequencies_t tf = {};
            if (amdsmi_get_clk_freq(gpu_handle, tc.t, &tf) !=
                    AMDSMI_STATUS_SUCCESS) {
                continue;
            }
            uint64_t cur = current_frequency_hz(&tf);
            if (cur == UINT32_MAX) {
                continue;
            }
            uint32_t tlo = 0, thi = 0;
            find_low_high_frequency(&tf, &tlo, &thi);
            store_clock(tc.ctype, cur / 1000000, tlo, thi);
        }
        watch_db->watch_info[gpu_id].clock_count = cc;
    }

    // loop through all watch fields
    for (uint32_t i = 0; i < g_watch_field_list.size(); i++) {
        // on fast only ticks the slow tier attrs keep their seeded values;
        // falling through would clobber them with this tick's zeroed locals
        if (!refresh_slow &&
            (g_watch_field_list[i].tier == GPU_WATCH_TIER_SLOW)) {
            continue;
        }
        switch (g_watch_field_list[i].id) {
        case AGA_GPU_WATCH_ATTR_ID_GPU_CLOCK:
            if (bulk_get_succeeded) {
                // GPU clock frequency in MHz
                watch_db->watch_info[gpu_id].gpu_clock =
                    AGA_WIDEN_UINT16_NA(gpu_metrics.current_gfxclk,
                                        UINT64_MAX);
            } else {
                clk_type = AMDSMI_CLK_TYPE_SYS;
                // get clock frequency
                amdsmi_ret = amdsmi_get_clk_freq(gpu_handle, clk_type, &freq_info);
                if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                    watch_db->watch_info[gpu_id].gpu_clock =
                        current_frequency_hz(&freq_info) / 1000000;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_MEM_CLOCK:
            // Prefer the one bulk gpu_metrics read (current_uclk, already MHz) over
            // a per-attr amdsmi_get_clk_freq. At 64 partitions under load the extra
            // per-attr amdsmi calls pile up and destabilize amdsmi; the discrete
            // read is only a fallback for a failed/NA bulk read. Same path for SPX
            // and CPX (the bulk read is per gpu_handle).
            if (bulk_get_succeeded && gpu_metrics.current_uclk != UINT16_MAX) {
                watch_db->watch_info[gpu_id].memory_clock =
                    gpu_metrics.current_uclk;
            } else {
                clk_type = AMDSMI_CLK_TYPE_MEM;
                amdsmi_ret = amdsmi_get_clk_freq(gpu_handle, clk_type, &freq_info);
                if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                    watch_db->watch_info[gpu_id].memory_clock =
                        current_frequency_hz(&freq_info) / 1000000;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_MEMORY_TEMP:
            if (bulk_get_succeeded) {
                // GPU memory temperature in celsius
                watch_db->watch_info[gpu_id].memory_temperature =
                    AGA_WIDEN_UINT16_NA(gpu_metrics.temperature_mem,
                                        UINT64_MAX);
            } else {
                sensor_type = AMDSMI_TEMPERATURE_TYPE_VRAM;
                // get GPU memory temperature
                amdsmi_ret = amdsmi_get_temp_metric(gpu_handle, sensor_type,
                                 AMDSMI_TEMP_CURRENT, &int64_val);
                if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                    watch_db->watch_info[gpu_id].memory_temperature = int64_val;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_GPU_TEMP:
            // Prefer the one bulk gpu_metrics read (temperature_edge, same field
            // the GpuGet fill_stats path uses) over a per-attr amdsmi_get_temp_metric:
            // at 64 partitions under load the extra per-attr amdsmi calls pile up and
            // destabilize amdsmi. Only fall back to the discrete edge→junction read
            // when the bulk read failed or edge is unsupported (0/NA). Same path for
            // SPX and CPX (the bulk read is per gpu_handle).
            if (bulk_get_succeeded && gpu_metrics.temperature_edge != 0 &&
                gpu_metrics.temperature_edge != UINT16_MAX) {
                watch_db->watch_info[gpu_id].gpu_temperature =
                    gpu_metrics.temperature_edge;
            } else {
                sensor_type = AMDSMI_TEMPERATURE_TYPE_EDGE;
                amdsmi_ret = amdsmi_get_temp_metric(gpu_handle, sensor_type,
                                 AMDSMI_TEMP_CURRENT, &int64_val);
                if (amdsmi_ret == AMDSMI_STATUS_NOT_SUPPORTED) {
                    // fallback to hotspot temperature as some card may not have
                    // edge temperature.
                    sensor_type = AMDSMI_TEMPERATURE_TYPE_JUNCTION;
                    amdsmi_ret = amdsmi_get_temp_metric(gpu_handle, sensor_type,
                                     AMDSMI_TEMP_CURRENT, &int64_val);
                }
                if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                    watch_db->watch_info[gpu_id].gpu_temperature = int64_val;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_POWER_USAGE:
            if (bulk_get_succeeded) {
                // GPU power usage
                if (gpu_metrics.average_socket_power == UINT16_MAX) {
                    watch_db->watch_info[gpu_id].power_usage =
                        AGA_WIDEN_UINT16_NA(gpu_metrics.current_socket_power,
                                            UINT64_MAX);
                } else {
                    watch_db->watch_info[gpu_id].power_usage =
                        gpu_metrics.average_socket_power;
                }
                power_from_bulk =
                    watch_db->watch_info[gpu_id].power_usage &&
                    (watch_db->watch_info[gpu_id].power_usage != UINT64_MAX);
            }
            // power usage was not read from GPU metrics; use other API to read
            // NOTE: tested via a local flag, not the destination field, which
            // may hold a carried forward value from the last slow refresh
            if (!power_from_bulk) {
                amdsmi_ret = amdsmi_get_power_info(gpu_handle, &power_info);
                if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                    if (power_info.average_socket_power != UINT16_MAX) {
                        watch_db->watch_info[gpu_id].power_usage =
                            power_info.average_socket_power;
                    } else {
                        watch_db->watch_info[gpu_id].power_usage =
                            AGA_WIDEN_UINT16_NA(power_info.current_socket_power,
                                                UINT64_MAX);
                    }
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_TX:
            watch_db->watch_info[gpu_id].pcie_tx_usage = pcie_tx;
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_RX:
            watch_db->watch_info[gpu_id].pcie_rx_usage = pcie_rx;
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_BANDWIDTH:
            // PCIe bandwidth needs a discrete amdsmi_get_pcie_info (not in the bulk
            // gpu_metrics struct, and its instantaneous Mb/s value differs from the
            // bulk's accumulated field). To keep the per-fast-tick amdsmi call count
            // low at 64 partitions (which otherwise destabilizes amdsmi), refresh it
            // on the slow tier only; the last value is carried forward between.
            if (refresh_slow) {
                amdsmi_ret = amdsmi_get_pcie_info(gpu_handle, &pcie_info);
                if (unlikely(amdsmi_ret == AMDSMI_STATUS_SUCCESS)) {
                    watch_db->watch_info[gpu_id].pcie_bandwidth =
                        AGA_WIDEN_UINT32_NA(pcie_info.pcie_metric.pcie_bandwidth,
                                            UINT64_MAX);
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_GPU_UTIL:
            if (bulk_get_succeeded) {
                // GPU utilization
                watch_db->watch_info[gpu_id].gpu_util =
                    AGA_WIDEN_UINT16_NA(gpu_metrics.average_gfx_activity,
                                        UINT64_MAX);
            } else {
                amdsmi_ret = amdsmi_get_gpu_activity(gpu_handle, &usage_info);
                if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                    watch_db->watch_info[gpu_id].gpu_util =
                        usage_info.gfx_activity;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_UMC_ACTIVITY:
            if (bulk_get_succeeded) {
                watch_db->watch_info[gpu_id].umc_activity =
                    AGA_WIDEN_UINT16_NA(gpu_metrics.average_umc_activity,
                                        UINT64_MAX);
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_MM_ACTIVITY:
            if (bulk_get_succeeded) {
                watch_db->watch_info[gpu_id].mm_activity =
                    AGA_WIDEN_UINT16_NA(gpu_metrics.average_mm_activity,
                                        UINT64_MAX);
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_AVG_POWER:
            if (bulk_get_succeeded) {
                watch_db->watch_info[gpu_id].avg_power =
                    AGA_WIDEN_UINT16_NA(gpu_metrics.average_socket_power,
                                        UINT64_MAX);
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_GFX_VOLTAGE:
            if (bulk_get_succeeded) {
                watch_db->watch_info[gpu_id].gfx_voltage =
                    AGA_WIDEN_UINT16_NA(gpu_metrics.voltage_gfx, UINT64_MAX);
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_SOC_VOLTAGE:
            if (bulk_get_succeeded) {
                watch_db->watch_info[gpu_id].soc_voltage =
                    AGA_WIDEN_UINT16_NA(gpu_metrics.voltage_soc, UINT64_MAX);
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_MEM_VOLTAGE:
            if (bulk_get_succeeded) {
                watch_db->watch_info[gpu_id].mem_voltage =
                    AGA_WIDEN_UINT16_NA(gpu_metrics.voltage_mem, UINT64_MAX);
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_ENERGY_CONSUMED:
            // scale the raw accumulator by the counter resolution to uJ, mirroring
            // smi_gpu_fill_stats; NA accumulator maps to the uint64 NA sentinel so
            // the exporter drops it. Fall back to the default resolution if the
            // GPUGet status path has not initialized the global yet.
            if (bulk_get_succeeded) {
                if (gpu_metrics.energy_accumulator == UINT64_MAX) {
                    watch_db->watch_info[gpu_id].energy_consumed = UINT64_MAX;
                } else {
                    double energy_res = (g_energy_counter_resolution != 0.0) ?
                        g_energy_counter_resolution : 15.3;
                    watch_db->watch_info[gpu_id].energy_consumed = (uint64_t)
                        ((double)gpu_metrics.energy_accumulator * energy_res);
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_REPLAY_COUNT:
            if (bulk_get_succeeded) {
                watch_db->watch_info[gpu_id].pcie_replay_count =
                    gpu_metrics.pcie_replay_count_acc;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_RECOVERY_COUNT:
            if (bulk_get_succeeded) {
                watch_db->watch_info[gpu_id].pcie_recovery_count =
                    gpu_metrics.pcie_l0_to_recov_count_acc;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_REPLAY_ROLLOVER_COUNT:
            if (bulk_get_succeeded) {
                watch_db->watch_info[gpu_id].pcie_replay_rollover_count =
                    gpu_metrics.pcie_replay_rover_count_acc;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_NACK_SENT_COUNT:
            if (bulk_get_succeeded) {
                watch_db->watch_info[gpu_id].pcie_nack_sent_count =
                    gpu_metrics.pcie_nak_sent_count_acc;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_PCIE_NACK_RECEIVED_COUNT:
            if (bulk_get_succeeded) {
                watch_db->watch_info[gpu_id].pcie_nack_received_count =
                    gpu_metrics.pcie_nak_rcvd_count_acc;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_GPU_POWER_USAGE:
            if (bulk_get_succeeded) {
                watch_db->watch_info[gpu_id].gpu_power_usage =
                    AGA_WIDEN_UINT16_NA(gpu_metrics.current_socket_power,
                                        UINT64_MAX);
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VRAM_MAX_BANDWIDTH:
            if (bulk_get_succeeded) {
                watch_db->watch_info[gpu_id].vram_max_bandwidth =
                    gpu_metrics.vram_max_bandwidth;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_EDGE_TEMP:
            if (bulk_get_succeeded) {
                watch_db->watch_info[gpu_id].edge_temperature =
                    AGA_WIDEN_UINT16_NA(gpu_metrics.temperature_edge,
                                        UINT64_MAX);
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE:
            // VRAM usage needs a discrete, partition-scoped amdsmi_get_gpu_memory_usage
            // (the bulk gpu_metrics vram field is unreliable, which is why the GpuGet
            // fill path also reads it discretely). Refresh it on the slow tier only to
            // keep the per-fast-tick amdsmi call count low at 64 partitions; the last
            // value is carried forward between slow ticks.
            if (refresh_slow) {
                amdsmi_ret = amdsmi_get_gpu_memory_usage(gpu_handle,
                                                         AMDSMI_MEM_TYPE_VRAM,
                                                         &uint64_val);
                if (amdsmi_ret == AMDSMI_STATUS_SUCCESS) {
                    // convert GPU memory usage from bytes to MB
                    watch_db->watch_info[gpu_id].gpu_memory_usage =
                        uint64_val/1024/1024;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_HBM_TEMP:
            if (bulk_get_succeeded) {
                uint32_t n = sizeof(gpu_metrics.temperature_hbm) /
                             sizeof(gpu_metrics.temperature_hbm[0]);
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].hbm_temperature[i] =
                        AGA_WIDEN_UINT16_NA(gpu_metrics.temperature_hbm[i],
                                            UINT64_MAX);
                }
                watch_db->watch_info[gpu_id].hbm_temperature_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VCN_ACTIVITY:
            if (bulk_get_succeeded) {
                uint32_t n = sizeof(gpu_metrics.vcn_activity) /
                             sizeof(gpu_metrics.vcn_activity[0]);
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].vcn_activity[i] =
                        AGA_WIDEN_UINT16_NA(gpu_metrics.vcn_activity[i],
                                            UINT64_MAX);
                }
                watch_db->watch_info[gpu_id].vcn_activity_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_JPEG_ACTIVITY:
            if (bulk_get_succeeded) {
                uint32_t n = sizeof(gpu_metrics.jpeg_activity) /
                             sizeof(gpu_metrics.jpeg_activity[0]);
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].jpeg_activity[i] =
                        AGA_WIDEN_UINT16_NA(gpu_metrics.jpeg_activity[i],
                                            UINT64_MAX);
                }
                watch_db->watch_info[gpu_id].jpeg_activity_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_LINK_RX:
            if (bulk_get_succeeded) {
                uint32_t n = sizeof(gpu_metrics.xgmi_read_data_acc) /
                             sizeof(gpu_metrics.xgmi_read_data_acc[0]);
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].xgmi_link_rx[i] =
                        gpu_metrics.xgmi_read_data_acc[i];
                }
                watch_db->watch_info[gpu_id].xgmi_link_rx_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_LINK_TX:
            if (bulk_get_succeeded) {
                uint32_t n = sizeof(gpu_metrics.xgmi_write_data_acc) /
                             sizeof(gpu_metrics.xgmi_write_data_acc[0]);
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].xgmi_link_tx[i] =
                        gpu_metrics.xgmi_write_data_acc[i];
                }
                watch_db->watch_info[gpu_id].xgmi_link_tx_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_GFX_BUSY_INST:
            if (bulk_get_succeeded) {
                uint32_t n = sizeof(gpu_metrics.xcp_stats[0].gfx_busy_inst) /
                             sizeof(gpu_metrics.xcp_stats[0].gfx_busy_inst[0]);
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].gfx_busy_inst[i] =
                        AGA_WIDEN_UINT32_NA(gpu_metrics.xcp_stats[0].gfx_busy_inst[i],
                                            UINT64_MAX);
                }
                watch_db->watch_info[gpu_id].gfx_busy_inst_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VCN_BUSY_INST:
            if (bulk_get_succeeded) {
                uint32_t n = sizeof(gpu_metrics.xcp_stats[0].vcn_busy) /
                             sizeof(gpu_metrics.xcp_stats[0].vcn_busy[0]);
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].vcn_busy_inst[i] =
                        AGA_WIDEN_UINT16_NA(gpu_metrics.xcp_stats[0].vcn_busy[i],
                                            UINT64_MAX);
                }
                watch_db->watch_info[gpu_id].vcn_busy_inst_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_JPEG_BUSY_INST:
            if (bulk_get_succeeded) {
                uint32_t n = sizeof(gpu_metrics.xcp_stats[0].jpeg_busy) /
                             sizeof(gpu_metrics.xcp_stats[0].jpeg_busy[0]);
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].jpeg_busy_inst[i] =
                        AGA_WIDEN_UINT16_NA(gpu_metrics.xcp_stats[0].jpeg_busy[i],
                                            UINT64_MAX);
                }
                watch_db->watch_info[gpu_id].jpeg_busy_inst_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_CURRENT_ACC_COUNTER:
            if (violation_get_succeeded) {
                watch_db->watch_info[gpu_id].viol_current_acc_counter =
                    violation_status.acc_counter;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_ACC:
            if (violation_get_succeeded) {
                watch_db->watch_info[gpu_id].viol_proc_hot_residency_acc =
                    violation_status.acc_prochot_thrm;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_ACC:
            if (violation_get_succeeded) {
                watch_db->watch_info[gpu_id].viol_ppt_residency_acc =
                    violation_status.acc_ppt_pwr;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_ACC:
            if (violation_get_succeeded) {
                watch_db->watch_info[gpu_id].viol_socket_thermal_residency_acc =
                    violation_status.acc_socket_thrm;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_ACC:
            if (violation_get_succeeded) {
                watch_db->watch_info[gpu_id].viol_vr_thermal_residency_acc =
                    violation_status.acc_vr_thrm;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_ACC:
            if (violation_get_succeeded) {
                watch_db->watch_info[gpu_id].viol_hbm_thermal_residency_acc =
                    violation_status.acc_hbm_thrm;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_PROC_HOT_RESIDENCY_PCT:
            if (violation_get_succeeded) {
                watch_db->watch_info[gpu_id].viol_proc_hot_residency_pct =
                    violation_status.per_prochot_thrm;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_PPT_RESIDENCY_PCT:
            if (violation_get_succeeded) {
                watch_db->watch_info[gpu_id].viol_ppt_residency_pct =
                    violation_status.per_ppt_pwr;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_SOCKET_THERMAL_RESIDENCY_PCT:
            if (violation_get_succeeded) {
                watch_db->watch_info[gpu_id].viol_socket_thermal_residency_pct =
                    violation_status.per_socket_thrm;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_VR_THERMAL_RESIDENCY_PCT:
            if (violation_get_succeeded) {
                watch_db->watch_info[gpu_id].viol_vr_thermal_residency_pct =
                    violation_status.per_vr_thrm;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_HBM_THERMAL_RESIDENCY_PCT:
            if (violation_get_succeeded) {
                watch_db->watch_info[gpu_id].viol_hbm_thermal_residency_pct =
                    violation_status.per_hbm_thrm;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_ACC:
            if (violation_get_succeeded) {
                uint32_t n = AMDSMI_MAX_NUM_XCC;
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].viol_gfx_below_power_acc[i] =
                        violation_status.acc_gfx_clk_below_host_limit_pwr[0][i];
                }
                watch_db->watch_info[gpu_id].viol_gfx_below_power_acc_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_ACC:
            if (violation_get_succeeded) {
                uint32_t n = AMDSMI_MAX_NUM_XCC;
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].viol_gfx_below_thermal_acc[i] =
                        violation_status.acc_gfx_clk_below_host_limit_thm[0][i];
                }
                watch_db->watch_info[gpu_id].viol_gfx_below_thermal_acc_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_ACC:
            if (violation_get_succeeded) {
                uint32_t n = AMDSMI_MAX_NUM_XCC;
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].viol_gfx_low_util_acc[i] =
                        violation_status.acc_low_utilization[0][i];
                }
                watch_db->watch_info[gpu_id].viol_gfx_low_util_acc_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_ACC:
            if (violation_get_succeeded) {
                uint32_t n = AMDSMI_MAX_NUM_XCC;
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].viol_gfx_below_total_acc[i] =
                        violation_status.acc_gfx_clk_below_host_limit_total[0][i];
                }
                watch_db->watch_info[gpu_id].viol_gfx_below_total_acc_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_POWER_PCT:
            if (violation_get_succeeded) {
                uint32_t n = AMDSMI_MAX_NUM_XCC;
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].viol_gfx_below_power_pct[i] =
                        violation_status.per_gfx_clk_below_host_limit_pwr[0][i];
                }
                watch_db->watch_info[gpu_id].viol_gfx_below_power_pct_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_THERMAL_PCT:
            if (violation_get_succeeded) {
                uint32_t n = AMDSMI_MAX_NUM_XCC;
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].viol_gfx_below_thermal_pct[i] =
                        violation_status.per_gfx_clk_below_host_limit_thm[0][i];
                }
                watch_db->watch_info[gpu_id].viol_gfx_below_thermal_pct_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_LOW_UTIL_PCT:
            if (violation_get_succeeded) {
                uint32_t n = AMDSMI_MAX_NUM_XCC;
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].viol_gfx_low_util_pct[i] =
                        violation_status.per_low_utilization[0][i];
                }
                watch_db->watch_info[gpu_id].viol_gfx_low_util_pct_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_VIOLATION_GFX_CLK_BELOW_HOST_LIMIT_TOTAL_PCT:
            if (violation_get_succeeded) {
                uint32_t n = AMDSMI_MAX_NUM_XCC;
                if (n > AGA_GPU_WATCH_MAX_INDEXED_VALS) {
                    n = AGA_GPU_WATCH_MAX_INDEXED_VALS;
                }
                for (uint32_t i = 0; i < n; i++) {
                    watch_db->watch_info[gpu_id].viol_gfx_below_total_pct[i] =
                        violation_status.per_gfx_clk_below_host_limit_total[0][i];
                }
                watch_db->watch_info[gpu_id].viol_gfx_below_total_pct_count = n;
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_CORRECT_TOTAL:
            // NA when no per block read succeeded, so an all-failed refresh is
            // not reported as a real zero error count
            watch_db->watch_info[gpu_id].total_correctable_errors =
                ecc_read_succeeded ? total_correctable_count : UINT64_MAX;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_UNCORRECT_TOTAL:
            watch_db->watch_info[gpu_id].total_uncorrectable_errors =
                ecc_read_succeeded ? total_uncorrectable_count : UINT64_MAX;
            break;
        case AGA_GPU_WATCH_ATTR_ID_ECC_DEF_TOTAL:
            watch_db->watch_info[gpu_id].total_deferred_errors =
                ecc_read_succeeded ? total_deferred_count : UINT64_MAX;
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_NOP_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_0_NOP_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor0_tx_nops =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_REQ_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_0_REQUEST_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor0_tx_requests =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_RESP_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_0_RESPONSE_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor0_tx_responses =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_BEATS_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_0_BEATS_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor0_tx_beats =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_NOP_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_1_NOP_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor1_tx_nops =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_REQ_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_1_REQUEST_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor1_tx_requests =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_RESP_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_1_RESPONSE_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor1_tx_responses =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_BEATS_TX:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_1_BEATS_TX;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    watch_db->watch_info[gpu_id].xgmi_neighbor1_tx_beats =
                        counter_value.value;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_0_THRPUT:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_DATA_OUT_0;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    time_sec =
                        (double)(counter_value.time_running) / 1000000000.0;
                    watch_db->watch_info[gpu_id].xgmi_neighbor0_tx_throughput =
                        (counter_value.value * 32) / time_sec;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_1_THRPUT:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_DATA_OUT_1;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    time_sec =
                        (double)(counter_value.time_running) / 1000000000.0;
                    watch_db->watch_info[gpu_id].xgmi_neighbor1_tx_throughput =
                        (counter_value.value * 32) / time_sec;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_2_THRPUT:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_DATA_OUT_2;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    time_sec =
                        (double)(counter_value.time_running) / 1000000000.0;
                    watch_db->watch_info[gpu_id].xgmi_neighbor2_tx_throughput =
                        (counter_value.value * 32) / time_sec;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_3_THRPUT:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_DATA_OUT_3;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    time_sec =
                        (double)(counter_value.time_running) / 1000000000.0;
                    watch_db->watch_info[gpu_id].xgmi_neighbor3_tx_throughput =
                        (counter_value.value * 32) / time_sec;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_4_THRPUT:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_DATA_OUT_4;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    time_sec =
                        (double)(counter_value.time_running) / 1000000000.0;
                    watch_db->watch_info[gpu_id].xgmi_neighbor4_tx_throughput =
                        (counter_value.value * 32) / time_sec;
                }
            }
            break;
        case AGA_GPU_WATCH_ATTR_ID_XGMI_5_THRPUT:
            {
                counter_key = reinterpret_cast<uint64_t>(gpu_handle) +
                                  AMDSMI_EVNT_XGMI_DATA_OUT_5;
                auto it = counter_handle_.find(counter_key);
                if (it != counter_handle_.end()) {
                    amdsmi_ret = amdsmi_gpu_read_counter(it->second,
                                                         &counter_value);
                    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
                        break;
                    }
                    time_sec =
                        (double)(counter_value.time_running) / 1000000000.0;
                    watch_db->watch_info[gpu_id].xgmi_neighbor5_tx_throughput =
                        (counter_value.value * 32) / time_sec;
                }
            }
            break;
        default:
            break;
        }
    }
    if (refresh_slow) {
        slow_cache_[gpu_id] = watch_db->watch_info[gpu_id];
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::watcher_update_watch_db(aga_gpu_watch_db_t *watch_db,
                                   bool refresh_slow) {
    // loop through all gpu devices
    for (uint32_t gpu = 0; gpu < num_gpu_; gpu++) {
        // update watch db
        smi_watcher_update_all_watch_fields_(gpu, gpu_handles_[gpu], watch_db,
                                             refresh_slow);
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::cleanup_gpu_watch_inactive_subscribers_(
               vector<gpu_watch_subscriber_info_t>& subscribers) {
    sdk_ret_t ret;
    gpu_watch_subscriber_info_t subscriber;
    aga_gpu_watch_client_ctxt_t *client_ctxt;
    set<aga_gpu_watch_client_ctxt_t *> client_set;

    for (auto it = subscribers.begin(); it != subscribers.end(); it++) {
        aga_task_spec_t task_spec = {};

        subscriber = *it;
        auto& client_info =
            gpu_watch_subscriber_db_.gpu_watch_map[subscriber.gpu_watch_id];
        // erase the client
        client_info.client_set.erase(subscriber.client_ctxt);

        // post task to API thread to decrement subscriber refcount

        // NOTE: multiple subscribers can become inactive for a given GPU watch
        // object, post 1 task for each inactive subscriber instead of one
        // task for all inactive subscribers
        task_spec.task = AGA_TASK_GPU_WATCH_SUBSCRIBE_DEL;
        task_spec.subscriber_spec.num_gpu_watch_ids = 1;
        task_spec.subscriber_spec.gpu_watch_ids[0] = subscriber.gpu_watch_id;
        ret = aga_task_create(&task_spec);
        if (unlikely(ret != SDK_RET_OK)) {
            AGA_TRACE_ERR("Failed to create task to decrement subscriber count "
                          "for GPU watch {}, client {}, client ctxt {}",
                          subscriber.gpu_watch_id.str(),
                          subscriber.client_ctxt->client.c_str(),
                          (void *)subscriber.client_ctxt);
        }
        client_set.insert(subscriber.client_ctxt);
    }

    for (auto it = client_set.begin(); it!= client_set.end(); it++) {
        client_ctxt = *it;
        // wakeup the front end thread so it can exit
        client_ctxt->client_inactive = true;
        AGA_TRACE_INFO("Signaling frontend gRPC thread to quit, client {}, "
                       "client ctxt {}, stream {}",
                       client_ctxt->client.c_str(),
                       (void *)client_ctxt,
                       client_ctxt->stream);
        pthread_cond_signal(&client_ctxt->cond);
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::gpu_watch_notify_subscribers(void) {
    sdk_ret_t ret;
    aga_obj_key_t key;
    aga_gpu_watch_client_ctxt_t *client_ctxt;
    gpu_watch_subscriber_info_t inactive_subscriber;
    vector<gpu_watch_subscriber_info_t> inactive_subscribers;
    // heap-allocated: the struct is large (AGA_MAX_GPU x AGA_GPU_WATCH_ATTRS_MAX
    // array-valued attrs) and this notify path would overflow the thread stack
    std::unique_ptr<aga_gpu_watch_info_t> info_holder(new aga_gpu_watch_info_t());
    aga_gpu_watch_info_t &info = *info_holder;

    for (auto& it : gpu_watch_subscriber_db_.gpu_watch_map) {
        key = it.first;
        auto& client_info = it.second;

        AGA_TRACE_VERBOSE("GPU watch {} notify subscribers", key.str());
        memset(&info, 0, sizeof(aga_gpu_watch_info_t));
        aga_gpu_watch_read(&key, &info);
        for (auto client_set_it = client_info.client_set.begin();
             client_set_it != client_info.client_set.end();
             client_set_it++) {
             client_ctxt = *client_set_it;
            ret = client_ctxt->write_cb(&info, client_ctxt);
            if (unlikely(ret != SDK_RET_OK)) {
                // add to list of clients not reachable
                inactive_subscriber.gpu_watch_id = info.spec.key;
                inactive_subscriber.client_ctxt = client_ctxt;
                inactive_subscribers.push_back(inactive_subscriber);
            }
        }
    }
    cleanup_gpu_watch_inactive_subscribers_(inactive_subscribers);
    return SDK_RET_OK;
}

static void
watch_timer_cb_ (event::timer_t *timer)
{
    sdk_ret_t ret;
    bool refresh_slow;
    aga_task_spec_t task_spec = {};
    static uint16_t fast_ticks = 0;
    static uint16_t slow_ticks = 0;
    static bool slow_primed = false;

    // only start updating watch fields once at least one client has subscribed
    // to a watch group
    if (!g_smi_state.any_watch_group_subscribed()) {
        return;
    }
    // the timer fires every AGA_WATCHER_INTERVAL seconds; sample the fast tier
    // once per fast period and the slow tier once per slow period
    fast_ticks += uint16_t(AGA_WATCHER_INTERVAL);
    slow_ticks += uint16_t(AGA_WATCHER_INTERVAL);
    if (fast_ticks < smi_watch_fast_period()) {
        return;
    }
    fast_ticks = 0;
    // force a slow refresh on the very first published sample, else counters
    // would be reported as zero until the first slow period elapses
    refresh_slow = !slow_primed || (slow_ticks >= smi_watch_slow_period());
    if (refresh_slow) {
        slow_ticks = 0;
        slow_primed = true;
    }
    // get latest values of all watch fields
    g_smi_state.watcher_update_watch_db(&task_spec.watch_db, refresh_slow);

    // post task to api thread
    task_spec.task = AGA_TASK_GPU_WATCH_DB_UPDATE;
    ret = aga_task_create(&task_spec);
    if (unlikely(ret != SDK_RET_OK)) {
        AGA_TRACE_ERR("Failed to create GPU watch db update task, err {}",
                      ret());
    }
    g_smi_state.gpu_watch_notify_subscribers();
}

/// \brief process an gpu watch subscribe request from client
/// \param[in] args    pointer to incoming request
/// \return SDK_RET_OK if success or error code in case of failure
sdk_ret_t
smi_state::process_gpu_watch_subscribe_req(
               aga_gpu_watch_subscribe_args_t *args) {
    gpu_watch_client_info_t client_info = {};

    for (size_t i = 0; i < args->gpu_watch_ids.size(); i++) {
        AGA_TRACE_DEBUG("Rcvd GPU watch {} subscribe request, client {}, "
                        "client ctxt {}, stream {}",
                        args->gpu_watch_ids[i].str(),
                        args->client_ctxt->client.c_str(),
                        (void *)args->client_ctxt,
                        (void *)args->client_ctxt->stream);
        auto& gpu_watch_map = gpu_watch_subscriber_db_.gpu_watch_map;
        // check if this watch was of interest to any client
        // already
        auto it = gpu_watch_map.find(args->gpu_watch_ids[i]);
        if (it == gpu_watch_map.end()) {
            // 1st time anyone is subscribing to this gpu watch
            client_info.client_set.insert(args->client_ctxt);
            gpu_watch_map[args->gpu_watch_ids[i]] = client_info;
        } else {
            // atleast one client is already interested in this gpu watch ,
            // check if this particular client already subscribed to this
            // gpu watch group
            auto set_it = it->second.client_set.find(
                    args->client_ctxt);
            if (set_it == it->second.client_set.end()) {
                // this client is a new subscriber for this gpu watch group
                it->second.client_set.insert(args->client_ctxt);
            } else {
                // this client is already subscribed to this gpu watch group
            }
        }
    }
    return SDK_RET_OK;
}

static void
gpu_watch_subscribe_ipc_cb_ (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    sdk_ret_t ret;
    aga_gpu_watch_subscribe_args_t *args;

    args = *(aga_gpu_watch_subscribe_args_t **)msg->data();
    if (args == NULL) {
        AGA_TRACE_ERR("Ignoring NULL GPU watch subscribe request received");
        return;
    }
    ret = g_smi_state.process_gpu_watch_subscribe_req(args);
    sdk::ipc::respond(msg, &ret, sizeof(ret));
}

static sdk_ret_t
gpu_create_counter_ (amdsmi_processor_handle gpu_handle,
                     amdsmi_event_type_t event_type,
                     amdsmi_event_handle_t *counter_handle)
{
    amdsmi_status_t amdsmi_ret;

    amdsmi_ret = amdsmi_gpu_create_counter(gpu_handle, event_type,
                                           counter_handle);
    if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
        AGA_TRACE_ERR("Failed to create counter {} on GPU {}, err {}",
                      event_type, gpu_handle, amdsmi_ret);
        return amdsmi_ret_to_sdk_ret(amdsmi_ret);
    } else {
        amdsmi_ret = amdsmi_gpu_control_counter(*counter_handle,
                                                AMDSMI_CNTR_CMD_START,
                                                NULL);
        if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
            AGA_TRACE_ERR("Failed to start counter {} on GPU {}, err {}",
                          event_type, gpu_handle, amdsmi_ret);
            return amdsmi_ret_to_sdk_ret(amdsmi_ret);
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::watcher_init(void) {
    uint32_t counters;
    uint64_t counter_key;
    sdk_ret_t ret = SDK_RET_OK;
    amdsmi_status_t amdsmi_ret;
    amdsmi_event_handle_t counter_handle;

    // initialize watch field list
    smi_watch_field_list_init();

    // create counters for xgmi stats
    for (uint32_t gpu = 0; gpu < num_gpu_; gpu++) {
        // check if xgmi counter groups are supported
        amdsmi_ret = amdsmi_gpu_counter_group_supported(gpu_handles_[gpu],
                                                        AMDSMI_EVNT_GRP_XGMI);
        if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
            AGA_TRACE_ERR("XGMI counter group not supported on GPU {}, ret {}",
                          gpu_handles_[gpu], amdsmi_ret);
            continue;
        }
        // check if atleast 8 counters are available for XGMI coutner group
        amdsmi_ret = amdsmi_get_gpu_available_counters(gpu_handles_[gpu],
                                                       AMDSMI_EVNT_GRP_XGMI,
                                                       &counters);
        if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
            AGA_TRACE_ERR("Counters unavailable for XGMI counter group on "
                          "GPU {}, ret {}", gpu_handles_[gpu], amdsmi_ret);
            continue;
        } else if (counters < 8) {
            AGA_TRACE_ERR("Only {} counters available for XGMI on GPU {}, "
                          "require {} counters", counters, gpu_handles_[gpu],
                          8);
            continue;
        }
        // create XGMI counters
        ret = gpu_create_counter_(gpu_handles_[gpu], AMDSMI_EVNT_XGMI_0_NOP_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_0_NOP_TX;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_0_REQUEST_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_0_REQUEST_TX;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_0_RESPONSE_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_0_RESPONSE_TX;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_0_BEATS_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_0_BEATS_TX;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu], AMDSMI_EVNT_XGMI_1_NOP_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_1_NOP_TX;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_1_REQUEST_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_1_REQUEST_TX;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_1_RESPONSE_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_1_RESPONSE_TX;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_1_BEATS_TX,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_1_BEATS_TX;
            counter_handle_[counter_key] = counter_handle;
        }
    }
    // create counters for xgmi throughput stats
    for (uint32_t gpu = 0; gpu < num_gpu_; gpu++) {
        // check if xgmi counter groups are supported
        amdsmi_ret = amdsmi_gpu_counter_group_supported(gpu_handles_[gpu],
                         AMDSMI_EVNT_GRP_XGMI_DATA_OUT);
        if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
            AGA_TRACE_ERR("XGMI throughput counter group not supported on "
                          "GPU {}, ret {}", gpu_handles_[gpu], amdsmi_ret);
            continue;
        }
        // check if atleast 6 counters are available for XGMI coutner group
        amdsmi_ret = amdsmi_get_gpu_available_counters(gpu_handles_[gpu],
                         AMDSMI_EVNT_GRP_XGMI_DATA_OUT, &counters);
        if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
            AGA_TRACE_ERR("Counters unavailable for XGMI throughput counter "
                          "group on GPU {}, ret {}", gpu_handles_[gpu],
                          amdsmi_ret);
            continue;
        } else if (counters < 6) {
            AGA_TRACE_ERR("Only {} counters available for XGMI throughput on "
                          "GPU {}, require {} counters", counters,
                          gpu_handles_[gpu], 6);
            continue;
        }
        // create XGMI throughput counters
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_DATA_OUT_0,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_DATA_OUT_0;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_DATA_OUT_1,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_DATA_OUT_1;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_DATA_OUT_2,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_DATA_OUT_2;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_DATA_OUT_3,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_DATA_OUT_3;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_DATA_OUT_4,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_DATA_OUT_4;
            counter_handle_[counter_key] = counter_handle;
        }
        ret = gpu_create_counter_(gpu_handles_[gpu],
                                  AMDSMI_EVNT_XGMI_DATA_OUT_5,
                                  &counter_handle);
        if (ret == SDK_RET_OK) {
            counter_key = reinterpret_cast<uint64_t>(gpu_handles_[gpu]) +
                              AMDSMI_EVNT_XGMI_DATA_OUT_5;
            counter_handle_[counter_key] = counter_handle;
        }
    }
    return ret;
}

sdk_ret_t
smi_state::read_counter(aga_gpu_handle_t gpu_handle, uint64_t counter,
                        uint64_t *value)
{
    uint64_t counter_key;
    amdsmi_status_t amdsmi_ret;
    amdsmi_counter_value_t counter_value;

    *value = 0;
    counter_key = reinterpret_cast<uint64_t>(gpu_handle) + counter;
    auto it = counter_handle_.find(counter_key);
    if (it != counter_handle_.end()) {
        amdsmi_ret = amdsmi_gpu_read_counter(it->second, &counter_value);
        if (amdsmi_ret != AMDSMI_STATUS_SUCCESS) {
            AGA_TRACE_VERBOSE("Failed to read counter {} for GPU {}, err ()",
                              counter, gpu_handle, amdsmi_ret);
            return amdsmi_ret_to_sdk_ret(amdsmi_ret);
        }
        *value = counter_value.value;
    }
    return SDK_RET_INVALID_ARG;
}

static void
watcher_thread_init_ (void *ctxt)
{
    static event::timer_t watch_timer;

    g_smi_state.watcher_init();
    // register for gpu watch subscribe messages
    sdk::ipc::reg_request_handler(AGA_IPC_MSG_ID_GPU_WATCH_SUBSCRIBE,
                                  gpu_watch_subscribe_ipc_cb_, NULL);
    // start watch timer
    event::timer_init(&watch_timer, watch_timer_cb_,
                      AGA_WATCHER_START_DELAY, AGA_WATCHER_INTERVAL);
    event::timer_start(&watch_timer);
}

static void
watcher_thread_exit_ (void *ctxt)
{
    // TODO: any timer related cleanup required?
}

sdk_ret_t
smi_state::spawn_watcher_thread_(void) {
    watcher_thread_ =
        sdk::event_thread::event_thread::factory(
            "smi-watcher", AGA_THREAD_ID_WATCHER,
            sdk::lib::THREAD_ROLE_CONTROL, 0x0, watcher_thread_init_,
            watcher_thread_exit_, NULL, // message
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            (THREAD_YIELD_ENABLE | THREAD_SYNC_IPC_ENABLE));
    SDK_ASSERT_TRACE_RETURN((watcher_thread_ != NULL), SDK_RET_ERR,
                            "GPU watcher thread create failure");
    watcher_thread_->start(NULL);
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::event_monitor_init(void) {
    amdsmi_status_t status;
    gpu_event_record_t null_event_record = {};

    if (events_disabled()) {
        return SDK_RET_OK;
    }
    // initialize the s/w state
    for (uint32_t d = 0; d < num_gpu_; d++) {
        SDK_SPINLOCK_INIT(&gpu_event_db_[gpu_handles_[d]].slock,
                          PTHREAD_PROCESS_SHARED);
    }
    // initialize event monitoring for all the devices
    for (uint32_t d = 0; d < num_gpu_; d++) {
        // initialize the event monitoring for the 1st time for all devices
        status = amdsmi_init_gpu_event_notification(gpu_handles_[d]);
        if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to do event notification initialization, "
                          "GPU {}, err {}", gpu_handles_[d], status);
            return amdsmi_ret_to_sdk_ret(status);
        }
        status = amdsmi_set_gpu_event_notification_mask(gpu_handles_[d],
                                                        AMDSMI_EVENT_MASK_ALL);
        if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
            AGA_TRACE_ERR("Failed to set event notification mask, "
                          "GPU {}, err {}", gpu_handles_[d], status);
            return amdsmi_ret_to_sdk_ret(status);
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::event_monitor_cleanup(void) {
    if (events_disabled()) {
        return SDK_RET_OK;
    }
    // stop monitoring
    for (uint32_t d = 0; d < num_gpu_; d++) {
        amdsmi_stop_gpu_event_notification(gpu_handles_[d]);
    }
    // cleanup the event state
    for (uint32_t d = 0; d < num_gpu_; d++) {
        SDK_SPINLOCK_LOCK(&gpu_event_db_[gpu_handles_[d]].slock);
        gpu_event_db_[gpu_handles_[d]].event_map.clear();
        SDK_SPINLOCK_UNLOCK(&gpu_event_db_[gpu_handles_[d]].slock);
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::cleanup_event_listeners_(
               vector<aga_event_listener_info_t>& listeners) {
    aga_event_listener_info_t listener;
    aga_event_client_ctxt_t *client_ctxt;
    set<aga_event_client_ctxt_t *> client_set;

    for (auto it = listeners.begin(); it != listeners.end(); it++) {
        listener = *it;

        // if client context of one gpu is inactive,
        // we should erase the client context from all gpus
        // and all events related to this gRPC stream before
        // waking up the front end, otherwise the client contexts
        // stored for other gpus for the same subscribe request
        // will eventually lead to agent crash

        for (uint32_t d = 0; d < num_gpu_; d++) {
            // lock the event state for this device
            SDK_SPINLOCK_LOCK(&gpu_event_db_[gpu_handles_[d]].slock);
            for (uint32_t e = (AGA_EVENT_ID_NONE + 1); e <= AGA_EVENT_ID_MAX;
                 e++) {
                auto& event_record =
                    gpu_event_db_[gpu_handles_[d]].event_map[(aga_event_id_t)e];
                // erase the client
                event_record.client_info.client_set.erase(listener.client_ctxt);
            }
            // unlock the event state for this device
            SDK_SPINLOCK_UNLOCK(&gpu_event_db_[gpu_handles_[d]].slock);
        }
        client_set.insert(listener.client_ctxt);
    }
    for (auto it = client_set.begin(); it!= client_set.end(); it++) {
        client_ctxt = *it;
        // wakeup the front end thread so it can exit
        client_ctxt->client_inactive = true;
        AGA_TRACE_INFO("Signaling frontend gRPC thread to quit, client {}, "
                       "client ctxt {}, stream {}",
                       client_ctxt->client.c_str(),
                       (void *)client_ctxt,
                       client_ctxt->stream);
        pthread_cond_signal(&client_ctxt->cond);
    }
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::handle_events(uint32_t num_events, void *event_buffer_) {
    sdk_ret_t ret;
    timespec_t ts;
    gpu_entry *gpu;
    aga_event_t event = {};
    aga_event_id_t event_id;
    amdsmi_processor_handle gpu_handle;
    aga_event_client_ctxt_t *client_ctxt;
    aga_event_listener_info_t inactive_listener;
    amdsmi_evt_notification_data_t *event_buffer;
    vector<aga_event_listener_info_t> inactive_listeners;

    event_buffer = (amdsmi_evt_notification_data_t *)event_buffer_;
    // get current time
    clock_gettime(CLOCK_REALTIME, &ts);
    // start processing all the events
    for (uint32_t i = 0; i < num_events; i++) {
        gpu_handle = event_buffer[i].processor_handle;
        gpu = gpu_db()->find(gpu_handle);
        if (gpu == NULL) {
            continue;
        }
        event_id = aga_event_id_from_smi_event_id(event_buffer[i].event);
        auto& event_map = gpu_event_db_[gpu_handle].event_map;

        // lock the event state for this device
        SDK_SPINLOCK_LOCK(&gpu_event_db_[gpu_handle].slock);
        // update our event state
        auto& event_record = event_map[event_id];
        event_record.timestamp = ts;
        strncpy(event_record.message, event_buffer[i].message,
                AGA_MAX_EVENT_STR);
        event_record.message[AGA_MAX_EVENT_STR] = '\0';
        // fill the event record
        event.id = event_id;
        event.timestamp = ts;
        event.gpu = gpu->key();
        strncpy(event.message, event_buffer[i].message,
                AGA_MAX_EVENT_STR);
        event.message[AGA_MAX_EVENT_STR] = '\0';
        // walk thru all the clients that are interested in this event and
        // notify them
        for (auto client_set_it = event_record.client_info.client_set.begin();
             client_set_it != event_record.client_info.client_set.end();
             client_set_it++) {
             client_ctxt = *client_set_it;
            // invoke the event notification callback
            ret = client_ctxt->notify_cb(&event, *client_set_it);
            if (unlikely(ret != SDK_RET_OK)) {
                // add to list of clients not reachable
                inactive_listener.gpu_id = gpu->id();
                inactive_listener.event = event_id;
                inactive_listener.client_ctxt = *client_set_it;
                inactive_listeners.push_back(inactive_listener);
            }
        }
        // unlock the event state maintained for this device
        SDK_SPINLOCK_UNLOCK(&gpu_event_db_[gpu_handle].slock);
    }
    // handle all the dead clients now
    cleanup_event_listeners_(inactive_listeners);
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::event_read(aga_event_read_cb_t cb, void *ctxt) {
    aga_event_t event;

    if (events_disabled()) {
        return SDK_RET_OK;
    }
    // traverse the event database per device
    for (uint32_t d = 0; d < num_gpu_; d++) {
        auto gpu = gpu_db()->find(gpu_handles_[d]);
        if (gpu == NULL) {
            continue;
        }
        auto& event_map = gpu_event_db_[gpu_handles_[d]].event_map;
        // lock the event map for this device
        SDK_SPINLOCK_LOCK(&gpu_event_db_[gpu_handles_[d]].slock);
        for (auto it = event_map.begin(); it != event_map.end(); it++) {
            event = {};
            auto& event_record = it->second;

            // fill the event information
            event.id = it->first;
            event.timestamp = event_record.timestamp;
            event.gpu = gpu->key();
            strncpy(event.message, event_record.message, AGA_MAX_EVENT_STR);
            event.message[AGA_MAX_EVENT_STR] = '\0';
            // call the callback now
            cb(&event, ctxt);
        }
        // unlock the event map for this device
        SDK_SPINLOCK_UNLOCK(&gpu_event_db_[gpu_handles_[d]].slock);
    }
    return SDK_RET_OK;
}

static void
event_monitor_timer_cb_ (event::timer_t *timer)
{
    amdsmi_status_t status;
    uint32_t num_elem = AGA_MAX_GPU * AGA_EVENT_ID_MAX;
    amdsmi_evt_notification_data_t event_ntfn_data[num_elem];

    if (events_disabled()) {
        return;
    }
    // get event information
    status = amdsmi_get_gpu_event_notification(AGA_SMI_EVENT_MONITOR_INTERVAL,
                                               &num_elem, event_ntfn_data);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to get event notification data, err {}", status);
        return;
    }

    // handle all the events
    g_smi_state.handle_events(num_elem, event_ntfn_data);
}

/// \brief process an event subscribe request from client
/// \param[in] req    pointer to incoming request
/// \return SDK_RET_OK if success or error code in case of failure
sdk_ret_t
smi_state::process_event_subscribe_req(aga_event_subscribe_args_t *req) {
    gpu_event_record_t event_record = {};

    for (size_t i = 0; i < req->events.size(); i++) {
        AGA_TRACE_DEBUG("Rcvd event {} subscribe request, client {}, "
                        "client ctxt {}, stream {}",  req->events[i],
                        req->client_ctxt->client.c_str(),
                        (void *)req->client_ctxt,
                        (void *)req->client_ctxt->stream);
        for (size_t g = 0; g < req->gpu_ids.size(); g++) {
            uint32_t d = req->gpu_ids[g];
            auto& event_map = gpu_event_db_[gpu_handles_[d]].event_map;

            // lock the event map for this device
            SDK_SPINLOCK_LOCK(&gpu_event_db_[gpu_handles_[d]].slock);
            // check if this event was of interest to any client or happened
            // already
            auto event_map_it = event_map.find(req->events[i]);
            if (event_map_it == event_map.end()) {
                // 1st time anyone is subscribing to this event
                event_record.client_info.client_set.insert(req->client_ctxt);
                event_map[req->events[i]] = event_record;
            } else {
                // atleast one client is already interested in this event, check
                // if this particular client already subscribed to this event
                auto set_it = event_map_it->second.client_info.client_set.find(
                                                       req->client_ctxt);
                if (set_it ==
                        event_map_it->second.client_info.client_set.end()) {
                    // this client is a new listener for this event
                    event_map_it->second.client_info.client_set.insert(
                                                         req->client_ctxt);
                } else {
                    // this client is already subscribed to this event
                }
            }
            // unlock the event map for this device
            SDK_SPINLOCK_UNLOCK(&gpu_event_db_[gpu_handles_[d]].slock);
        }
    }
    return SDK_RET_OK;
}

/// \brief callback function to process IPC msg from gRPC thread
///        to handle event subscription requests
/// \param[in] msg    received IPC message
/// \param[in] ctxt   opaque context (used when callback was registered)
static void
event_subscribe_ipc_cb_ (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    sdk_ret_t ret;
    aga_event_subscribe_args_t *req;

    if (events_disabled()) {
        ret = SDK_RET_OK;
        sdk::ipc::respond(msg, &ret, sizeof(ret));
        return;
    }
    req = *(aga_event_subscribe_args_t **)msg->data();
    if (req == NULL) {
        AGA_TRACE_ERR("Ignoring NULL event subscribe request received");
        return;
    }
    ret = g_smi_state.process_event_subscribe_req(req);
    sdk::ipc::respond(msg, &ret, sizeof(ret));
}

static inline const char *
event_description_ (aga_event_id_t event_id)
{
    switch (event_id) {
    case AGA_EVENT_ID_VM_PAGE_FAULT:
        return "Test event: VM page fault occured";
    case AGA_EVENT_ID_THERMAL_THROTTLE:
        return
            "Test event: clock frequency has decreased due to temperature rise";
    case AGA_EVENT_ID_GPU_PRE_RESET:
        return "Teset event: GPU reset about to happen";
    case AGA_EVENT_ID_GPU_POST_RESET:
        return "Test event: GPU reset happened";
    case AGA_EVENT_ID_MIGRATE_START:
        return "Test event: GPU migrate start";
    case AGA_EVENT_ID_MIGRATE_END:
        return "Test event: GPU migrate end";
    case AGA_EVENT_ID_PAGE_FAULT_START:
        return "Test event: GPU page fault start";
    case AGA_EVENT_ID_PAGE_FAULT_END:
        return "Test event: GPU page fault end";
    case AGA_EVENT_ID_QUEUE_EVICTION:
        return "Test event: GPU queue eviction";
    case AGA_EVENT_ID_QUEUE_RESTORE:
        return "Test event: GPU queue restore";
    case AGA_EVENT_ID_UNMAP_FROM_GPU:
        return "Test event: Unmap from GPU";
    case AGA_EVENT_ID_PROCESS_START:
        return "Test event: KFD process start";
    case AGA_EVENT_ID_PROCESS_END:
        return "Test event: KFD process end";
    default:
        break;
    }
    return "Test event: unknown event";
}

sdk_ret_t
smi_state::process_event_gen_req(aga_event_gen_args_t *args) {
    sdk_ret_t ret;
    uint32_t num_elem = 0;
    amdsmi_evt_notification_type_t smi_event;
    amdsmi_evt_notification_data_t event_data[AGA_MAX_GPU * AGA_EVENT_ID_MAX];

    for (size_t i = 0; i < args->gpu_ids.size(); i++) {
        for (size_t e = 0; e < args->events.size(); e++) {
            ret = aga_event_id_to_smi_event_id(args->events[e], &smi_event);
            if (unlikely(ret != SDK_RET_OK)) {
                AGA_TRACE_ERR("Failed to generate event {}, smi event not "
                              "found", args->events[e]);
                return ret;
            }
            event_data[num_elem].event = smi_event;
            event_data[num_elem].processor_handle =
                gpu_handles_[args->gpu_ids[i]];
            strncpy(event_data[num_elem].message,
                    event_description_(args->events[e]),
                    AMDSMI_MAX_STRING_LENGTH);
            num_elem++;
        }
    }
    // handle all the events
    g_smi_state.handle_events(num_elem, event_data);
    return SDK_RET_OK;
}

/// \brief callback function to process IPC msg from gRPC thread
///        to handle event generate requests
/// \param[in] msg    received IPC message
/// \param[in] ctxt   opaque context (used when callback was registered)
static void
event_gen_ipc_cb_ (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    sdk_ret_t ret;
    aga_event_gen_args_t *args;

    if (events_disabled()) {
        ret = SDK_RET_OK;
        sdk::ipc::respond(msg, &ret, sizeof(ret));
        return;
    }
    args = (aga_event_gen_args_t *)msg->data();
    if (args == NULL) {
        AGA_TRACE_ERR("Ignoring NULL event generate request received");
        return;
    }
    ret = g_smi_state.process_event_gen_req(args);
    sdk::ipc::respond(msg, &ret, sizeof(ret));
}

static void
event_monitor_thread_init_ (void *ctxt)
{
    static event::timer_t event_monitor_timer;

    // initialize event monitoring state
    g_smi_state.event_monitor_init();
    // subscribe to all IPC msgs of interest
    sdk::ipc::reg_request_handler(AGA_IPC_MSG_ID_EVENT_SUBSCRIBE,
                                  event_subscribe_ipc_cb_, NULL);
    sdk::ipc::reg_request_handler(AGA_IPC_MSG_ID_EVENT_GEN,
                                  event_gen_ipc_cb_, NULL);
    // start event monitoring timer
    event::timer_init(&event_monitor_timer, event_monitor_timer_cb_,
                      AGA_SMI_EVENT_MONITOR_START_DELAY,
                      AGA_SMI_EVENT_MONITOR_INTERVAL);
    event::timer_start(&event_monitor_timer);
}

void
event_monitor_thread_exit_ (void *ctxt)
{
    // cleanup the event monitoring state
    g_smi_state.event_monitor_cleanup();
}

sdk_ret_t
smi_state::spawn_event_monitor_thread_(void) {
    event_monitor_thread_ =
        sdk::event_thread::event_thread::factory(
            "event-monitor", AGA_THREAD_ID_EVENT_MONITOR,
            sdk::lib::THREAD_ROLE_CONTROL, 0x0, event_monitor_thread_init_,
            event_monitor_thread_exit_, NULL, // message
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(
                                  sdk::lib::THREAD_ROLE_CONTROL),
            (THREAD_YIELD_ENABLE | THREAD_SYNC_IPC_ENABLE));
    SDK_ASSERT_TRACE_RETURN((event_monitor_thread_ != NULL), SDK_RET_ERR,
                            "GPU event monitor thread create failure");
    event_monitor_thread_->start(NULL);
    return SDK_RET_OK;
}

sdk_ret_t
static bool
wsl2_detected (void)
{
    struct stat st;
    return (stat("/dev/dxg", &st) == 0) && (stat("/dev/kfd", &st) != 0);
}

smi_state::init(aga_api_init_params_t *init_params) {
    if (!wsl2_detected()) {
        AGA_TRACE_ERR("WSL2 not detected (/dev/dxg absent or /dev/kfd present); "
                      "wslamdsmi backend requires WSL2");
        return SDK_RET_ERR;
    }
    AGA_TRACE_INFO("WSL2 detected; using librocdxg amdsmi backend");

    sdk_ret_t ret;
    amdsmi_status_t status;
    aga_gpu_profile_t gpu[AGA_MAX_GPU];

    // initialize smi library
    status = amdsmi_init(AMDSMI_INIT_AMD_GPUS);
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to initialize amd smi library, err {}", status);
        return amdsmi_ret_to_sdk_ret(status);
    }
    // discover gpus
    ret = aga::smi_discover_gpus(&num_gpu_, gpu);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    for (uint32_t i = 0; i < num_gpu_; i++) {
        gpu_handles_[i] = gpu[i].handle;
    }
    // spawn event monitor thread
    spawn_event_monitor_thread_();
    // spawn watcher thread
    spawn_watcher_thread_();
    initialized_ = true;
    return SDK_RET_OK;
}

sdk_ret_t
smi_state::teardown (void)
{
    amdsmi_status_t status;

    if (!initialized_) {
        return SDK_RET_OK;
    }
    // stop the event monitor and watcher threads before shutting down the SMI
    // library to avoid racing their timer callbacks against SMI teardown
    if (event_monitor_thread_) {
        event_monitor_thread_->stop();
        event_monitor_thread_->wait();
    }
    if (watcher_thread_) {
        watcher_thread_->stop();
        watcher_thread_->wait();
    }
    status = amdsmi_shut_down();
    if (unlikely(status != AMDSMI_STATUS_SUCCESS)) {
        AGA_TRACE_ERR("Failed to shutdown amd smi library, err {}", status);
        return amdsmi_ret_to_sdk_ret(status);
    }
    return SDK_RET_OK;
}

/// \@}

}    // namespace aga
