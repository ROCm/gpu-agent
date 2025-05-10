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

//------------------------------------------------------------------------------
///
/// \file
/// gpctl command line interface for gpu protobufs
///
//------------------------------------------------------------------------------

package cmd

import (
	"encoding/json"
	"fmt"
	"io"
	"strconv"
	"strings"

	uuid "github.com/satori/go.uuid"
	"github.com/spf13/cobra"
	yaml "gopkg.in/yaml.v2"

	"github.com/ROCm/gpu-agent/sw/nic/gpuagent/cli/utils"
	aga "github.com/ROCm/gpu-agent/sw/nic/gpuagent/gen/go"
)

var (
	gpuID               string
	gpuAdminState       string
	overDriveLevel      uint32
	powerCap            uint64
	perfLevel           string
	gpuClkFreq          string
	memClkFreq          string
	fanSpeed            uint64
	gpuAdminStateVal    aga.GPUAdminState
	PerformanceLevelVal aga.GPUPerformanceLevel
	clockType           aga.GPUClockType
	gpuClkType          string
	gpuClkFreqLo        uint32
	gpuClkFreqHi        uint32
	memClkFreqLo        uint32
	memClkFreqHi        uint32
	printHdr            bool
)

const (
	UINT16_MAX_VAL_UINT16 uint16 = 0xffff
	UINT16_MAX_VAL_UINT32 uint32 = 0xffff
	UINT16_MAX_VAL_UINT64 uint64 = 0xffff
	UINT32_MAX_VAL_UINT32 uint32 = 0xffffffff
	UINT32_MAX_VAL_UINT64 uint64 = 0xffffffff
	UINT64_MAX_VAL        uint64 = 0xffffffffffffffff
)

var gpuShowCmd = &cobra.Command{
	Use:   "gpu",
	Short: "show GPU information",
	Long:  "show GPU information",
	RunE:  gpuShowCmdHandler,
}

var gpuAllShowCmd = &cobra.Command{
	Use:   "all",
	Short: "show all GPU object",
	Long:  "show all GPU object",
	RunE:  gpuAllShowCmdHandler,
}

var gpuPartitionsShowCmd = &cobra.Command{
	Use:   "compute-partition",
	Short: "show physical GPU's compute partitions",
	Long:  "show physical GPU's compute partitions",
	RunE:  gpuPartitionsShowCmdHandler,
}

var gpuBadPageShowCmd = &cobra.Command{
	Use:   "bad-page",
	Short: "show GPU bad page information",
	Long:  "show GPU bad page information",
	RunE:  gpuBadPageShowCmdHandler,
}

var gpuStatsShowCmd = &cobra.Command{
	Use:   "statistics",
	Short: "show GPU statistics",
	Long:  "show GPU statistics",
	RunE:  gpuStatsShowCmdHandler,
}

var gpuUpdateCmd = &cobra.Command{
	Use:     "gpu",
	Short:   "update gpu object",
	Long:    "update gpu object",
	PreRunE: gpuUpdateCmdPreRunE,
	RunE:    gpuUpdateCmdHandler,
}

var gpuResetCmd = &cobra.Command{
	Use:     "reset",
	Short:   "reset gpu object/settings",
	Long:    "reset gpu object/settings",
	PreRunE: gpuResetCmdPreRunE,
	RunE:    gpuResetCmdHandler,
}

func init() {
	ShowCmd.AddCommand(gpuShowCmd)
	gpuShowCmd.Flags().BoolP("yaml", "y", false, "Output in yaml")
	gpuShowCmd.Flags().BoolP("json", "j", false, "Output in json")
	gpuShowCmd.Flags().BoolP("status", "s", false, "Show GPU status")
	gpuShowCmd.Flags().Bool("summary", false, "Display number of objects")
	gpuShowCmd.Flags().StringVarP(&gpuID, "id", "i", "", "Specify GPU id")
	gpuShowCmd.Flags().BoolP("partitioned", "p", false,
		"Show only partitioned GPUs")

	gpuShowCmd.AddCommand(gpuAllShowCmd)
	gpuAllShowCmd.Flags().StringVarP(&gpuID, "id", "i", "", "Specify GPU id")

	gpuShowCmd.AddCommand(gpuStatsShowCmd)
	gpuStatsShowCmd.Flags().StringVarP(&gpuID, "id", "i", "", "Specify GPU id")

	gpuShowCmd.AddCommand(gpuPartitionsShowCmd)
	gpuPartitionsShowCmd.Flags().StringVarP(&gpuID, "id", "i", "", "Specify partitioned GPU's id")
	gpuPartitionsShowCmd.Flags().BoolP("yaml", "y", false, "Output in yaml")
	gpuPartitionsShowCmd.Flags().BoolP("json", "j", false, "Output in json")

	gpuShowCmd.AddCommand(gpuBadPageShowCmd)
	gpuBadPageShowCmd.Flags().StringVarP(&gpuID, "id", "i", "",
		"Specify GPU id")

	DebugUpdateCmd.AddCommand(gpuUpdateCmd)
	gpuUpdateCmd.Flags().StringVarP(&gpuID, "id", "i", "", "Specify GPU id")
	gpuUpdateCmd.Flags().StringVarP(&gpuAdminState, "admin-state", "a", "",
		"Specify admin state (up/down)")
	gpuUpdateCmd.Flags().Uint32VarP(&overDriveLevel, "overdrive-level", "o", 0,
		"Specify GPU clock overdrive level in percentage")
	gpuUpdateCmd.Flags().Uint64VarP(&powerCap, "power-cap", "p", 0,
		"Specify max package power GPU can consume (in Watts)")
	gpuUpdateCmd.Flags().StringVarP(&perfLevel, "perf-level", "l", "",
		"Specify GPU performance level (none/auto/low/high/deterministic/"+
			"memclock/sysclock/manual)")
	gpuUpdateCmd.Flags().StringVarP(&gpuClkType, "clock-type", "t", "",
		"Specify GPU clock type (memory, system, video or data)")
	gpuUpdateCmd.Flags().StringVarP(&gpuClkFreq, "clock-frequency", "c", "",
		"Specify GPU clock frequency range (lo-hi)")
	gpuUpdateCmd.Flags().Uint64VarP(&fanSpeed, "fan-speed", "s", 0,
		"Specify fan speed")
	gpuUpdateCmd.MarkFlagRequired("id")
	// TODO: RAS spec

	gpuUpdateCmd.AddCommand(gpuResetCmd)
	gpuResetCmd.Flags().StringVarP(&gpuID, "id", "i", "", "Specify GPU id")
	gpuResetCmd.Flags().Bool("clocks", false,
		"Reset clocks and overdrive to default")
	gpuResetCmd.Flags().Bool("fans", false, "Reset fans to automatic control")
	gpuResetCmd.Flags().Bool("power-profile", false,
		"Reset power profile to default")
	gpuResetCmd.Flags().Bool("power-overdrive", false,
		"Set the maximum GPU power back to the device deafult state")
	gpuResetCmd.Flags().Bool("xgmi-error", false,
		"Reset XGMI error status/count")
	gpuResetCmd.Flags().Bool("perf-determinism", false,
		"Disable performance determinism")
	gpuResetCmd.Flags().Bool("compute-partition", false,
		"Resets to boot compute partition state")
	gpuResetCmd.Flags().Bool("nps-mode", false, "Reset to boot NPS mode state")
	gpuResetCmd.MarkFlagRequired("id")
}

func printGPUPartitions(resp *aga.GPUComputePartition) {
	fmt.Printf("%-40s%-16s", utils.IdToStr(resp.GetId()),
		strings.Replace(resp.GetComputePartitionType().String(),
			"GPU_COMPUTE_PARTITION_TYPE_", "", -1))

	for i, partition := range resp.GPUPartition {
		if i != 0 {
			fmt.Printf("%-56s%-40s\n", "", utils.IdToStr(partition))
		} else {
			fmt.Printf("%-40s\n", "", utils.IdToStr(partition))
		}
	}
}

