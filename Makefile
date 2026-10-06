CUR_DIR = $(PWD)
CUR_USER:=$(shell whoami)
CUR_TIME:=$(shell date +%Y-%m-%d_%H.%M.%S)
GPUAGENT_BLD_CONTAINER_IMAGE ?= gpuagent-builder-rhel:9
GPUAGENT_BLD_CONTAINER_IMAGE_UBUNTU ?= gpuagent-bldr-ubuntu:22.04
# inner build -j width. override: make GPUAGENT_JOBS=N gpuagent. default nproc.
GPUAGENT_JOBS ?= $(shell nproc)
CONTAINER_NAME := gpuagent-ctr-${CUR_USER}_${CUR_TIME}
CONTAINER_WORKDIR := /usr/src/github.com/ROCm/gpu-agent
BUILD_DATE ?= $(shell date   +%Y-%m-%dT%H:%M:%S%z)
GIT_COMMIT ?= $(shell git rev-list -1 HEAD --abbrev-commit)
BUILD_BASE_IMAGE ?= registry.access.redhat.com/ubi9/ubi:9.4
GO_VERSION ?= $(shell awk '$$1 == "go" { print $$2; exit }' sw/nic/gpuagent/go.mod)

export BUILD_BASE_IMAGE
export GPUAGENT_BLD_CONTAINER_IMAGE
export GPUAGENT_BLD_CONTAINER_IMAGE_UBUNTU
export GO_VERSION

.PHONY: all
all:
	${MAKE} gpuagent

.PHONY: gopkglist
gopkglist:
	go install github.com/gogo/protobuf/protoc-gen-gogofast@v1.3.2
	go install github.com/pseudomuto/protoc-gen-doc/cmd/protoc-gen-doc@v1.5.1
	go install google.golang.org/protobuf/cmd/protoc-gen-go@v1.34.2
	go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@v1.5.1

.PHONY: gpuagent
gpuagent:
	docker run --rm --privileged \
		--name ${CONTAINER_NAME} \
		--network host \
		-e "USER_NAME=$(shell whoami)" \
		-e "USER_UID=$(shell id -u)" \
		-e "USER_GID=$(shell id -g)" \
		-e "GIT_COMMIT=${GIT_COMMIT}" \
		-e "GIT_VERSION=${GIT_VERSION}" \
		-e "BUILD_DATE=${BUILD_DATE}" \
		-v $(CURDIR):$(CONTAINER_WORKDIR) \
		-w $(CONTAINER_WORKDIR) \
		${GPUAGENT_BLD_CONTAINER_IMAGE} \
		bash -c " cd $(CONTAINER_WORKDIR) && source ~/.bashrc && make gopkglist && make -j$(GPUAGENT_JOBS) -C sw/nic/gpuagent all"

.PHONY: docker-shell
docker-shell:
	docker run --rm -it --privileged \
		--name ${CONTAINER_NAME} \
		--network host \
		-e "USER_NAME=$(shell whoami)" \
		-e "USER_UID=$(shell id -u)" \
		-e "USER_GID=$(shell id -g)" \
		-e "GIT_COMMIT=${GIT_COMMIT}" \
		-e "GIT_VERSION=${GIT_VERSION}" \
		-e "BUILD_DATE=${BUILD_DATE}" \
		-v $(CURDIR):$(CONTAINER_WORKDIR) \
		-w $(CONTAINER_WORKDIR) \
		${GPUAGENT_BLD_CONTAINER_IMAGE} \
		bash -c " cd $(CONTAINER_WORKDIR) && git config --global --add safe.directory $(CONTAINER_WORKDIR) && bash"

# gpuagent_wsl: native build (no Docker) linking librocdxg libamd_smi.
# Produces gpuagent-wsl binary. Requires rocdxg-amd-smi-lib at /opt/rocm-wsl
# and WSL2 kernel. Override WSL_AMD_SMI_LIB_DIR if the library is elsewhere.
#
# Third-party libs (protobuf, grpc, abseil…) must be pre-built. Run
# `make build-libs-docker` once if they have not been built yet.
.PHONY: gpuagent_wsl
gpuagent_wsl:
	${MAKE} -j$(GPUAGENT_JOBS) -C sw/nic/gpuagent \
		ABS_DIR=$(CURDIR)/sw \
		gpuagent_wsl

