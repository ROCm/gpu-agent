CUR_DIR = $(PWD)
CUR_USER:=$(shell whoami)
CUR_TIME:=$(shell date +%Y-%m-%d_%H.%M.%S)
GPUAGENT_BLD_CONTAINER_IMAGE ?= gpuagent-builder-rhel:9
GPUAGENT_BLD_CONTAINER_IMAGE_UBUNTU ?= gpuagent-bldr-ubuntu:22.04
CONTAINER_NAME := gpuagent-ctr-${CUR_USER}_${CUR_TIME}
CONTAINER_WORKDIR := /usr/src/github.com/ROCm/gpu-agent
BUILD_DATE ?= $(shell date   +%Y-%m-%dT%H:%M:%S%z)
GIT_COMMIT ?= $(shell git rev-list -1 HEAD --abbrev-commit)
BUILD_BASE_IMAGE ?= registry.access.redhat.com/ubi9/ubi:9.4

export BUILD_BASE_IMAGE
export GPUAGENT_BLD_CONTAINER_IMAGE
export GPUAGENT_BLD_CONTAINER_IMAGE_UBUNTU

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
		bash -c " cd $(CONTAINER_WORKDIR) && source ~/.bashrc && make gopkglist && make -C sw/nic/gpuagent all"

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

.PHONY: build-container
build-container:
	${MAKE} -C tools/build-container