type ShadowGPUComputePartition struct {
	Id                   string
	ComputePartitionType aga.GPUComputePartitionType
	GPUPartitions        []string
}

func NewGPUComputePartition(resp *aga.GPUComputePartition) *ShadowGPUComputePartition {
	var gpuPartitions []string
	for _, child := range resp.GetGPUPartition() {
		gpuPartitions = append(gpuPartitions, utils.IdToStr(child))
	}
	return &ShadowGPUComputePartition{
		Id:                   utils.IdToStr(resp.GetId()),
		ComputePartitionType: resp.GetComputePartitionType(),
		GPUPartitions:        gpuPartitions,
	}
}

func printGPUPartitionsJson(resp *aga.GPUComputePartition) {
	partition := NewGPUComputePartition(resp)
	b, _ := json.MarshalIndent(partition, "  ", " ")
	fmt.Println(string(b))
}

func gpuPartitionsShowCmdHandler(cmd *cobra.Command, args []string) error {
	if len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	if cmd != nil {
		if cmd.Flags().Changed("id") {
			if err := utils.IsUUIDValid(gpuID); err != nil {
				return err
			}
		}
		cmd.SilenceUsage = true
	}
	respMsg := &aga.GPUComputePartitionGetResponse{}
	var req *aga.GPUComputePartitionGetRequest
	if cmd != nil && cmd.Flags().Changed("id") {
		// get specific GPU
		req = &aga.GPUComputePartitionGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(gpuID).Bytes()},
		}
	} else {
		// get all GPUs
		req = &aga.GPUComputePartitionGetRequest{
			Id: [][]byte{},
		}
	}

	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent " +
			"running?")
	}
	defer c.Close()
	defer cancel()

	client := aga.NewGPUSvcClient(c)
	respMsg, err = client.GPUComputePartitionGet(ctxt, req)
	if err != nil {
		return fmt.Errorf("Getting GPU failed, err %v", err)
	}

	if respMsg.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Operation failed with %v error", respMsg.ApiStatus)
	}

	// print GPUs
	if cmd != nil && cmd.Flags().Changed("yaml") {
		yamlArr, _ := yaml.Marshal(respMsg.Response)
		fmt.Println(string(yamlArr))
	} else if cmd != nil && cmd.Flags().Changed("json") {
		// json output requires that all GPUs are listed within [] braces
		if cmd.Flags().Changed("json") {
			fmt.Printf("[\n")
		}
		rcvdResp := false
		for _, resp := range respMsg.Response {
			if rcvdResp == true {
				// json output requires a , after each GPU
				fmt.Printf(",\n")
			}
			printGPUPartitionsJson(resp)
			rcvdResp = true
		}
		// json output requires that all GPUs are listed within [] braces
		if cmd.Flags().Changed("json") {
			fmt.Printf("\n]\n")
		}
	} else {
		hdrLine := strings.Repeat("-", 96)
		fmt.Println(hdrLine)
		fmt.Printf("%-40s%-16s%-40s\n", "PhysicalGPU", "PartitionType",
			"GPUPartitions")
		fmt.Println(hdrLine)
		for _, resp := range respMsg.Response {
			printGPUPartitions(resp)
		}
	}
	return nil
}

func gpuShowCmdHandler(cmd *cobra.Command, args []string) error {
	if len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	if cmd != nil {
		if cmd.Flags().Changed("id") {
			if err := utils.IsUUIDValid(gpuID); err != nil {
				return err
			}
		}
		cmd.SilenceUsage = true
	}
	respMsg := &aga.GPUGetResponse{}
	var req *aga.GPUGetRequest
	if cmd != nil && cmd.Flags().Changed("id") {
		// get specific GPU
		req = &aga.GPUGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(gpuID).Bytes()},
		}
	} else {
		// get all GPUs
		req = &aga.GPUGetRequest{
			Id: [][]byte{},
		}
	}

	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent " +
			"running?")
	}
	defer c.Close()
	defer cancel()

	client := aga.NewGPUSvcClient(c)
	respMsg, err = client.GPUGet(ctxt, req)
	if err != nil {
		return fmt.Errorf("Getting GPU failed, err %v", err)
	}

	if respMsg.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Operation failed with %v error", respMsg.ApiStatus)
	}

	var response []*aga.GPU
	for _, resp := range respMsg.Response {
		status := resp.GetStatus()
		if len(status.GetGPUPartition()) > 0 {
			if cmd != nil && cmd.Flags().Changed("partitioned") {
				response = append(response, resp)
			}
		} else {
			if cmd == nil || !cmd.Flags().Changed("partitioned") {
				response = append(response, resp)
			}
		}
	}

	// print GPUs
	if cmd != nil && cmd.Flags().Changed("yaml") {
		yamlArr, _ := yaml.Marshal(response)
		fmt.Println(string(yamlArr))
	} else if cmd != nil && cmd.Flags().Changed("json") {
		// json output requires that all GPUs are listed within [] braces
		if cmd.Flags().Changed("json") {
			fmt.Printf("[\n")
		}
		rcvdResp := false
		for _, resp := range response {
			if rcvdResp == true {
				// json output requires a , after each GPU
				fmt.Printf(",\n")
			}
			printGPUJson(resp)
			rcvdResp = true
		}
		// json output requires that all GPUs are listed within [] braces
		if cmd.Flags().Changed("json") {
			fmt.Printf("\n]\n")
		}
	} else if cmd != nil && cmd.Flags().Changed("summary") {
		printGPUSummary(len(response))
	} else if cmd != nil && cmd.Flags().Changed("status") {
		for _, resp := range response {
			printGPUStatus(resp, true)
		}
		printGPUSummary(len(response))
	} else {
		for _, resp := range response {
			printGPUSpec(resp, true)
		}
		printGPUSummary(len(response))
	}
	return nil
}

func printGPUBadPageHeader() {
	hdrLine := strings.Repeat("-", 80)
	fmt.Println(hdrLine)
	fmt.Printf("%-40s%-16s%-12s%-12s\n",
		"GPU", "PageAddress", "PageSize", "Status")
	fmt.Println(hdrLine)
}

func gpuBadPageShowCmdHandler(cmd *cobra.Command, args []string) error {
	if len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	if cmd != nil {
		if cmd.Flags().Changed("id") {
			if err := utils.IsUUIDValid(gpuID); err != nil {
				return err
			}
		}
		cmd.SilenceUsage = true
	}
	var req *aga.GPUBadPageGetRequest
	var rsp *aga.GPUBadPageGetResponse
	if cmd != nil && cmd.Flags().Changed("id") {
		// get for specific GPU
		req = &aga.GPUBadPageGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(gpuID).Bytes()},
		}
	} else {
		// get for all GPUs
		req = &aga.GPUBadPageGetRequest{
			Id: [][]byte{},
		}
	}

	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent running?")
	}
	defer c.Close()
	defer cancel()

	client := aga.NewDebugGPUSvcClient(c)
	stream, err := client.GPUBadPageGet(ctxt, req)
	if err != nil {
		return fmt.Errorf("Getting mapping failed, err %v", err)
	}
	firstResp := true
	currGPU := ""
	for {
		rsp, err = stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			return fmt.Errorf("GPU bad page get failure, err %v\n", err)
		}
		if rsp.ApiStatus != aga.ApiStatus_API_STATUS_OK {
			return fmt.Errorf("Operation failed with %v error", rsp.ApiStatus)
		}
		if firstResp == true {
			printGPUBadPageHeader()
			firstResp = false
		}
		// print GPU bad pages
		for _, record := range rsp.Record {
			if currGPU == utils.IdToStr(record.GetGPU()) {
				fmt.Printf("%-40s%-16x%-12d%-12s\n", "",
					record.GetPageAddress(), record.GetPageSize(),
					strings.ToLower(strings.Replace(
						record.GetPageStatus().String(),
						"GPU_PAGE_STATUS_", "", -1)))
			} else {
				currGPU = utils.IdToStr(record.GetGPU())
				fmt.Printf("%-40s%-16x%-12d%-12s\n", currGPU,
					record.GetPageAddress(), record.GetPageSize(),
					strings.ToLower(strings.Replace(
						record.GetPageStatus().String(),
						"GPU_PAGE_STATUS_", "", -1)))
			}
		}
	}
	return nil
}

