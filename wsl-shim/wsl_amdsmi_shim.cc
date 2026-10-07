/*
 * wsl_amdsmi_shim.cc — LD_PRELOAD shim redirecting AMD SMI calls to the
 * WSL2-aware libamd_smi from librocdxg (/opt/rocm-wsl/lib/libamd_smi.so)
 * when running under WSL2 (/dev/dxg present, /dev/kfd absent).
 *
 * On bare-metal the shim is a no-op: every intercepted symbol falls through
 * to RTLD_NEXT (the real libamd_smi.so.26).
 *
 * Build:
 *   g++ -std=c++17 -shared -fPIC -ldl \
 *       -I../sw/nic/third-party/rocm/wsl_amd_smi_lib/include \
 *       -o libamd_smi_wsl_shim.so wsl_amdsmi_shim.cc
 *   or: make -C wsl-shim (from repo root)
 *
 * Deploy:
 *   cp libamd_smi_wsl_shim.so /usr/local/metrics/lib/
 *   # Add to /usr/local/etc/metrics/gpuagent.conf:
 *   #   LD_PRELOAD=/usr/local/metrics/lib/libamd_smi_wsl_shim.so
 *
 * Copyright (c) Stephen Bates, 2026. Apache-2.0.
 */

#include <dlfcn.h>
#include <stdio.h>
#include <sys/stat.h>

/* Pull in the vendored amdsmi.h for correct type definitions. */
extern "C" {
#include "amd_smi/amdsmi.h"
}

// ---------------------------------------------------------------------------
// WSL2 detection — run once at constructor time
// ---------------------------------------------------------------------------

static bool   g_wsl2    = false;
static void  *g_wsl_lib = nullptr;

static bool detect_wsl2(void) {
    struct stat st;
    return (stat("/dev/dxg", &st) == 0) && (stat("/dev/kfd", &st) != 0);
}

__attribute__((constructor))
static void shim_init(void) {
    g_wsl2 = detect_wsl2();
    if (!g_wsl2) return;

    g_wsl_lib = dlopen("/opt/rocm-wsl/lib/libamd_smi.so",
                       RTLD_NOW | RTLD_GLOBAL);
    if (!g_wsl_lib) {
        fprintf(stderr,
                "[wsl_amdsmi_shim] WSL2 detected but dlopen failed: %s\n"
                "[wsl_amdsmi_shim] Falling back to system libamd_smi\n",
                dlerror());
        g_wsl2 = false;
    } else {
        fprintf(stderr,
                "[wsl_amdsmi_shim] WSL2: routing amdsmi via "
                "/opt/rocm-wsl/lib/libamd_smi.so\n");
    }
}

// ---------------------------------------------------------------------------
// resolve(name) — returns a function pointer from the WSL library on WSL2,
// or from the next library in the chain otherwise.
// ---------------------------------------------------------------------------

static void *resolve(const char *name) {
    if (g_wsl2 && g_wsl_lib) {
        void *sym = dlsym(g_wsl_lib, name);
        if (sym) return sym;
        // Symbol absent in the WSL library.  On WSL2 all handles (processor,
        // socket, event) are opaque objects created by the librocdxg instance;
        // passing them to the system libamd_smi via RTLD_NEXT would corrupt
        // state or crash.  Return NULL so the SHIM_FN wrapper returns
        // AMDSMI_STATUS_NOT_SUPPORTED instead of calling the wrong library.
        return nullptr;
    }
    return dlsym(RTLD_NEXT, name);
}

// ---------------------------------------------------------------------------
// Interceptor macro — each interposed symbol is defined as a thin wrapper
// that resolves the function pointer on first call (Meyer's static local).
// The signature is taken verbatim from the vendored amdsmi.h via decltype.
// ---------------------------------------------------------------------------

/*
 * SHIM_FN(name, (params...), (args...))
 * Wrap each amdsmi function: resolve from WSL lib or RTLD_NEXT, call through.
 * Variadic macro so commas inside the paren-groups are not seen as arg seps.
 */
