#
# Copyright(C) Advanced Micro Devices, Inc. All rights reserved.
#
# You may not use this software and documentation (if any) (collectively,
# the "Materials") except in compliance with the terms and conditions of
# the Software License Agreement included with the Materials or otherwise as
# set forth in writing and signed by you and an authorized signatory of AMD.
# If you do not have a copy of the Software License Agreement, contact your
# AMD representative for a copy.
#
# You agree that you will not reverse engineer or decompile the Materials,
# in whole or in part, except as allowed by applicable law.
#
# THE MATERIALS ARE DISTRIBUTED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OR
# REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

# Image name for the Docker container
IMAGE_NAME := gpuagent-go-test-env
GRPC_BASE_URL ?= "127.0.0.1"

HOST_MOUNT_DIR := $(PWD)/../../../..
PROTOS_DIR :=./sw/nic/gpuagent/protos

# Container mount point
CONTAINER_MOUNT_POINT := /gpu-agent

DOCKERFILE := Dockerfile

# Default target
all: test clean-image

# Build the Docker image
build-image:
	docker build -t $(IMAGE_NAME) .

# Run the tests in a container
test: build-image
	docker run --rm \
	-e GRPC_BASE_URL=$(GRPC_BASE_URL) \
	-v $(HOST_MOUNT_DIR):$(CONTAINER_MOUNT_POINT) $(IMAGE_NAME) \
	sh -c "make -C $(CONTAINER_MOUNT_POINT)/sw/nic/gpuagent/protos && go test -v $(CONTAINER_MOUNT_POINT)/sw/nic/gpuagent/tests/ && rm -rf $(CONTAINER_MOUNT_POINT)/sw/nic/gpuagent/gen"

# Clean up the Docker image
clean-image:
	docker rmi -f $(IMAGE_NAME)

.PHONY: all build-image test clean-image