func gpuStatsShowCmdHandler(cmd *cobra.Command, args []string) error {
	if len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	if cmd != nil {
		if cmd.Flags().Changed("id") {
			if err := utils.IsUUIDValid(gpuID); err != nil {
				return err
			}
		}
		cmd.SilenceUsage = true
	}
	respMsg := &aga.GPUGetResponse{}
	var req *aga.GPUGetRequest
	if cmd != nil && cmd.Flags().Changed("id") {
		// get specific GPU
		req = &aga.GPUGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(gpuID).Bytes()},
		}
	} else {
		// get all GPUs
		req = &aga.GPUGetRequest{
			Id: [][]byte{},
		}
	}

	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent running?")
	}
	defer c.Close()
	defer cancel()

	client := aga.NewGPUSvcClient(c)
	respMsg, err = client.GPUGet(ctxt, req)
	if err != nil {
		return fmt.Errorf("Getting GPU failed, err %v", err)
	}

	if respMsg.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Operation failed with %v error", respMsg.ApiStatus)
	}

	// print GPUs
	for _, resp := range respMsg.Response {
		printGPUStats(resp, true)
	}
	printGPUSummary(len(respMsg.Response))
	return nil
}

func gpuAllShowCmdHandler(cmd *cobra.Command, args []string) error {
	if len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	if cmd != nil {
		if cmd.Flags().Changed("id") {
			if err := utils.IsUUIDValid(gpuID); err != nil {
				return err
			}
		}
		cmd.SilenceUsage = true
	}
	respMsg := &aga.GPUGetResponse{}
	var req *aga.GPUGetRequest
	if cmd != nil && cmd.Flags().Changed("id") {
		// get specific GPU
		req = &aga.GPUGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(gpuID).Bytes()},
		}
	} else {
		// get all GPUs
		req = &aga.GPUGetRequest{
			Id: [][]byte{},
		}
	}

	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent " +
			"running?")
	}
	defer c.Close()
	defer cancel()

	client := aga.NewGPUSvcClient(c)
	respMsg, err = client.GPUGet(ctxt, req)
	if err != nil {
		return fmt.Errorf("Getting GPU failed, err %v", err)
	}

	if respMsg.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Operation failed with %v error", respMsg.ApiStatus)
	}

	// print GPUs
	for _, resp := range respMsg.Response {
		printGPUSpec(resp, false)
		printGPUStatus(resp, false)
		printGPUStats(resp, false)
	}
	printGPUSummary(len(respMsg.Response))
	return nil
}

func printGPUSummary(count int) {
	fmt.Printf("\nNo. of gpus : %d\n\n", count)
}

func printGPUSpec(gpu *aga.GPU, specOnly bool) {
	spec := gpu.GetSpec()
	status := gpu.GetStatus()

	fmt.Printf("%-40s : %s (%d)\n", "Id", utils.IdToStr(spec.GetId()),
		status.GetIndex())
	if spec.GetAdminState() != aga.GPUAdminState_GPU_ADMIN_STATE_NONE {
		fmt.Printf("%-40s : %s\n", "Admin state",
			strings.ToLower(strings.Replace(spec.GetAdminState().String(),
				"GPU_ADMIN_STATE_", "", -1)))
	}
	if spec.GetOverDriveLevel() != 0.0 {
		fmt.Printf("%-40s : %f\n", "Clock overdrive level",
			spec.GetOverDriveLevel())
	}
	if spec.GetGPUPowerCap() != 0 {
		fmt.Printf("%-40s : %d\n", "Power overdrive (in watts)",
			spec.GetGPUPowerCap())
	}
	if spec.GetPerformanceLevel() !=
		aga.GPUPerformanceLevel_GPU_PERF_LEVEL_NONE {
		fmt.Printf("%-40s : %s\n", "Performance level",
			strings.ToLower(strings.Replace(spec.GetPerformanceLevel().String(),
				"GPU_PERF_LEVEL_", "", -1)))
	}
	for _, clockFreq := range spec.GetClockFrequency() {
		fmt.Printf("%-40s : %s\n", "GPU clock type",
			strings.Replace(clockFreq.GetClockType().String(),
				"GPU_CLOCK_TYPE_", "", -1))
		fmt.Printf("  %-38s : %d - %d\n",
			"Frequency range (in MHz)",
			clockFreq.GetLowFrequency(),
			clockFreq.GetHighFrequency())
	}
	if spec.GetFanSpeed() != 0.0 {
		fmt.Printf("%-40s : %f\n", "Fan speed", spec.GetFanSpeed())
	}
	if spec.GetComputePartitionType() !=
		aga.GPUComputePartitionType_GPU_COMPUTE_PARTITION_TYPE_NONE {
		fmt.Printf("%-40s : %s\n", "Compute partition type",
			strings.Replace(spec.GetComputePartitionType().String(),
				"GPU_COMPUTE_PARTITION_TYPE_", "", -1))
	}
	// TODO: fill GPU RAS Spec
	if specOnly {
		fmt.Printf("\n%s\n", strings.Repeat("-", 80))
	}
}

func printPCIeStatusHdr(indent string) {
	if printHdr == false {
		fmt.Printf(indent + "PCIe status : \n")
		printHdr = true
	}
}

func printVRAMStatusHdr(indent string) {
	if printHdr == false {
		fmt.Printf(indent + "VRAM status:\n")
		printHdr = true
	}
}

