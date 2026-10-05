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
.PHONY: gpuagent_wsl
gpuagent_wsl:
	${MAKE} -j$(GPUAGENT_JOBS) -C sw/nic/gpuagent gpuagent_wsl

# wsl-shim: LD_PRELOAD shim alternative — redirects amdsmi calls to the
# WSL libamd_smi at runtime; useful when gpuagent-wsl cannot be used.
.PHONY: wsl-shim
wsl-shim:
	${MAKE} -C wsl-shim

.PHONY: build-container
build-container:
	${MAKE} -C tools/build-container
