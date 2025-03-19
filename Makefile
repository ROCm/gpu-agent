-include dev.env

DOCKER_REGISTRY ?= docker.io/rocm
BUILD_BASE_IMAGE ?= ubuntu:22.04

TOP_DIR := $(PWD)

export BUILD_BASE_IMAGE
export TOP_DIR

# must build docker-build once in workspace
.PHONY: docker-build
docker-build:
	${MAKE} -C sw/nic/gpuagent docker-build

.PHONY: docker-compile
docker-compile:
	${MAKE} -C sw/nic/gpuagent docker-compile

.PHONY: docker-shell
docker-shell:
	${MAKE} -C sw/nic/gpuagent docker-shell

.PHONY: docker-clean
docker-clean:
	${MAKE} -C sw/nic/gpuagent clean