func printGPUStatus(gpu *aga.GPU, statusOnly bool) {
	var indent string
	spec := gpu.GetSpec()
	status := gpu.GetStatus()

	if statusOnly {
		fmt.Printf("\n%-38s : %s (%d)\n", "GPU id", utils.IdToStr(spec.GetId()),
			status.GetIndex())
		indent = ""
	} else {
		fmt.Printf("\nStatus :\n")
		indent = "  "
	}
	fmt.Printf(indent+"%-38s : %d\n", "Index", status.GetIndex())
	fmt.Printf(indent+"%-38s : 0x%x\n", "GPU handle", status.GetGPUHandle())
	if status.GetSerialNum() != "" {
		fmt.Printf(indent+"%-38s : %s\n", "Serial number",
			status.GetSerialNum())
	}
	if status.GetCardSeries() != "" {
		fmt.Printf(indent+"%-38s : %s\n", "Card series", status.GetCardSeries())
	}
	if status.GetCardModel() != "" {
		fmt.Printf(indent+"%-38s : %s\n", "Card model", status.GetCardModel())
	}
	if status.GetCardVendor() != "" {
		fmt.Printf(indent+"%-38s : %s\n", "Card vendor", status.GetCardVendor())
	}
	if status.GetCardSKU() != "" {
		fmt.Printf(indent+"%-38s : %s\n", "Card SKU", status.GetCardSKU())
	}
	if status.GetDriverVersion() != "" {
		fmt.Printf(indent+"%-38s : %s\n", "Driver version",
			status.GetDriverVersion())
	}
	if status.GetVBIOSPartNumber() != "" {
		fmt.Printf(indent+"%-38s : %s\n", "VBIOS part number",
			status.GetVBIOSPartNumber())
	}
	if status.GetVBIOSVersion() != "" {
		fmt.Printf(indent+"%-38s : %s\n", "VBIOS version",
			status.GetVBIOSVersion())
	}
	switch spec.GetComputePartitionType() {
	case aga.GPUComputePartitionType_GPU_COMPUTE_PARTITION_TYPE_NONE:
		break
	default:
		fmt.Printf(indent+"%-38s : %d\n", "Partition ID",
			status.GetPartitionId())
	}
	fwVer := status.GetFirmwareVersion()
	if len(fwVer) != 0 {
		fmt.Printf(indent + "Firmware versions:\n")
		for i := 0; i < len(fwVer); i++ {
			fwVerStr := fmt.Sprintf("%s %s", fwVer[i].GetFirmware(),
				"firmware version")
			fmt.Printf(indent+"  %-36s : %s\n", fwVerStr, fwVer[i].GetVersion())
		}
	}
	if status.GetMemoryVendor() != "" {
		fmt.Printf(indent+"%-38s : %s\n", "Memory vendor",
			status.GetMemoryVendor())
	}
	if status.GetOperStatus() != aga.GPUOperStatus_GPU_OPER_STATUS_NONE {
		fmt.Printf(indent+"%-38s : %s\n", "Operational status",
			strings.ToLower(strings.Replace(status.GetOperStatus().String(),
				"GPU_OPER_STATUS_", "", -1)))
	}
	clkStr := ""
	idxr := 0
	for _, clkStatus := range status.GetClockStatus() {
		curClkStr := strings.Replace(clkStatus.GetType().String(),
			"GPU_CLOCK_TYPE_", "", -1)
		if clkStr != curClkStr {
			clkStr = curClkStr
			idxr = 0
		}
		if clkStatus.GetType() != aga.GPUClockType_GPU_CLOCK_TYPE_NONE &&
			clkStatus.GetFrequency() != 0 &&
			clkStatus.GetFrequency() != UINT16_MAX_VAL_UINT32 {
			fmt.Printf(indent+"%-38s : %s_%d\n", "GPU clock type", clkStr, idxr)
			fmt.Printf(indent+"  %-36s : %d\n", "Frequency (in MHz)",
				clkStatus.GetFrequency())
			fmt.Printf(indent+"  %-36s : %d - %d\n", "Frequency range (in MHz)",
				clkStatus.GetLowFrequency(), clkStatus.GetHighFrequency())
			if clkStatus.GetLocked() {
				fmt.Printf(indent+"  %-36s : true\n", "Clock locked")
			}
			if clkStatus.GetDeepSleep() {
				fmt.Printf(indent+"  %-36s : true\n", "Deep sleep enabled")
			}
		}
		idxr++
	}
	kfdPids := status.GetKFDProcessId()
	if len(kfdPids) != 0 {
		kfdPidStr := fmt.Sprintf("%-38s : ", "KFD process id using GPU")
		for i := 0; i < len(kfdPids); i++ {
			fmt.Printf(indent+"%-41s%d\n", kfdPidStr, kfdPids[i])
			kfdPidStr = ""
		}
	}
	// TODO: fill GPU RAS status
	xgmiStatus := status.GetXGMIStatus()
	if xgmiStatus.GetErrorStatus() !=
		aga.GPUXGMIErrorStatus_GPU_XGMI_STATUS_NONE {
		fmt.Printf(indent+"%-38s : %s\n", "XGMI error status",
			strings.ToLower(strings.Replace(
				xgmiStatus.GetErrorStatus().String(), "GPU_XGMI_STATUS_",
				"", -1)))
	}
	if (xgmiStatus.GetWidth() != 0) &&
		(xgmiStatus.GetWidth() != UINT16_MAX_VAL_UINT64) {
		fmt.Printf(indent+"%-38s : %v\n", "XGMI link width (in GB/s)",
			xgmiStatus.GetWidth())
	}
	if (xgmiStatus.GetSpeed() != 0) &&
		(xgmiStatus.GetSpeed() != UINT16_MAX_VAL_UINT64) {
		fmt.Printf(indent+"%-38s : %v\n", "XGMI link speed (in GB/s)",
			xgmiStatus.GetSpeed())
	}
	if status.GetThrottlingStatus() !=
		aga.GPUThrottlingStatus_GPU_THROTTLING_STATUS_NONE {
		fmt.Printf(indent+"%-38s : %s\n", "GPU throttling",
			strings.ToLower(strings.Replace(
				status.GetThrottlingStatus().String(), "GPU_THROTTLING_STATUS_",
				"", -1)))
	}
	if (status.GetFWTimestamp() != 0) &&
		(status.GetFWTimestamp() != UINT64_MAX_VAL) {
		fmt.Printf(indent+"%-38s : %v\n", "FW timestamp (in ns)",
			status.GetFWTimestamp())
	}
	/* commenting voltage-curve-point display for time being until it is added
	   back to status proto
		vcp := status.GetVoltageCurvePoint()
		if len(vcp) != 0 {
			valid_vc := false
			for i := 0; i < len(vcp); i++ {
				if vcp[i].GetFrequency() != 0 || vcp[i].GetVoltage() != 0 {
					valid_vc = true
				}
			}
			if valid_vc {
				fmt.Printf(indent+"Voltage curve points:\n")
				for i := 0; i < len(vcp); i++ {
					if vcp[i].GetFrequency() != 0 || vcp[i].GetVoltage() != 0 {
						fmt.Printf(indent+"  %-36s : %d\n", "Curve point",
						vcp[i].GetPoint())
						fmt.Printf(indent+"    %-34s : %d\n",
						"Frequency (in MHz)",
							vcp[i].GetFrequency())
						fmt.Printf(indent+"    %-34s : %d\n", "Voltage (in mV)",
							vcp[i].GetVoltage())
					}
				}
			}
		}
	*/
	if status.GetPCIeStatus() != nil {
		printHdr = false
		pcie := status.GetPCIeStatus()
		if pcie.GetVersion() != 0 {
			printPCIeStatusHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Version", pcie.GetVersion())
		}
		if pcie.GetSlotType() != aga.PCIeSlotType_PCIE_SLOT_TYPE_NONE {
			printPCIeStatusHdr(indent)
			fmt.Printf(indent+"  %-36s : %s\n", "Card form factor",
				strings.ToLower(strings.Replace(pcie.GetSlotType().String(),
					"PCIE_SLOT_TYPE_", "", -1)))
		}
		if pcie.GetPCIeBusId() != "" {
			printPCIeStatusHdr(indent)
			fmt.Printf(indent+"  %-36s : %s\n", "Bus id", pcie.GetPCIeBusId())
		}
		if pcie.GetWidth() != 0 {
			printPCIeStatusHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Current number of lanes",
				pcie.GetWidth())
		}
		if pcie.GetMaxWidth() != 0 {
			printPCIeStatusHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Maximum number of lanes",
				pcie.GetMaxWidth())
		}
		if pcie.GetSpeed() != 0 {
			printPCIeStatusHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Current speed (in GT/s)",
				pcie.GetSpeed())
		}
		if pcie.GetMaxSpeed() != 0 {
			printPCIeStatusHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Maximum speed (in GT/s)",
				pcie.GetMaxSpeed())
		}
		if pcie.GetBandwidth() != 0 &&
			pcie.GetBandwidth() != UINT32_MAX_VAL_UINT64 {
			printPCIeStatusHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Current bandwidth (in MB/s)",
				pcie.GetBandwidth())
		}
	}
	if status.GetVRAMStatus() != nil {
		printHdr = false
		vram := status.GetVRAMStatus()
		if vram.GetType() != aga.VRAMType_VRAM_TYPE_NONE {
			printVRAMStatusHdr(indent)
			fmt.Printf(indent+"  %-36s : %s\n", "VRAM type",
				strings.ToLower(strings.Replace(vram.GetType().String(),
					"VRAM_TYPE_", "", -1)))
		}
		if vram.GetVendor() != aga.VRAMVendor_VRAM_VENDOR_NONE {
			printVRAMStatusHdr(indent)
			if vram.GetVendor() != aga.VRAMVendor_VRAM_VENDOR_UNKNOWN {
				fmt.Printf(indent+"  %-36s : %s\n", "VRAM vendor",
					strings.ToLower(strings.Replace(vram.GetVendor().String(),
						"VRAM_VENDOR_", "", -1)))
			} else {
				fmt.Printf(indent+"  %-36s : %s\n", "VRAM vendor", "-")
			}
		}
		if vram.GetSize_() != 0 {
			printVRAMStatusHdr(indent)
			fmt.Printf(indent+"  %-36s : %v\n", "VRAM size (in MB)",
				vram.GetSize_())
		}
	}
	if statusOnly {
		fmt.Printf("\n%s\n", strings.Repeat("-", 80))
	}
}

