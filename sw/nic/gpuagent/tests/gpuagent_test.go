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

package main

import (
	"context"
	"fmt"
	"os"
	"strings"
	"testing"

	"github.com/ROCm/gpu-agent/sw/nic/gpuagent/cli/utils"
	amdgpu "github.com/ROCm/gpu-agent/sw/nic/gpuagent/gen/go"
	"google.golang.org/grpc"
)

var (
	grpcBaseURL   = os.Getenv("GRPC_BASE_URL")
	gpuSvcClient  amdgpu.GPUSvcClient
	topoSvcClient amdgpu.TopoSvcClient
	ctxt          context.Context
	cancel        context.CancelFunc
	conn          *grpc.ClientConn
)

func Assert(t *testing.T, b bool, errString string) {
	if !b {
		t.Fatalf(errString)
	}
}

func getGpu(t *testing.T, gpuID []byte) []*amdgpu.GPU {
	respMsg := &amdgpu.GPUGetResponse{}
	req := &amdgpu.GPUGetRequest{
		Id: [][]byte{},
	}

	if gpuID != nil {
		req.Id = append(req.Id, gpuID)
	}

	respMsg, err := gpuSvcClient.GPUGet(ctxt, req)
	Assert(t, err == nil, fmt.Sprintf("Failed to get GPUs err: %v", err))
	Assert(t, respMsg.ApiStatus == amdgpu.ApiStatus_API_STATUS_OK, fmt.Sprintf("Operation failed with %v error", respMsg.ApiStatus))

	var response []*amdgpu.GPU
	for _, resp := range respMsg.Response {
		response = append(response, resp)
	}

	return response
}

func TestMain(m *testing.M) {
	fmt.Printf("grpcbaseurl: %v\n", grpcBaseURL)
	fmt.Println("Running Unit test cases for gpuagent")
	var err error
	conn, ctxt, cancel, err = utils.CreateNewAGAGRPClient(grpcBaseURL)

	if err != nil {
		fmt.Println("Could not connect to the GPU agent, is agent running?")
		os.Exit(1)
	}
	defer conn.Close()
	defer cancel()

	gpuSvcClient = amdgpu.NewGPUSvcClient(conn)
	topoSvcClient = amdgpu.NewTopoSvcClient(conn)

	exitCode := m.Run()
	os.Exit(exitCode)
}

// Tests Get ALL GPUs and get GPU by ID
func TestGetGpu(t *testing.T) {
	gpus := getGpu(t, nil)

	// at least on GPU has to be returned
	Assert(t, len(gpus) != 0, fmt.Sprintf("No GPUs returned"))

	fmt.Printf("TestGetGpusAll: No of GPUs returned: %d\n", len(gpus))

	//verify vendor is AMD
	for _, gpu := range gpus {
		Assert(t, gpu.Status.CardVendor == "AMD", fmt.Sprintf("Expected card vendor to be AMD, got: %v", gpu.Status.CardVendor))
	}
	//fmt.Println("PASS: Test Get All GPUs")

	//fmt.Println("Running: Test Get GPU by ID")

	gpuID := gpus[0].Spec.Id
	gpus = getGpu(t, gpuID)

	// at least on GPU has to be returned
	Assert(t, len(gpus) == 1, fmt.Sprintf("No GPUs returned for ID: %v", utils.IdToStr(gpuID)))

	//verify vendor is AMD
	Assert(t, gpus[0].Status.CardVendor == "AMD", fmt.Sprintf("Expected card vendor to be AMD, got: %v", gpus[0].Status.CardVendor))

	// verify the ID
	Assert(t, strings.Compare(utils.IdToStr(gpuID), utils.IdToStr(gpus[0].Spec.Id)) == 0, fmt.Sprintf("Expected gpu ID: %v, got: %v", utils.IdToStr(gpuID), utils.IdToStr(gpus[0].Spec.Id)))
}

func TestGpuAdminStUpdate(t *testing.T) {
	gpus := getGpu(t, nil)
	Assert(t, len(gpus) != 0, fmt.Sprintf("No GPUs returned"))

	gpuSpec := gpus[0].GetSpec()

	updateSpec := *gpuSpec

	updateSpec.AdminState = amdgpu.GPUAdminState_GPU_ADMIN_STATE_UP
	if gpuSpec.AdminState == amdgpu.GPUAdminState_GPU_ADMIN_STATE_UP {
		updateSpec.AdminState = amdgpu.GPUAdminState_GPU_ADMIN_STATE_DOWN
	}

	reqMsg := &amdgpu.GPUUpdateRequest{
		Spec: []*amdgpu.GPUSpec{
			&updateSpec,
		},
	}
	// GPU agent call
	updateRespMsg, err := gpuSvcClient.GPUUpdate(ctxt, reqMsg)

	Assert(t, err == nil, fmt.Sprintf("Updating GPU failed, err %v", err))
	Assert(t, updateRespMsg.ApiStatus == amdgpu.ApiStatus_API_STATUS_OPERATION_NOT_SUPPORTED, fmt.Sprintf("Operation failed with error %v, error code %v",
		updateRespMsg.ApiStatus, updateRespMsg.ErrorCode))

	// GET to verify the admin-state
	retGpu := getGpu(t, gpus[0].Spec.Id)

	Assert(t, retGpu[0].Spec.AdminState != updateSpec.AdminState, fmt.Sprintf("admin-state got updated"))
}