# gpuagent_wsl_docker: build gpuagent-wsl inside the RHEL9 build container.
# A minimal stub libamd_smi.so is synthesised so the linker is satisfied
# without requiring rocdxg-amd-smi-lib on the container host.  The binary
# will not run (no real GPU lib), but this target verifies compile + link.
WSL_STUB_DIR := /opt/rocm-wsl/lib
.PHONY: gpuagent_wsl_docker
gpuagent_wsl_docker:
	docker run --rm --privileged \
		--name ${CONTAINER_NAME} \
		--network host \
		-e "USER_NAME=$(shell whoami)" \
		-e "USER_UID=$(shell id -u)" \
		-e "USER_GID=$(shell id -g)" \
		-e "GIT_COMMIT=${GIT_COMMIT}" \
		-e "GIT_VERSION=${GIT_VERSION}" \
		-e "BUILD_DATE=${BUILD_DATE}" \
		-v $(CURDIR):$(CONTAINER_WORKDIR) \
		-w $(CONTAINER_WORKDIR) \
		${GPUAGENT_BLD_CONTAINER_IMAGE} \
		bash -c " cd $(CONTAINER_WORKDIR) && source ~/.bashrc && \
		  mkdir -p $(WSL_STUB_DIR) && \
		  cat > /tmp/amdsmi_stub.c << 'STUB' \
typedef unsigned long long uint64_t; \
typedef unsigned int uint32_t; \
typedef unsigned short uint16_t; \
typedef int amdsmi_status_t; \
typedef void* amdsmi_processor_handle; \
typedef void* amdsmi_socket_handle; \
typedef void* amdsmi_event_handle_t; \
\
amdsmi_status_t amdsmi_init(uint64_t f){return 0;} \
amdsmi_status_t amdsmi_shut_down(void){return 0;} \
amdsmi_status_t amdsmi_get_socket_handles(uint32_t *c,amdsmi_socket_handle *h){return 2;} \
amdsmi_status_t amdsmi_get_processor_handles(amdsmi_socket_handle s,uint32_t *c,amdsmi_processor_handle *h){return 2;} \
amdsmi_status_t amdsmi_get_processor_type(amdsmi_processor_handle h,int *t){return 2;} \
amdsmi_status_t amdsmi_get_gpu_asic_info(amdsmi_processor_handle h,void *i){return 2;} \
amdsmi_status_t amdsmi_get_gpu_board_info(amdsmi_processor_handle h,void *i){return 2;} \
amdsmi_status_t amdsmi_get_gpu_vbios_info(amdsmi_processor_handle h,void *i){return 2;} \
amdsmi_status_t amdsmi_get_gpu_driver_info(amdsmi_processor_handle h,void *i){return 2;} \
amdsmi_status_t amdsmi_get_gpu_enumeration_info(amdsmi_processor_handle h,void *i){return 2;} \
amdsmi_status_t amdsmi_get_gpu_device_bdf(amdsmi_processor_handle h,void *b){return 2;} \
amdsmi_status_t amdsmi_get_gpu_device_uuid(amdsmi_processor_handle h,uint32_t *l,char *u){return 2;} \
amdsmi_status_t amdsmi_get_gpu_bdf_id(amdsmi_processor_handle h,uint64_t *b){return 2;} \
amdsmi_status_t amdsmi_get_gpu_activity(amdsmi_processor_handle h,void *e){return 2;} \
amdsmi_status_t amdsmi_get_gpu_memory_total(amdsmi_processor_handle h,int t,uint64_t *v){return 2;} \
amdsmi_status_t amdsmi_get_gpu_memory_usage(amdsmi_processor_handle h,int t,uint64_t *v){return 2;} \
amdsmi_status_t amdsmi_get_gpu_vram_info(amdsmi_processor_handle h,void *i){return 2;} \
amdsmi_status_t amdsmi_get_gpu_vram_vendor(amdsmi_processor_handle h,char *b,uint32_t l){return 2;} \
amdsmi_status_t amdsmi_get_power_info(amdsmi_processor_handle h,void *i){return 2;} \
amdsmi_status_t amdsmi_get_power_cap_info(amdsmi_processor_handle h,uint32_t s,void *i){return 2;} \
amdsmi_status_t amdsmi_get_supported_power_cap(amdsmi_processor_handle h,uint32_t *c,uint32_t *i,void *t){return 2;} \
amdsmi_status_t amdsmi_set_power_cap(amdsmi_processor_handle h,uint32_t s,uint64_t c){return 2;} \
amdsmi_status_t amdsmi_get_temp_metric(amdsmi_processor_handle h,int s,int m,long long *t){return 2;} \
amdsmi_status_t amdsmi_get_gpu_metrics_info(amdsmi_processor_handle h,void *m){return 2;} \
amdsmi_status_t amdsmi_get_pcie_info(amdsmi_processor_handle h,void *i){return 2;} \
amdsmi_status_t amdsmi_get_gpu_pci_throughput(amdsmi_processor_handle h,uint64_t *s,uint64_t *r,uint64_t *m){return 2;} \
amdsmi_status_t amdsmi_get_gpu_pci_replay_counter(amdsmi_processor_handle h,uint64_t *c){return 2;} \
amdsmi_status_t amdsmi_get_gpu_perf_level(amdsmi_processor_handle h,int *p){return 2;} \
amdsmi_status_t amdsmi_set_gpu_perf_level(amdsmi_processor_handle h,int p){return 2;} \
amdsmi_status_t amdsmi_get_gpu_overdrive_level(amdsmi_processor_handle h,uint32_t *o){return 2;} \
amdsmi_status_t amdsmi_set_gpu_overdrive_level(amdsmi_processor_handle h,uint32_t o){return 2;} \
amdsmi_status_t amdsmi_get_gpu_od_volt_info(amdsmi_processor_handle h,void *o){return 2;} \
amdsmi_status_t amdsmi_get_clk_freq(amdsmi_processor_handle h,int t,void *f){return 2;} \
amdsmi_status_t amdsmi_get_clock_info(amdsmi_processor_handle h,int t,void *i){return 2;} \
amdsmi_status_t amdsmi_get_gpu_ecc_count(amdsmi_processor_handle h,int b,void *e){return 2;} \
amdsmi_status_t amdsmi_get_gpu_bad_page_info(amdsmi_processor_handle h,uint32_t *n,void *i){return 2;} \
amdsmi_status_t amdsmi_get_fw_info(amdsmi_processor_handle h,void *i){return 2;} \
amdsmi_status_t amdsmi_get_gpu_kfd_info(amdsmi_processor_handle h,void *i){return 2;} \
amdsmi_status_t amdsmi_get_gpu_process_list(amdsmi_processor_handle h,uint32_t *m,void *l){return 2;} \
amdsmi_status_t amdsmi_get_energy_count(amdsmi_processor_handle h,uint64_t *p,float *r,uint64_t *t){return 2;} \
amdsmi_status_t amdsmi_get_gpu_memory_partition(amdsmi_processor_handle h,char *m,uint32_t l){return 2;} \
amdsmi_status_t amdsmi_set_gpu_memory_partition(amdsmi_processor_handle h,int m){return 2;} \
amdsmi_status_t amdsmi_get_gpu_virtualization_mode(amdsmi_processor_handle h,int *m){return 2;} \
amdsmi_status_t amdsmi_get_gpu_accelerator_partition_profile(amdsmi_processor_handle h,void *p,uint32_t *i){return 2;} \
amdsmi_status_t amdsmi_set_gpu_compute_partition(amdsmi_processor_handle h,int p){return 2;} \
amdsmi_status_t amdsmi_get_violation_status(amdsmi_processor_handle h,void *v){return 2;} \
amdsmi_status_t amdsmi_get_gpu_partition_metrics_info(amdsmi_processor_handle h,void *m){return 2;} \
amdsmi_status_t amdsmi_get_gpu_cper_entries(amdsmi_processor_handle h,uint32_t s,char *d,uint64_t *b,void **e,uint64_t *c,uint64_t *u){return 2;} \
amdsmi_status_t amdsmi_get_afids_from_cper(char *b,uint32_t s,uint64_t *a,uint32_t *n){return 2;} \
amdsmi_status_t amdsmi_topo_get_link_type(amdsmi_processor_handle s,amdsmi_processor_handle d,uint64_t *h,int *t){return 2;} \
amdsmi_status_t amdsmi_topo_get_link_weight(amdsmi_processor_handle s,amdsmi_processor_handle d,uint64_t *w){return 2;} \
amdsmi_status_t amdsmi_gpu_xgmi_error_status(amdsmi_processor_handle h,int *s){return 2;} \
amdsmi_status_t amdsmi_reset_gpu_xgmi_error(amdsmi_processor_handle h){return 2;} \
amdsmi_status_t amdsmi_get_gpu_available_counters(amdsmi_processor_handle h,int g,uint32_t *a){return 2;} \
amdsmi_status_t amdsmi_gpu_create_counter(amdsmi_processor_handle h,int t,amdsmi_event_handle_t *e){return 2;} \
amdsmi_status_t amdsmi_gpu_control_counter(amdsmi_event_handle_t e,int c,void *a){return 2;} \
amdsmi_status_t amdsmi_gpu_read_counter(amdsmi_event_handle_t e,void *v){return 2;} \
amdsmi_status_t amdsmi_gpu_counter_group_supported(amdsmi_processor_handle h,int g){return 2;} \
amdsmi_status_t amdsmi_init_gpu_event_notification(amdsmi_processor_handle h){return 2;} \
amdsmi_status_t amdsmi_set_gpu_event_notification_mask(amdsmi_processor_handle h,uint64_t m){return 2;} \
amdsmi_status_t amdsmi_get_gpu_event_notification(int t,uint32_t *n,void *d){return 2;} \
amdsmi_status_t amdsmi_stop_gpu_event_notification(amdsmi_processor_handle h){return 2;} \
amdsmi_status_t amdsmi_reset_gpu(amdsmi_processor_handle h){return 2;} \
amdsmi_status_t amdsmi_reset_gpu_fan(amdsmi_processor_handle h,uint32_t s){return 2;} \
amdsmi_status_t amdsmi_set_gpu_fan_speed(amdsmi_processor_handle h,uint32_t s,uint64_t v){return 2;} \
amdsmi_status_t amdsmi_set_gpu_clk_limit(amdsmi_processor_handle h,int t,int l,uint64_t v){return 2;} \
amdsmi_status_t amdsmi_set_gpu_power_profile(amdsmi_processor_handle h,uint32_t r,int p){return 2;} \
amdsmi_status_t amdsmi_get_gpu_volt_metric(amdsmi_processor_handle h,int s,int m,long long *v){return 2;} \
amdsmi_status_t amdsmi_get_gpu_fan_rpms(amdsmi_processor_handle h,uint32_t s,long long *v){return 2;} \
amdsmi_status_t amdsmi_get_utilization_count(amdsmi_processor_handle h,void *c,uint32_t n,uint64_t *t){return 2;} \
amdsmi_status_t amdsmi_get_gpu_busy_percent(amdsmi_processor_handle h,uint32_t *p){return 2;} \
int amdsmi_ret_to_sdk_ret(amdsmi_status_t s){return 1;} \
STUB\
		  gcc -shared -fPIC -o $(WSL_STUB_DIR)/libamd_smi.so /tmp/amdsmi_stub.c && \
		  ln -sf libamd_smi.so $(WSL_STUB_DIR)/libamd_smi.so.1 && \
		  make gopkglist && \
		  make -j$(GPUAGENT_JOBS) -C sw/nic/gpuagent \
		    WSL_AMD_SMI_LIB_DIR=$(WSL_STUB_DIR) gpuagent_wsl"

# build-libs-docker: compile third-party libs inside the build container so
# that `make gpuagent_wsl` (a native build) can link against them.
.PHONY: build-libs-docker
build-libs-docker:
	docker run --rm --privileged \
		--name ${CONTAINER_NAME} \
		--network host \
		-e "USER_NAME=$(shell whoami)" \
		-e "USER_UID=$(shell id -u)" \
		-e "USER_GID=$(shell id -g)" \
		-v $(CURDIR):$(CONTAINER_WORKDIR) \
		-w $(CONTAINER_WORKDIR) \
		${GPUAGENT_BLD_CONTAINER_IMAGE} \
		bash -c "make -j$(GPUAGENT_JOBS) -C sw/nic/gpuagent build-libs"

# wsl-shim: LD_PRELOAD shim alternative — redirects amdsmi calls to the
# WSL libamd_smi at runtime; useful when gpuagent-wsl cannot be used.
.PHONY: wsl-shim
wsl-shim:
	${MAKE} -C wsl-shim

.PHONY: build-container
build-container:
	${MAKE} -C tools/build-container