func printVRAMUsageHdr(indent string) {
	if printHdr == false {
		fmt.Printf(indent + "VRAM usage:\n")
		printHdr = true
	}
}

func printVoltageHdr(indent string) {
	if printHdr == false {
		fmt.Printf(indent + "GPU voltage statistics:\n")
		printHdr = true
	}
}

func printTemperatureHdr(indent string) {
	if printHdr == false {
		fmt.Printf(indent + "Temperature information:\n")
		printHdr = true
	}
}

func printPCIeHdr(indent string) {
	if printHdr == false {
		fmt.Printf(indent + "PCIe statistics:\n")
		printHdr = true
	}
}

func printUsageHdr(indent string) {
	if printHdr == false {
		fmt.Printf(indent + "Current GPU usage:\n")
		printHdr = true
	}
}

func printGPUStats(gpu *aga.GPU, statsOnly bool) {
	var indent string
	spec := gpu.GetSpec()
	stats := gpu.GetStats()
	status := gpu.GetStatus()

	if statsOnly {
		fmt.Printf("\n%-38s : %s (%d)\n", "GPU id", utils.IdToStr(spec.GetId()),
			status.GetIndex())
		indent = ""
	} else {
		fmt.Printf("\nStatistics :\n")
		indent = "  "
	}
	if stats.GetPackagePower() != 0 &&
		stats.GetPackagePower() != UINT16_MAX_VAL_UINT64 {
		fmt.Printf(indent+"%-38s : %d\n", "Current graphics power (in Watts)",
			stats.GetPackagePower())
	}
	if stats.GetAvgPackagePower() != 0 &&
		stats.GetAvgPackagePower() != UINT16_MAX_VAL_UINT64 {
		fmt.Printf(indent+"%-38s : %d\n", "Average graphics power (in Watts)",
			stats.GetAvgPackagePower())
	}
	if stats.GetTemperature() != nil {
		printHdr = false
		if stats.GetTemperature().GetEdgeTemperature() != 0 {
			printTemperatureHdr(indent)
			fmt.Printf(indent+"  %-36s : %.1f\n", "Edge temperature (in C)",
				stats.GetTemperature().GetEdgeTemperature())
		}
		if stats.GetTemperature().GetJunctionTemperature() != 0 {
			printTemperatureHdr(indent)
			fmt.Printf(indent+"  %-36s : %.1f\n", "Junction temperature (in C)",
				stats.GetTemperature().GetJunctionTemperature())
		}
		if stats.GetTemperature().GetMemoryTemperature() != 0 {
			printTemperatureHdr(indent)
			fmt.Printf(indent+"  %-36s : %.1f\n", "VRAM temperature (in C)",
				stats.GetTemperature().GetMemoryTemperature())
		}
		hbmTemp := stats.GetTemperature().GetHBMTemperature()
		for index, temp := range hbmTemp {
			if temp != 0 {
				printTemperatureHdr(indent)
				hbmStr := "HBM " + strconv.Itoa(index) + " temperature (in C)"
				fmt.Printf(indent+"  %-36s : %.1f\n", hbmStr, temp)
			}
		}
	}
	if stats.GetUsage() != nil {
		printHdr = false
		if stats.GetUsage().GetGFXActivity() != 0 &&
			stats.GetUsage().GetGFXActivity() != UINT16_MAX_VAL_UINT32 {
			printUsageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "GFX activity",
				stats.GetUsage().GetGFXActivity())
		}
		if stats.GetUsage().GetUMCActivity() != 0 &&
			stats.GetUsage().GetUMCActivity() != UINT16_MAX_VAL_UINT32 {
			printUsageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "UMC activity",
				stats.GetUsage().GetUMCActivity())
		}
		if stats.GetUsage().GetMMActivity() != 0 &&
			stats.GetUsage().GetMMActivity() != UINT16_MAX_VAL_UINT32 {
			printUsageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "MM activity",
				stats.GetUsage().GetMMActivity())
		}
		vStr := fmt.Sprintf("  %-36s : ", "VCN activity")
		// used to decide if vcn activity should be printed or not
		validEntry := false
		for _, vcn := range stats.GetUsage().GetVCNActivity() {
			// only if at least one of the vcn activities is a valid value do we
			// print the field
			if vcn == UINT16_MAX_VAL_UINT32 {
				vStr = fmt.Sprintf("%sN/A ", vStr)
			} else if vcn == 0 {
				vStr = fmt.Sprintf("%s%d ", vStr, vcn)
			} else {
				validEntry = true
				vStr = fmt.Sprintf("%s%d ", vStr, vcn)
			}
		}
		if validEntry {
			printUsageHdr(indent)
			fmt.Printf(indent+"%s\n", vStr)
			validEntry = false
		}
		jStr := fmt.Sprintf("  %-36s : ", "JPEG activity")
		for i, jpeg := range stats.GetUsage().GetJPEGActivity() {
			// only if at least one of the jpeg activities is a valid value do
			// we print the field
			if jpeg == UINT16_MAX_VAL_UINT32 {
				jStr = fmt.Sprintf("%sN/A ", jStr)
			} else if jpeg == 0 {
				jStr = fmt.Sprintf("%s%d ", jStr, jpeg)
			} else {
				validEntry = true
				jStr = fmt.Sprintf("%s%d ", jStr, jpeg)
			}
			if (i+1)%8 == 0 {
				jStr = fmt.Sprintf("%s\n%s%-41s", jStr, indent, "")
			}
		}
		if validEntry {
			printUsageHdr(indent)
			fmt.Printf(indent+"%s\n", jStr)
			validEntry = false
		}
	}
	if stats.GetVoltage() != nil {
		v := stats.GetVoltage()
		printHdr = false
		if v.GetVoltage() != 0 && v.GetVoltage() != UINT16_MAX_VAL_UINT64 {
			printVoltageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Current voltage (in mV)",
				v.GetVoltage())
		}
		if v.GetGFXVoltage() != 0 &&
			v.GetGFXVoltage() != UINT16_MAX_VAL_UINT64 {
			printVoltageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n",
				"Current graphics voltage(in mV)", v.GetGFXVoltage())
		}
		if v.GetMemoryVoltage() != 0 &&
			v.GetMemoryVoltage() != UINT16_MAX_VAL_UINT64 {
			printVoltageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Current memory voltage(in mV)",
				v.GetMemoryVoltage())
		}
	}
	if stats.GetPCIeStats() != nil {
		printHdr = false
		p := stats.GetPCIeStats()
		if p.GetReplayCount() != 0 && p.GetReplayCount() != UINT64_MAX_VAL {
			printPCIeHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Replay count",
				p.GetReplayCount())
		}
		if p.GetRecoveryCount() != 0 &&
			p.GetRecoveryCount() != UINT64_MAX_VAL {
			printPCIeHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Recovery count",
				p.GetRecoveryCount())
		}
		if p.GetReplayRolloverCount() != 0 &&
			p.GetReplayRolloverCount() != UINT64_MAX_VAL {
			printPCIeHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Replay rollover count",
				p.GetReplayRolloverCount())
		}
		if p.GetNACKSentCount() != 0 &&
			p.GetNACKSentCount() != UINT64_MAX_VAL {
			printPCIeHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "NACKs sent",
				p.GetNACKSentCount())
		}
		if p.GetNACKReceivedCount() != 0 &&
			p.GetNACKReceivedCount() != UINT64_MAX_VAL {
			printPCIeHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "NACKs received",
				p.GetNACKReceivedCount())
		}
	}
	if stats.GetVRAMUsage() != nil {
		printHdr = false
		vram := stats.GetVRAMUsage()
		if vram.GetTotalVRAM() != 0 {
			printVRAMUsageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Total VRAM (in MB)",
				vram.GetTotalVRAM())
		}
		if vram.GetUsedVRAM() != 0 {
			printVRAMUsageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Used VRAM (in MB)",
				vram.GetUsedVRAM())
		}
		if vram.GetFreeVRAM() != 0 {
			printVRAMUsageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Free VRAM (in MB)",
				vram.GetFreeVRAM())
		}
		if vram.GetTotalVisibleVRAM() != 0 {
			printVRAMUsageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Total visible VRAM (in MB)",
				vram.GetTotalVisibleVRAM())
		}
		if vram.GetUsedVisibleVRAM() != 0 {
			printVRAMUsageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Used visible VRAM (in MB)",
				vram.GetUsedVisibleVRAM())
		}
		if vram.GetFreeVisibleVRAM() != 0 {
			printVRAMUsageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Free visible VRAM (in MB)",
				vram.GetFreeVisibleVRAM())
		}
		if vram.GetTotalGTT() != 0 {
			printVRAMUsageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Total GTT (in MB)",
				vram.GetTotalGTT())
		}
		if vram.GetUsedGTT() != 0 {
			printVRAMUsageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Used GTT (in MB)",
				vram.GetUsedGTT())
		}
		if vram.GetFreeGTT() != 0 {
			printVRAMUsageHdr(indent)
			fmt.Printf(indent+"  %-36s : %d\n", "Free GTT (in MB)",
				vram.GetFreeGTT())
		}
	}
	if stats.GetEnergyConsumed() != 0 {
		fmt.Printf(indent+"%-38s : %.2f\n",
			"Accumulated energy consumed (in uJ)",
			stats.GetEnergyConsumed())
	}
	if stats.GetTotalCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "Total correctable errors",
			stats.GetTotalCorrectableErrors())
	}
	if stats.GetTotalUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "Total uncorrectable errors",
			stats.GetTotalUncorrectableErrors())
	}
	if stats.GetSDMACorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "SDMA correctable errors",
			stats.GetSDMACorrectableErrors())
	}
	if stats.GetSDMAUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "SDMA uncorrectable errors",
			stats.GetSDMAUncorrectableErrors())
	}
	if stats.GetGFXCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "GFX correctable errors",
			stats.GetGFXCorrectableErrors())
	}
	if stats.GetGFXUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "GFX uncorrectable errors",
			stats.GetGFXUncorrectableErrors())
	}
	if stats.GetMMHUBCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "MMHUB correctable errors",
			stats.GetMMHUBCorrectableErrors())
	}
	if stats.GetMMHUBUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "MMHUB uncorrectable errors",
			stats.GetMMHUBUncorrectableErrors())
	}
	if stats.GetATHUBCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "ATHUB correctable errors",
			stats.GetATHUBCorrectableErrors())
	}
	if stats.GetATHUBUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "ATHUB uncorrectable errors",
			stats.GetATHUBUncorrectableErrors())
	}
	if stats.GetBIFCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "BIF correctable errors",
			stats.GetBIFCorrectableErrors())
	}
	if stats.GetBIFUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "BIF uncorrectable errors",
			stats.GetBIFUncorrectableErrors())
	}
	if stats.GetHDPCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "HDP correctable errors",
			stats.GetHDPCorrectableErrors())
	}
	if stats.GetHDPUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "HDP uncorrectable errors",
			stats.GetHDPUncorrectableErrors())
	}
	if stats.GetXGMIWAFLCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "XGMI WAFL correctable errors",
			stats.GetXGMIWAFLCorrectableErrors())
	}
	if stats.GetXGMIWAFLUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "XGMI WAFL uncorrectable errors",
			stats.GetXGMIWAFLUncorrectableErrors())
	}
	if stats.GetDFCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "DF correctable errors",
			stats.GetDFCorrectableErrors())
	}
	if stats.GetDFUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "DF uncorrectable errors",
			stats.GetDFUncorrectableErrors())
	}
	if stats.GetSMNCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "SMN correctable errors",
			stats.GetSMNCorrectableErrors())
	}
	if stats.GetSMNUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "SMN uncorrectable errors",
			stats.GetSMNUncorrectableErrors())
	}
	if stats.GetSEMCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "SEM correctable errors",
			stats.GetSEMCorrectableErrors())
	}
	if stats.GetSEMUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "SEM uncorrectable errors",
			stats.GetSEMUncorrectableErrors())
	}
	if stats.GetMP0CorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "MP0 correctable errors",
			stats.GetMP0CorrectableErrors())
	}
	if stats.GetMP0UncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "MP0 uncorrectable errors",
			stats.GetMP0UncorrectableErrors())
	}
	if stats.GetMP1CorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "MP1 correctable errors",
			stats.GetMP1CorrectableErrors())
	}
	if stats.GetMP1UncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "MP1 uncorrectable errors",
			stats.GetMP1UncorrectableErrors())
	}
	if stats.GetFUSECorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "FUSE correctable errors",
			stats.GetFUSECorrectableErrors())
	}
	if stats.GetFUSEUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "FUSE uncorrectable errors",
			stats.GetFUSEUncorrectableErrors())
	}
	if stats.GetUMCCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "UMC correctable errors",
			stats.GetUMCCorrectableErrors())
	}
	if stats.GetUMCUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "UMC uncorrectable errors",
			stats.GetUMCUncorrectableErrors())
	}
	if stats.GetMCACorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "MCA correctable errors",
			stats.GetMCACorrectableErrors())
	}
	if stats.GetMCAUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "MCA uncorrectable errors",
			stats.GetMCAUncorrectableErrors())
	}
	if stats.GetVCNCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "VCN correctable errors",
			stats.GetVCNCorrectableErrors())
	}
	if stats.GetVCNUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "VCN uncorrectable errors",
			stats.GetVCNUncorrectableErrors())
	}
	if stats.GetJPEGCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "JPEG correctable errors",
			stats.GetJPEGCorrectableErrors())
	}
	if stats.GetJPEGUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "JPEG uncorrectable errors",
			stats.GetJPEGUncorrectableErrors())
	}
	if stats.GetIHCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "IH correctable errors",
			stats.GetIHCorrectableErrors())
	}
	if stats.GetIHUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "IH uncorrectable errors",
			stats.GetIHUncorrectableErrors())
	}
	if stats.GetMPIOCorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "MPIO correctable errors",
			stats.GetMPIOCorrectableErrors())
	}
	if stats.GetMPIOUncorrectableErrors() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "MPIO uncorrectable errors",
			stats.GetMPIOUncorrectableErrors())
	}
	if stats.GetXGMINeighbor0TxNOPs() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "Nops sent to XGMI neighbor0",
			stats.GetXGMINeighbor0TxNOPs())
	}
	if stats.GetXGMINeighbor0TxRequests() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "Outgoing requests to XGMI neighbor0",
			stats.GetXGMINeighbor0TxRequests())
	}
	if stats.GetXGMINeighbor0TxResponses() != 0 {
		fmt.Printf(indent+"%-38s : %d\n",
			"Outgoing responses to XGMI neighbor0",
			stats.GetXGMINeighbor0TxRequests())
	}
	if stats.GetXGMINeighbor0TXBeats() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "Data beats sent to neighbor0",
			stats.GetXGMINeighbor0TXBeats())
	}
	if stats.GetXGMINeighbor1TxNOPs() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "Nops sent to XGMI neighbor1",
			stats.GetXGMINeighbor1TxNOPs())
	}
	if stats.GetXGMINeighbor1TxRequests() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "Outgoing requests to XGMI neighbor1",
			stats.GetXGMINeighbor1TxRequests())
	}
	if stats.GetXGMINeighbor1TxResponses() != 0 {
		fmt.Printf(indent+"%-38s : %d\n",
			"Outgoing responses to XGMI neighbor1",
			stats.GetXGMINeighbor1TxRequests())
	}
	if stats.GetXGMINeighbor1TXBeats() != 0 {
		fmt.Printf(indent+"%-38s : %d\n", "Data beats sent to neighbor1",
			stats.GetXGMINeighbor1TXBeats())
	}
	if stats.GetXGMINeighbor0TxThroughput() != 0 {
		fmt.Printf(indent+"%-38s : %d\n",
			"Tx throughput to XGMI neighbor0 (in BPS)",
			stats.GetXGMINeighbor0TxThroughput())
	}
	if stats.GetXGMINeighbor1TxThroughput() != 0 {
		fmt.Printf(indent+"%-38s : %d\n",
			"Tx throughput to XGMI neighbor1 (in BPS)",
			stats.GetXGMINeighbor1TxThroughput())
	}
	if stats.GetXGMINeighbor2TxThroughput() != 0 {
		fmt.Printf(indent+"%-38s : %d\n",
			"Tx throughput to XGMI neighbor2 (in BPS)",
			stats.GetXGMINeighbor2TxThroughput())
	}
	if stats.GetXGMINeighbor3TxThroughput() != 0 {
		fmt.Printf(indent+"%-38s : %d\n",
			"Tx throughput to XGMI neighbor3 (in BPS)",
			stats.GetXGMINeighbor3TxThroughput())
	}
	if stats.GetXGMINeighbor4TxThroughput() != 0 {
		fmt.Printf(indent+"%-38s : %d\n",
			"Tx throughput to XGMI neighbor4 (in BPS)",
			stats.GetXGMINeighbor4TxThroughput())
	}
	if stats.GetXGMINeighbor5TxThroughput() != 0 {
		fmt.Printf(indent+"%-38s : %d\n",
			"Tx throughput to XGMI neighbor5 (in BPS)",
			stats.GetXGMINeighbor5TxThroughput())
	}
	if stats.GetPowerUsage() != 0 {
		fmt.Printf(indent+"%-38s : %d\n",
			"Power usage (in Watts)", stats.GetPowerUsage())
	}
	if (stats.GetFanSpeed() != 0) &&
		(stats.GetFanSpeed() != UINT16_MAX_VAL_UINT64) {
		fmt.Printf(indent+"%-38s : %d\n",
			"Fan speed (in RPMs)", stats.GetFanSpeed())
	}
	if (stats.GetGFXActivityAccumulated() != 0) &&
		(stats.GetGFXActivityAccumulated() != UINT64_MAX_VAL) {
		fmt.Printf(indent+"%-38s : %d\n",
			"GFX activity accumulated",
			stats.GetGFXActivityAccumulated())
	}
	if (stats.GetMemoryActivityAccumulated() != 0) &&
		(stats.GetMemoryActivityAccumulated() != UINT64_MAX_VAL) {
		fmt.Printf(indent+"%-38s : %d\n",
			"Memory activity accumulated",
			stats.GetMemoryActivityAccumulated())
	}
	for i, linkStats := range stats.GetXGMILinkStats() {
		link := "Link " + fmt.Sprintf("%v", i+1)
		if (linkStats.GetDataRead() != 0) &&
			(linkStats.GetDataRead() != UINT64_MAX_VAL) {
			fmt.Printf(indent+"%-38s : %d\n", link+" data read (in KB)",
				linkStats.GetDataRead())
		}
		if (linkStats.GetDataWrite() != 0) &&
			(linkStats.GetDataWrite() != UINT64_MAX_VAL) {
			fmt.Printf(indent+"%-38s : %d\n", link+" data written (in KB)",
				linkStats.GetDataWrite())
		}
	}
	if stats.GetViolationStats() != nil {
		vStats := stats.GetViolationStats()
		fmt.Printf(indent+"%-38s : %d\n", "Current accumulated counter",
			vStats.GetCurrentAccumulatedCounter())
		fmt.Printf(indent+"%-38s : %d\n", "Processor hot residency accumulated",
			vStats.GetProcessorHotResidencyAccumulated())
		fmt.Printf(indent+"%-38s : %d\n", "PPT residency accumulated",
			vStats.GetPPTResidencyAccumulated())
		fmt.Printf(indent+"%-38s : %d\n",
			"Socket thermal residency accumulated",
			vStats.GetSocketThermalResidencyAccumulated())
		fmt.Printf(indent+"%-38s : %d\n", "VR thermal residency accumulated",
			vStats.GetVRThermalResidencyAccumulated())
		fmt.Printf(indent+"%-38s : %d\n", "HBM thermal residency accumulated",
			vStats.GetHBMThermalResidencyAccumulated())
	}

	fmt.Printf("\n%s\n", strings.Repeat("-", 80))
}