#define SHIM_FN(name, params, args)                                  \
extern "C" amdsmi_status_t name params {                            \
    static auto *fn =                                                \
        reinterpret_cast<decltype(&name)>(resolve(#name));           \
    if (!fn) return AMDSMI_STATUS_NOT_SUPPORTED;                     \
    return fn args;                                                  \
}

/* Zero-argument variant (amdsmi_shut_down takes void). */
#define SHIM_FN0(name)                                               \
extern "C" amdsmi_status_t name(void) {                             \
    static auto *fn =                                                \
        reinterpret_cast<decltype(&name)>(resolve(#name));           \
    if (!fn) return AMDSMI_STATUS_NOT_SUPPORTED;                     \
    return fn();                                                     \
}

// ---------------------------------------------------------------------------
// Interposed symbols — covers every amdsmi_* call in gpuagent 1.5.1
// ---------------------------------------------------------------------------

SHIM_FN(amdsmi_init,
        (uint64_t init_flags), (init_flags))

SHIM_FN0(amdsmi_shut_down)

SHIM_FN(amdsmi_get_socket_handles,
        (uint32_t *socket_count, amdsmi_socket_handle *socket_handles),
        (socket_count, socket_handles))

SHIM_FN(amdsmi_get_processor_handles,
        (amdsmi_socket_handle socket_handle, uint32_t *processor_count,
         amdsmi_processor_handle *processor_handles),
        (socket_handle, processor_count, processor_handles))

SHIM_FN(amdsmi_get_processor_type,
        (amdsmi_processor_handle processor_handle,
         processor_type_t *processor_type),
        (processor_handle, processor_type))

SHIM_FN(amdsmi_get_gpu_asic_info,
        (amdsmi_processor_handle processor_handle, amdsmi_asic_info_t *info),
        (processor_handle, info))

SHIM_FN(amdsmi_get_gpu_board_info,
        (amdsmi_processor_handle processor_handle, amdsmi_board_info_t *info),
        (processor_handle, info))

SHIM_FN(amdsmi_get_gpu_vbios_info,
        (amdsmi_processor_handle processor_handle, amdsmi_vbios_info_t *info),
        (processor_handle, info))

SHIM_FN(amdsmi_get_gpu_driver_info,
        (amdsmi_processor_handle processor_handle, amdsmi_driver_info_t *info),
        (processor_handle, info))

SHIM_FN(amdsmi_get_gpu_enumeration_info,
        (amdsmi_processor_handle processor_handle,
         amdsmi_enumeration_info_t *info),
        (processor_handle, info))

SHIM_FN(amdsmi_get_gpu_device_bdf,
        (amdsmi_processor_handle processor_handle, amdsmi_bdf_t *bdf),
        (processor_handle, bdf))

SHIM_FN(amdsmi_get_gpu_device_uuid,
        (amdsmi_processor_handle processor_handle, unsigned int *uuid_length,
         char *uuid),
        (processor_handle, uuid_length, uuid))

SHIM_FN(amdsmi_get_gpu_bdf_id,
        (amdsmi_processor_handle processor_handle, uint64_t *bdfid),
        (processor_handle, bdfid))

SHIM_FN(amdsmi_get_gpu_activity,
        (amdsmi_processor_handle processor_handle,
         amdsmi_engine_usage_t *engine_usage),
        (processor_handle, engine_usage))

SHIM_FN(amdsmi_get_gpu_memory_total,
        (amdsmi_processor_handle processor_handle,
         amdsmi_memory_type_t mem_type, uint64_t *total),
        (processor_handle, mem_type, total))

SHIM_FN(amdsmi_get_gpu_memory_usage,
        (amdsmi_processor_handle processor_handle,
         amdsmi_memory_type_t mem_type, uint64_t *used),
        (processor_handle, mem_type, used))

SHIM_FN(amdsmi_get_gpu_vram_info,
        (amdsmi_processor_handle processor_handle, amdsmi_vram_info_t *info),
        (processor_handle, info))

SHIM_FN(amdsmi_get_gpu_vram_vendor,
        (amdsmi_processor_handle processor_handle, char *brand, uint32_t len),
        (processor_handle, brand, len))

SHIM_FN(amdsmi_get_power_info,
        (amdsmi_processor_handle processor_handle, amdsmi_power_info_t *info),
        (processor_handle, info))

SHIM_FN(amdsmi_get_power_cap_info,
        (amdsmi_processor_handle processor_handle, uint32_t sensor_ind,
         amdsmi_power_cap_info_t *info),
        (processor_handle, sensor_ind, info))

SHIM_FN(amdsmi_get_supported_power_cap,
        (amdsmi_processor_handle processor_handle, uint32_t *sensor_count,
         uint32_t *sensor_inds, amdsmi_power_cap_type_t *sensor_types),
        (processor_handle, sensor_count, sensor_inds, sensor_types))

SHIM_FN(amdsmi_set_power_cap,
        (amdsmi_processor_handle processor_handle, uint32_t sensor_ind,
         uint64_t cap),
        (processor_handle, sensor_ind, cap))

SHIM_FN(amdsmi_get_temp_metric,
        (amdsmi_processor_handle processor_handle,
         amdsmi_temperature_type_t sensor_type,
         amdsmi_temperature_metric_t metric, int64_t *temperature),
        (processor_handle, sensor_type, metric, temperature))

SHIM_FN(amdsmi_get_gpu_metrics_info,
        (amdsmi_processor_handle processor_handle,
         amdsmi_gpu_metrics_t *pgpu_metrics),
        (processor_handle, pgpu_metrics))

SHIM_FN(amdsmi_get_pcie_info,
        (amdsmi_processor_handle processor_handle, amdsmi_pcie_info_t *info),
        (processor_handle, info))

SHIM_FN(amdsmi_get_gpu_pci_throughput,
        (amdsmi_processor_handle processor_handle, uint64_t *sent,
         uint64_t *received, uint64_t *max_pkt_sz),
        (processor_handle, sent, received, max_pkt_sz))

SHIM_FN(amdsmi_get_gpu_perf_level,
        (amdsmi_processor_handle processor_handle,
         amdsmi_dev_perf_level_t *perf),
        (processor_handle, perf))

SHIM_FN(amdsmi_set_gpu_perf_level,
        (amdsmi_processor_handle processor_handle,
         amdsmi_dev_perf_level_t perf_lvl),
        (processor_handle, perf_lvl))

SHIM_FN(amdsmi_get_gpu_overdrive_level,
        (amdsmi_processor_handle processor_handle, uint32_t *od),
        (processor_handle, od))

SHIM_FN(amdsmi_set_gpu_overdrive_level,
        (amdsmi_processor_handle processor_handle, uint32_t od),
        (processor_handle, od))

SHIM_FN(amdsmi_get_gpu_od_volt_info,
        (amdsmi_processor_handle processor_handle,
         amdsmi_od_volt_freq_data_t *odv),
        (processor_handle, odv))

SHIM_FN(amdsmi_get_clk_freq,
        (amdsmi_processor_handle processor_handle, amdsmi_clk_type_t clk_type,
         amdsmi_frequencies_t *f),
        (processor_handle, clk_type, f))

SHIM_FN(amdsmi_get_clock_info,
        (amdsmi_processor_handle processor_handle, amdsmi_clk_type_t clk_type,
         amdsmi_clk_info_t *info),
        (processor_handle, clk_type, info))

SHIM_FN(amdsmi_get_gpu_ecc_count,
        (amdsmi_processor_handle processor_handle, amdsmi_gpu_block_t block,
         amdsmi_error_count_t *ec),
        (processor_handle, block, ec))

SHIM_FN(amdsmi_get_gpu_bad_page_info,
        (amdsmi_processor_handle processor_handle, uint32_t *num_pages,
         amdsmi_retired_page_record_t *info),
        (processor_handle, num_pages, info))

SHIM_FN(amdsmi_get_fw_info,
        (amdsmi_processor_handle processor_handle, amdsmi_fw_info_t *info),
        (processor_handle, info))

SHIM_FN(amdsmi_get_gpu_kfd_info,
        (amdsmi_processor_handle processor_handle, amdsmi_kfd_info_t *info),
        (processor_handle, info))

SHIM_FN(amdsmi_get_gpu_process_list,
        (amdsmi_processor_handle processor_handle, uint32_t *max_processes,
         amdsmi_proc_info_t *list),
        (processor_handle, max_processes, list))

SHIM_FN(amdsmi_get_energy_count,
        (amdsmi_processor_handle processor_handle, uint64_t *power,
         float *counter_resolution, uint64_t *timestamp),
        (processor_handle, power, counter_resolution, timestamp))

SHIM_FN(amdsmi_get_gpu_memory_partition,
        (amdsmi_processor_handle processor_handle,
         char *memory_partition, uint32_t len),
        (processor_handle, memory_partition, len))

SHIM_FN(amdsmi_set_gpu_memory_partition,
        (amdsmi_processor_handle processor_handle,
         amdsmi_memory_partition_type_t memory_partition),
        (processor_handle, memory_partition))

SHIM_FN(amdsmi_get_gpu_virtualization_mode,
        (amdsmi_processor_handle processor_handle,
         amdsmi_virtualization_mode_t *mode),
        (processor_handle, mode))

SHIM_FN(amdsmi_get_gpu_accelerator_partition_profile,
        (amdsmi_processor_handle processor_handle,
         amdsmi_accelerator_partition_profile_t *profile, uint32_t *id),
        (processor_handle, profile, id))

SHIM_FN(amdsmi_set_gpu_compute_partition,
        (amdsmi_processor_handle processor_handle,
         amdsmi_compute_partition_type_t compute_partition),
        (processor_handle, compute_partition))

SHIM_FN(amdsmi_get_violation_status,
        (amdsmi_processor_handle processor_handle,
         amdsmi_violation_status_t *violation_status),
        (processor_handle, violation_status))

SHIM_FN(amdsmi_get_gpu_partition_metrics_info,
        (amdsmi_processor_handle processor_handle,
         amdsmi_gpu_metrics_t *pgpu_metrics),
        (processor_handle, pgpu_metrics))

SHIM_FN(amdsmi_get_gpu_cper_entries,
        (amdsmi_processor_handle processor_handle, uint32_t severity_mask,
         char *cper_data, uint64_t *buf_size, amdsmi_cper_hdr_t **cper_hdrs,
         uint64_t *entry_count, uint64_t *cursor),
        (processor_handle, severity_mask, cper_data, buf_size, cper_hdrs,
         entry_count, cursor))

SHIM_FN(amdsmi_get_afids_from_cper,
        (char *cper_buffer, uint32_t buf_size, uint64_t *afids,
         uint32_t *num_afids),
        (cper_buffer, buf_size, afids, num_afids))

SHIM_FN(amdsmi_topo_get_link_type,
        (amdsmi_processor_handle processor_handle_src,
         amdsmi_processor_handle processor_handle_dst,
         uint64_t *hops, amdsmi_link_type_t *type),
        (processor_handle_src, processor_handle_dst, hops, type))

SHIM_FN(amdsmi_topo_get_link_weight,
        (amdsmi_processor_handle processor_handle_src,
         amdsmi_processor_handle processor_handle_dst, uint64_t *weight),
        (processor_handle_src, processor_handle_dst, weight))

SHIM_FN(amdsmi_gpu_xgmi_error_status,
        (amdsmi_processor_handle processor_handle,
         amdsmi_xgmi_status_t *status),
        (processor_handle, status))

SHIM_FN(amdsmi_reset_gpu_xgmi_error,
        (amdsmi_processor_handle processor_handle),
        (processor_handle))

SHIM_FN(amdsmi_get_gpu_available_counters,
        (amdsmi_processor_handle processor_handle,
         amdsmi_event_group_t grp, uint32_t *available),
        (processor_handle, grp, available))

SHIM_FN(amdsmi_gpu_create_counter,
        (amdsmi_processor_handle processor_handle,
         amdsmi_event_type_t type, amdsmi_event_handle_t *evnt_handle),
        (processor_handle, type, evnt_handle))

SHIM_FN(amdsmi_gpu_control_counter,
        (amdsmi_event_handle_t evt_handle, amdsmi_counter_command_t cmd,
         void *cmd_args),
        (evt_handle, cmd, cmd_args))

SHIM_FN(amdsmi_gpu_read_counter,
        (amdsmi_event_handle_t evt_handle, amdsmi_counter_value_t *value),
        (evt_handle, value))

SHIM_FN(amdsmi_gpu_counter_group_supported,
        (amdsmi_processor_handle processor_handle,
         amdsmi_event_group_t group),
        (processor_handle, group))

SHIM_FN(amdsmi_init_gpu_event_notification,
        (amdsmi_processor_handle processor_handle),
        (processor_handle))

SHIM_FN(amdsmi_set_gpu_event_notification_mask,
        (amdsmi_processor_handle processor_handle, uint64_t mask),
        (processor_handle, mask))

SHIM_FN(amdsmi_get_gpu_event_notification,
        (int timeout_ms, uint32_t *num_elem,
         amdsmi_evt_notification_data_t *data),
        (timeout_ms, num_elem, data))

SHIM_FN(amdsmi_stop_gpu_event_notification,
        (amdsmi_processor_handle processor_handle),
        (processor_handle))

SHIM_FN(amdsmi_reset_gpu,
        (amdsmi_processor_handle processor_handle),
        (processor_handle))

SHIM_FN(amdsmi_reset_gpu_fan,
        (amdsmi_processor_handle processor_handle, uint32_t sensor_ind),
        (processor_handle, sensor_ind))

SHIM_FN(amdsmi_set_gpu_fan_speed,
        (amdsmi_processor_handle processor_handle, uint32_t sensor_ind,
         uint64_t speed),
        (processor_handle, sensor_ind, speed))

SHIM_FN(amdsmi_set_gpu_clk_limit,
        (amdsmi_processor_handle processor_handle, amdsmi_clk_type_t clk_type,
         amdsmi_clk_limit_type_t limit_type, uint64_t clk_value),
        (processor_handle, clk_type, limit_type, clk_value))

SHIM_FN(amdsmi_set_gpu_power_profile,
        (amdsmi_processor_handle processor_handle, uint32_t reserved,
         amdsmi_power_profile_preset_masks_t profile),
        (processor_handle, reserved, profile))
