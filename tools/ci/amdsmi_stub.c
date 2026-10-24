/*
 * amdsmi_stub.c — Minimal stub for CI compile+link checks of gpuagent-wsl.
 *
 * The gpuagent_wsl_docker Makefile target uses this file to synthesise a
 * libamd_smi.so that satisfies the linker without requiring rocdxg-amd-smi-lib
 * on the build host.  Every function returns AMDSMI_STATUS_NOT_SUPPORTED (2)
 * so the resulting binary will not produce real data; the target is
 * compile-and-link verification only.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 */

typedef unsigned long long uint64_t;
typedef unsigned int       uint32_t;
typedef unsigned short     uint16_t;
typedef long long          int64_t;
typedef int                amdsmi_status_t;
typedef void              *amdsmi_processor_handle;
typedef void              *amdsmi_socket_handle;
typedef void              *amdsmi_event_handle_t;

#define NOT_SUPPORTED 2

amdsmi_status_t amdsmi_init(uint64_t f)                                                          { return 0; }
amdsmi_status_t amdsmi_shut_down(void)                                                            { return 0; }
amdsmi_status_t amdsmi_get_socket_handles(uint32_t *c, amdsmi_socket_handle *h)                  { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_processor_handles(amdsmi_socket_handle s, uint32_t *c, amdsmi_processor_handle *h) { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_processor_type(amdsmi_processor_handle h, int *t)                     { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_asic_info(amdsmi_processor_handle h, void *i)                     { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_board_info(amdsmi_processor_handle h, void *i)                    { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_vbios_info(amdsmi_processor_handle h, void *i)                    { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_driver_info(amdsmi_processor_handle h, void *i)                   { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_enumeration_info(amdsmi_processor_handle h, void *i)              { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_device_bdf(amdsmi_processor_handle h, void *b)                    { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_device_uuid(amdsmi_processor_handle h, uint32_t *l, char *u)      { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_bdf_id(amdsmi_processor_handle h, uint64_t *b)                    { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_activity(amdsmi_processor_handle h, void *e)                      { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_memory_total(amdsmi_processor_handle h, int t, uint64_t *v)       { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_memory_usage(amdsmi_processor_handle h, int t, uint64_t *v)       { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_vram_info(amdsmi_processor_handle h, void *i)                     { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_vram_vendor(amdsmi_processor_handle h, char *b, uint32_t l)       { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_power_info(amdsmi_processor_handle h, void *i)                        { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_power_cap_info(amdsmi_processor_handle h, uint32_t s, void *i)        { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_supported_power_cap(amdsmi_processor_handle h, uint32_t *c, uint32_t *i, void *t) { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_set_power_cap(amdsmi_processor_handle h, uint32_t s, uint64_t c)          { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_temp_metric(amdsmi_processor_handle h, int s, int m, int64_t *t)      { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_metrics_info(amdsmi_processor_handle h, void *m)                  { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_pcie_info(amdsmi_processor_handle h, void *i)                         { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_pci_throughput(amdsmi_processor_handle h, uint64_t *s, uint64_t *r, uint64_t *m) { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_pci_replay_counter(amdsmi_processor_handle h, uint64_t *c)        { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_perf_level(amdsmi_processor_handle h, int *p)                     { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_set_gpu_perf_level(amdsmi_processor_handle h, int p)                      { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_overdrive_level(amdsmi_processor_handle h, uint32_t *o)           { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_set_gpu_overdrive_level(amdsmi_processor_handle h, uint32_t o)            { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_od_volt_info(amdsmi_processor_handle h, void *o)                  { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_clk_freq(amdsmi_processor_handle h, int t, void *f)                   { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_clock_info(amdsmi_processor_handle h, int t, void *i)                 { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_ecc_count(amdsmi_processor_handle h, int b, void *e)              { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_bad_page_info(amdsmi_processor_handle h, uint32_t *n, void *i)    { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_fw_info(amdsmi_processor_handle h, void *i)                           { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_kfd_info(amdsmi_processor_handle h, void *i)                      { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_process_list(amdsmi_processor_handle h, uint32_t *m, void *l)     { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_energy_count(amdsmi_processor_handle h, uint64_t *p, float *r, uint64_t *t) { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_memory_partition(amdsmi_processor_handle h, char *m, uint32_t l)  { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_set_gpu_memory_partition(amdsmi_processor_handle h, int m)                { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_virtualization_mode(amdsmi_processor_handle h, int *m)            { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_accelerator_partition_profile(amdsmi_processor_handle h, void *p, uint32_t *i) { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_set_gpu_compute_partition(amdsmi_processor_handle h, int p)               { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_violation_status(amdsmi_processor_handle h, void *v)                  { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_partition_metrics_info(amdsmi_processor_handle h, void *m)        { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_cper_entries(amdsmi_processor_handle h, uint32_t s, char *d, uint64_t *b, void **e, uint64_t *c, uint64_t *u) { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_afids_from_cper(char *b, uint32_t s, uint64_t *a, uint32_t *n)        { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_topo_get_link_type(amdsmi_processor_handle s, amdsmi_processor_handle d, uint64_t *h, int *t) { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_topo_get_link_weight(amdsmi_processor_handle s, amdsmi_processor_handle d, uint64_t *w) { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_gpu_xgmi_error_status(amdsmi_processor_handle h, int *s)                  { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_reset_gpu_xgmi_error(amdsmi_processor_handle h)                           { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_available_counters(amdsmi_processor_handle h, int g, uint32_t *a) { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_gpu_create_counter(amdsmi_processor_handle h, int t, amdsmi_event_handle_t *e) { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_gpu_control_counter(amdsmi_event_handle_t e, int c, void *a)              { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_gpu_read_counter(amdsmi_event_handle_t e, void *v)                        { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_gpu_counter_group_supported(amdsmi_processor_handle h, int g)             { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_init_gpu_event_notification(amdsmi_processor_handle h)                    { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_set_gpu_event_notification_mask(amdsmi_processor_handle h, uint64_t m)    { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_event_notification(int t, uint32_t *n, void *d)                   { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_stop_gpu_event_notification(amdsmi_processor_handle h)                    { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_reset_gpu(amdsmi_processor_handle h)                                      { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_reset_gpu_fan(amdsmi_processor_handle h, uint32_t s)                      { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_set_gpu_fan_speed(amdsmi_processor_handle h, uint32_t s, uint64_t v)      { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_set_gpu_clk_limit(amdsmi_processor_handle h, int t, int l, uint64_t v)    { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_set_gpu_power_profile(amdsmi_processor_handle h, uint32_t r, int p)       { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_volt_metric(amdsmi_processor_handle h, int s, int m, int64_t *v)  { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_fan_rpms(amdsmi_processor_handle h, uint32_t s, int64_t *v)       { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_utilization_count(amdsmi_processor_handle h, void *c, uint32_t n, uint64_t *t) { return NOT_SUPPORTED; }
amdsmi_status_t amdsmi_get_gpu_busy_percent(amdsmi_processor_handle h, uint32_t *p)              { return NOT_SUPPORTED; }
int             amdsmi_ret_to_sdk_ret(amdsmi_status_t s)                                         { return 1; }