type ShadowGPU struct {
	Id string
	*aga.GPUSpec
	*aga.GPUStatus
	*aga.GPUStats
}

func NewGPU(resp *aga.GPU) *ShadowGPU {
	return &ShadowGPU{
		Id:        utils.IdToStr(resp.GetSpec().GetId()),
		GPUSpec:   resp.GetSpec(),
		GPUStatus: resp.GetStatus(),
		GPUStats:  resp.GetStats(),
	}
}

func printGPUJson(resp *aga.GPU) {
	gpu := NewGPU(resp)
	b, _ := json.MarshalIndent(gpu, "  ", " ")
	fmt.Println(string(b))
}

func gpuUpdateCmdPreRunE(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	if cmd.Flags().NFlag() == 1 {
		return fmt.Errorf("Nothing to update")
	}
	if err := utils.IsUUIDValid(gpuID); err != nil {
		return err
	}
	if cmd.Flags().Changed("admin-state") {
		switch strings.ToLower(gpuAdminState) {
		case "up":
			gpuAdminStateVal = aga.GPUAdminState_GPU_ADMIN_STATE_UP
		case "down":
			gpuAdminStateVal = aga.GPUAdminState_GPU_ADMIN_STATE_DOWN
		default:
			return fmt.Errorf("Invalid argument for \"admin-state\", please " +
				"refer help")
		}
	}
	if cmd.Flags().Changed("perf-level") {
		switch strings.ToLower(perfLevel) {
		case "none":
			PerformanceLevelVal = aga.GPUPerformanceLevel_GPU_PERF_LEVEL_NONE
		case "auto":
			PerformanceLevelVal = aga.GPUPerformanceLevel_GPU_PERF_LEVEL_AUTO
		case "low":
			PerformanceLevelVal = aga.GPUPerformanceLevel_GPU_PERF_LEVEL_LOW
		case "high":
			PerformanceLevelVal = aga.GPUPerformanceLevel_GPU_PERF_LEVEL_HIGH
		case "deterministic":
			PerformanceLevelVal =
				aga.GPUPerformanceLevel_GPU_PERF_LEVEL_DETERMINISTIC
		case "memclock":
			PerformanceLevelVal =
				aga.GPUPerformanceLevel_GPU_PERF_LEVEL_STABLE_MIN_MCLK
		case "sysclock":
			PerformanceLevelVal =
				aga.GPUPerformanceLevel_GPU_PERF_LEVEL_STABLE_MIN_SCLK
		case "manual":
			PerformanceLevelVal = aga.GPUPerformanceLevel_GPU_PERF_LEVEL_MANUAL
		default:
			return fmt.Errorf("Invalid argument for \"perf-level\", please " +
				"refer help")
		}
	}
	if cmd.Flags().Changed("clock-frequency") !=
		cmd.Flags().Changed("clock-type") {
		return fmt.Errorf("Both \"clock-type\" and \"clock-frequency\" need " +
			"to be specified")
	}
	if cmd.Flags().Changed("clock-type") {
		switch strings.ToLower(gpuClkType) {
		case "memory":
			clockType = aga.GPUClockType_GPU_CLOCK_TYPE_MEMORY
		case "system":
			clockType = aga.GPUClockType_GPU_CLOCK_TYPE_SYSTEM
		case "video":
			clockType = aga.GPUClockType_GPU_CLOCK_TYPE_VIDEO
		case "data":
			clockType = aga.GPUClockType_GPU_CLOCK_TYPE_DATA
		default:
			return fmt.Errorf("Invalid \"clock-type\" specified, please " +
				"refer help")
		}
	}
	if cmd.Flags().Changed("clock-frequency") {
		_, err := fmt.Sscanf(gpuClkFreq, "%d-%d", &gpuClkFreqLo, &gpuClkFreqHi)
		if err != nil {
			return fmt.Errorf("Invalid range for \"clock-frequency\", please " +
				"refer help")
		}
	}
	return nil
}