func TestGpuPerfLevelUpdate(t *testing.T) {
	gpus := getGpu(t, nil)
	Assert(t, len(gpus) != 0, fmt.Sprintf("No GPUs returned"))

	getNewPerfLevel := func(perf amdgpu.GPUPerformanceLevel) amdgpu.GPUPerformanceLevel {
		for l := amdgpu.GPUPerformanceLevel_GPU_PERF_LEVEL_NONE; l <= amdgpu.GPUPerformanceLevel_GPU_PERF_LEVEL_MANUAL; l++ {
			if perf != l {
				return l
			}
		}
		return amdgpu.GPUPerformanceLevel_GPU_PERF_LEVEL_NONE
	}
	gpuSpec := gpus[0].GetSpec()

	updateSpec := *gpuSpec

	origPerfLevel := gpuSpec.PerformanceLevel
	updateSpec.PerformanceLevel = getNewPerfLevel(origPerfLevel)

	reqMsg := &amdgpu.GPUUpdateRequest{
		Spec: []*amdgpu.GPUSpec{
			&updateSpec,
		},
	}
	// GPU agent call
	updateRespMsg, err := gpuSvcClient.GPUUpdate(ctxt, reqMsg)
	Assert(t, err == nil, fmt.Sprintf("Updating GPU failed, err %v", err))
	Assert(t, updateRespMsg.ApiStatus == amdgpu.ApiStatus_API_STATUS_OPERATION_NOT_SUPPORTED, fmt.Sprintf("Operation failed with error %v, error code %v",
		updateRespMsg.ApiStatus, updateRespMsg.ErrorCode))

	// GET to verify the perf-level
	retGpu := getGpu(t, gpus[0].Spec.Id)

	Assert(t, retGpu[0].Spec.PerformanceLevel != updateSpec.PerformanceLevel, fmt.Sprintf("perf-level got updated"))
}

func TestGpuOverdriveLevelUpdate(t *testing.T) {
	gpus := getGpu(t, nil)
	Assert(t, len(gpus) != 0, fmt.Sprintf("No GPUs returned"))

	gpuSpec := gpus[0].GetSpec()

	updateSpec := *gpuSpec

	origOverdriveLevel := gpuSpec.OverDriveLevel
	updateSpec.OverDriveLevel = 13

	reqMsg := &amdgpu.GPUUpdateRequest{
		Spec: []*amdgpu.GPUSpec{
			&updateSpec,
		},
	}
	// GPU agent call
	updateRespMsg, err := gpuSvcClient.GPUUpdate(ctxt, reqMsg)
	Assert(t, err == nil, fmt.Sprintf("Updating GPU failed, err %v", err))
	Assert(t, updateRespMsg.ApiStatus == amdgpu.ApiStatus_API_STATUS_OPERATION_NOT_SUPPORTED, fmt.Sprintf("Operation failed with error %v, error code %v",
		updateRespMsg.ApiStatus, updateRespMsg.ErrorCode))

	// GET to verify the overdrive-level
	retGpu := getGpu(t, gpus[0].Spec.Id)

	Assert(t, retGpu[0].Spec.OverDriveLevel == origOverdriveLevel, fmt.Sprintf("overdrive-level got updated"))
}

func TestGpuDeviceTopology(t *testing.T) {
	req := &amdgpu.DeviceTopologyGetRequest{}
	// GPU agent call
	resp, err := topoSvcClient.DeviceTopologyGet(ctxt, req)
	Assert(t, err == nil, fmt.Sprintf("Device topology get failed, err %v", err))

	Assert(t, resp.ApiStatus == amdgpu.ApiStatus_API_STATUS_OK, fmt.Sprintf("Operation failed with error %v, error code %v",
		resp.ApiStatus, resp.ErrorCode))

	Assert(t, len(resp.GetDeviceTopology()) != 0, fmt.Sprintf("Get device topology response is empty"))
}

func TestGpuReset(t *testing.T) {
	gpus := getGpu(t, nil)
	Assert(t, len(gpus) != 0, fmt.Sprintf("No GPUs returned"))

	reqMsg := &amdgpu.GPUResetRequest{}
	reqMsg.Id = append(reqMsg.Id, gpus[0].Spec.Id)
	reqMsg.Reset_ = &amdgpu.GPUResetRequest_ResetFans{
		ResetFans: true,
	}

	// GPU agent call
	updateRespMsg, err := gpuSvcClient.GPUReset(ctxt, reqMsg)
	Assert(t, err == nil, fmt.Sprintf("GPU Reset failed, err %v", err))

	Assert(t, updateRespMsg.ApiStatus == amdgpu.ApiStatus_API_STATUS_OPERATION_NOT_SUPPORTED, fmt.Sprintf("Operation failed with error %v, error code %v",
		updateRespMsg.ApiStatus, updateRespMsg.ErrorCode))
}