func gpuUpdateCmdHandler(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	if len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	cmd.SilenceUsage = true

	// get GPU spec
	respMsg := &aga.GPUGetResponse{}
	var req *aga.GPUGetRequest
	if cmd.Flags().Changed("id") {
		// get specific GPU
		req = &aga.GPUGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(gpuID).Bytes()},
		}
	}
	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent " +
			"running?")
	}
	defer c.Close()
	defer cancel()

	client := aga.NewGPUSvcClient(c)
	respMsg, err = client.GPUGet(ctxt, req)
	if err != nil {
		return fmt.Errorf("Getting GPU failed, err %v", err)
	}
	if respMsg.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Get GPU failed with %v error", respMsg.ApiStatus)
	}
	resp := respMsg.GetResponse()[0]
	gpuSpec := resp.GetSpec()
	if gpuSpec == nil {
		return fmt.Errorf("GPU object not found")
	}
	// update the configured fields
	updateSpec := *gpuSpec
	if cmd.Flags().Changed("admin-state") {
		updateSpec.AdminState = gpuAdminStateVal
	}
	if cmd.Flags().Changed("overdrive-level") {
		updateSpec.OverDriveLevel = overDriveLevel
	}
	if cmd.Flags().Changed("power-cap") {
		updateSpec.GPUPowerCap = powerCap
	}
	if cmd.Flags().Changed("perf-level") {
		updateSpec.PerformanceLevel = PerformanceLevelVal
	}
	if cmd.Flags().Changed("clock-frequency") {
		for i, freq := range updateSpec.GetClockFrequency() {
			if freq.GetClockType() == clockType {
				updateSpec.ClockFrequency[i] = &aga.GPUClockFrequencyRange{
					ClockType:     clockType,
					LowFrequency:  gpuClkFreqLo,
					HighFrequency: gpuClkFreqHi,
				}
			}
		}
	}
	if cmd.Flags().Changed("fan-speed") {
		updateSpec.FanSpeed = fanSpeed
	}
	reqMsg := &aga.GPUUpdateRequest{
		Spec: []*aga.GPUSpec{
			&updateSpec,
		},
	}
	// GPU agent call
	updateRespMsg, err := client.GPUUpdate(ctxt, reqMsg)
	if err != nil {
		return fmt.Errorf("Updating GPU failed, err %v", err)
	}
	if updateRespMsg.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Operation failed with error %v, error code %v",
			updateRespMsg.ApiStatus, updateRespMsg.ErrorCode)
	}
	fmt.Printf("Updating GPU succeeded\n")
	return nil
}

func gpuResetCmdPreRunE(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	if err := utils.IsUUIDValid(gpuID); err != nil {
		return err
	}
	numFlags := 0
	if cmd.Flags().Changed("clocks") {
		numFlags += 1
	}
	if cmd.Flags().Changed("fans") {
		numFlags += 1
	}
	if cmd.Flags().Changed("power-profile") {
		numFlags += 1
	}
	if cmd.Flags().Changed("power-overdrive") {
		numFlags += 1
	}
	if cmd.Flags().Changed("xgmi-error") {
		numFlags += 1
	}
	if cmd.Flags().Changed("perf-determinism") {
		numFlags += 1
	}
	if cmd.Flags().Changed("compute-partition") {
		numFlags += 1
	}
	if cmd.Flags().Changed("nps-mode") {
		numFlags += 1
	}
	if numFlags == 0 {
		// more than 1 reset option is specified, reject
		return fmt.Errorf("Invalid arguments, one of \"clocks\", \"fans\", " +
			"\"power-profile\", \"power-overdrive\", \"xgmi-error\", " +
			"\"perf-determinism\", \"compute-partition\", \"nps-mode\" must " +
			"be specified")
	}
	// all above options are mutually exclusive
	if numFlags > 1 {
		// more than 1 reset option is specified, reject
		return fmt.Errorf("Invalid arguments, \"clocks\", \"fans\", " +
			"\"power-profile\", \"power-overdrive\", \"xgmi-error\", " +
			"\"perf-determinism\", \"compute-partition\", \"nps-mode\" are " +
			"mutually exlcusive, specify only one")
	}
	return nil
}

func gpuResetCmdHandler(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	if len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	cmd.SilenceUsage = true

	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent " +
			"running?")
	}
	defer c.Close()
	defer cancel()

	req := &aga.GPUResetRequest{
		Id: [][]byte{uuid.FromStringOrNil(gpuID).Bytes()},
	}
	if cmd.Flags().Changed("clocks") {
		req.Reset_ = &aga.GPUResetRequest_ResetClocks{
			ResetClocks: true,
		}
	} else if cmd.Flags().Changed("fans") {
		req.Reset_ = &aga.GPUResetRequest_ResetFans{
			ResetFans: true,
		}
	} else if cmd.Flags().Changed("power-profile") {
		req.Reset_ = &aga.GPUResetRequest_ResetPowerProfile{
			ResetPowerProfile: true,
		}
	} else if cmd.Flags().Changed("power-overdrive") {
		req.Reset_ = &aga.GPUResetRequest_ResetPowerOverDrive{
			ResetPowerOverDrive: true,
		}
	} else if cmd.Flags().Changed("xgmi-error") {
		req.Reset_ = &aga.GPUResetRequest_ResetXGMIError{
			ResetXGMIError: true,
		}
	} else if cmd.Flags().Changed("perf-determinism") {
		req.Reset_ = &aga.GPUResetRequest_ResetPerfDeterminism{
			ResetPerfDeterminism: true,
		}
	} else if cmd.Flags().Changed("compute-partition") {
		req.Reset_ = &aga.GPUResetRequest_ResetComputePartition{
			ResetComputePartition: true,
		}
	} else if cmd.Flags().Changed("nps-mode") {
		req.Reset_ = &aga.GPUResetRequest_ResetNPSMode{
			ResetNPSMode: true,
		}
	}
	client := aga.NewGPUSvcClient(c)
	respMsg, err := client.GPUReset(ctxt, req)
	if err != nil {
		return fmt.Errorf("Resetting GPU failed, err %v", err)
	}
	if respMsg.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Operation failed with error %v, error code %v",
			respMsg.ApiStatus, respMsg.ErrorCode)
	}
	fmt.Printf("Resetting GPU succeeded\n")
	return nil
}
