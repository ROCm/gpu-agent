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
/// gpuctl command line interface for gpu watch protobufs
///
//------------------------------------------------------------------------------

package cmd

import (
	"encoding/json"
	"fmt"
	"io"
	"strings"

	uuid "github.com/satori/go.uuid"
	"github.com/spf13/cobra"
	yaml "gopkg.in/yaml.v2"

	"github.com/ROCm/gpu-agent/sw/nic/gpuagent/cli/utils"
	aga "github.com/ROCm/gpu-agent/sw/nic/gpuagent/gen/go"
)

var (
	gpuWatchID       string
	gpuWatchUUIDs    [][]byte
	gpuWatchGPUsStr  string
	gpuWatchGPUs     []string
	gpuWatchGPUIDs   [][]byte
	gpuWatchAttrsStr string
	gpuWatchAttrs    []string
	gpuWatchAttrIDs  []aga.GPUWatchAttrId
)

var gpuWatchCreateCmd = &cobra.Command{
	Use:     "gpu-watch",
	Short:   "create GPU watch",
	Long:    "create GPU watch object",
	PreRunE: gpuWatchCreateCmdPreRun,
	RunE:    gpuWatchCreateCmdHandler,
}

var gpuWatchDeleteCmd = &cobra.Command{
	Use:   "gpu-watch",
	Short: "delete GPU watch",
	Long:  "delete GPU watch object",
	RunE:  gpuWatchDeleteCmdHandler,
}

var gpuWatchShowCmd = &cobra.Command{
	Use:   "gpu-watch",
	Short: "show GPU watch object",
	Long:  "show GPU watch object",
	RunE:  gpuWatchShowCmdHandler,
}

var gpuWatchAllShowCmd = &cobra.Command{
	Use:   "all",
	Short: "show all GPU watch object",
	Long:  "show all GPU watch object",
	RunE:  gpuWatchAllShowCmdHandler,
}

var gpuWatchStatsShowCmd = &cobra.Command{
	Use:   "statistics",
	Short: "show GPU watch statistics",
	Long:  "show GPU watch statistics",
	RunE:  gpuWatchStatsShowCmdHandler,
}

var gpuWatchDebugCmd = &cobra.Command{
	Use:   "gpu-watch",
	Short: "GPU watch object",
	Long:  "GPU watch object",
}

var gpuWatchSubscribeCmd = &cobra.Command{
	Use:     "subscribe",
	Short:   "subscribe to GPU watch objects",
	Long:    "subscribe to GPU watch objects",
	PreRunE: gpuWatchSubscribeCmdPreRun,
	RunE:    gpuWatchSubscribeCmdHandler,
}

func init() {
	DebugCreateCmd.AddCommand(gpuWatchCreateCmd)
	gpuWatchCreateCmd.Flags().StringVarP(&gpuWatchID, "id", "i", "",
		"Specify GPU watch id")
	gpuWatchCreateCmd.Flags().StringVarP(&gpuWatchGPUsStr, "gpu", "g",
		"", "Specify comma separated list of GPUs to watch")
	gpuWatchCreateCmd.Flags().StringVarP(&gpuWatchAttrsStr, "attr", "a",
		"", "Specify comma separated list of attributes to watch "+
			"(gpu-clock, memory-clock, memory-temp, gpu-temp, power-usage, "+
			"ecc-total, pcie-bandwidth, gpu-util, memory-usage, ecc-count, "+
			"xgmi-tx, xgmi-throughput)")
	gpuWatchCreateCmd.MarkFlagRequired("id")
	gpuWatchCreateCmd.MarkFlagRequired("gpu")
	gpuWatchCreateCmd.MarkFlagRequired("attr")

	DebugDeleteCmd.AddCommand(gpuWatchDeleteCmd)
	gpuWatchDeleteCmd.Flags().StringVarP(&gpuWatchID, "id", "i", "",
		"Specify GPU watch id")
	gpuWatchDeleteCmd.MarkFlagRequired("id")

	ShowCmd.AddCommand(gpuWatchShowCmd)
	gpuWatchShowCmd.Flags().StringVarP(&gpuWatchID, "id", "i", "",
		"Specify GPU watch id")
	gpuWatchShowCmd.Flags().Bool("status", false, "Show GPU watch status")
	gpuWatchShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	gpuWatchShowCmd.Flags().Bool("json", false, "Output in json")

	gpuWatchShowCmd.AddCommand(gpuWatchAllShowCmd)
	gpuWatchAllShowCmd.Flags().StringVarP(&gpuWatchID, "id", "i", "",
		"Specify GPU watch id")

	gpuWatchShowCmd.AddCommand(gpuWatchStatsShowCmd)
	gpuWatchStatsShowCmd.Flags().StringVarP(&gpuWatchID, "id", "i", "",
		"Specify GPU watch id")

	debugCmd.AddCommand(gpuWatchDebugCmd)
	gpuWatchDebugCmd.AddCommand(gpuWatchSubscribeCmd)
	gpuWatchSubscribeCmd.Flags().StringVarP(&gpuWatchID, "id", "i", "",
		"Specify comma separated list of GPU watch ids to subscribe")
	gpuWatchSubscribeCmd.MarkFlagRequired("id")
}

func gpuWatchCreateCmdPreRun(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	if err := utils.IsUUIDValid(gpuWatchID); err != nil {
		return err
	}
	gpuWatchGPUs := strings.Split(gpuWatchGPUsStr, ",")
	for _, gpu := range gpuWatchGPUs {
		if err := utils.IsUUIDValid(gpu); err != nil {
			return err
		}
		gpuWatchGPUIDs = append(gpuWatchGPUIDs,
			uuid.FromStringOrNil(gpu).Bytes())
	}
	gpuWatchAttrs := strings.Split(gpuWatchAttrsStr, ",")
	for _, attr := range gpuWatchAttrs {
		switch strings.ToLower(attr) {
		case "gpu-clock":
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_GPU_CLOCK)
		case "memory-clock":
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_MEM_CLOCK)
		case "memory-temp":
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_MEMORY_TEMP)
		case "gpu-temp":
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_GPU_TEMP)
		case "power-usage":
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_POWER_USAGE)
		case "ecc-total":
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_CORRECT_TOTAL)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_UNCORRECT_TOTAL)
		case "pcie-bandwidth":
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_PCIE_BANDWIDTH)
		case "gpu-util":
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_GPU_UTIL)
		case "memory-usage":
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_GPU_MEMORY_USAGE)
		case "ecc-count":
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_SDMA_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_SDMA_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_GFX_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_GFX_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_MMHUB_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_MMHUB_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_ATHUB_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_ATHUB_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_PCIE_BIF_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_HDP_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_HDP_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_XGMI_WAFL_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_DF_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_DF_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_SMN_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_SMN_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_SEM_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_SEM_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_MP0_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_MP0_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_MP1_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_MP1_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_FUSE_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_FUSE_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_UMC_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_UMC_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_MCA_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_MCA_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_VCN_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_VCN_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_JPEG_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_JPEG_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_IH_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_IH_UE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_MPIO_CE)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_ECC_MPIO_UE)
		case "xgmi-tx":
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_XGMI_0_NOP_TX)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_XGMI_1_NOP_TX)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_XGMI_0_REQ_TX)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_XGMI_1_REQ_TX)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_XGMI_0_RESP_TX)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_XGMI_1_RESP_TX)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_XGMI_0_BEATS_TX)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_XGMI_1_BEATS_TX)
		case "xgmi-throughput":
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_XGMI_0_THRPUT)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_XGMI_1_THRPUT)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_XGMI_2_THRPUT)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_XGMI_3_THRPUT)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_XGMI_4_THRPUT)
			gpuWatchAttrIDs = append(gpuWatchAttrIDs,
				aga.GPUWatchAttrId_GPU_WATCH_ATTR_ID_XGMI_5_THRPUT)
		default:
			return fmt.Errorf("Invalid GPU watch attribute specified")
		}
	}
	return nil
}

func gpuWatchCreateCmdHandler(cmd *cobra.Command, args []string) error {
	if cmd == nil || len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	cmd.SilenceUsage = true
	spec := &aga.GPUWatchSpec{
		Id:        uuid.FromStringOrNil(gpuWatchID).Bytes(),
		GPU:       gpuWatchGPUIDs,
		Attribute: gpuWatchAttrIDs,
	}
	req := &aga.GPUWatchRequest{
		Spec: []*aga.GPUWatchSpec{spec},
	}
	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent running?")
	}
	defer c.Close()
	defer cancel()
	client := aga.NewGPUWatchSvcClient(c)
	respMsg, err := client.GPUWatchCreate(ctxt, req)
	if err != nil {
		return fmt.Errorf("Creating GPU watch failed, err %v", err)
	}
	if respMsg.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Operation failed with %v error", respMsg.ApiStatus)
	}
	fmt.Printf("GPU watch %s created successfully\n", gpuWatchID)
	return nil
}

func gpuWatchDeleteCmdHandler(cmd *cobra.Command, args []string) error {
	if cmd == nil || len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	cmd.SilenceUsage = true
	if err := utils.IsUUIDValid(gpuWatchID); err != nil {
		return err
	}
	req := &aga.GPUWatchDeleteRequest{
		Id: [][]byte{uuid.FromStringOrNil(gpuWatchID).Bytes()},
	}
	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent running?")
	}
	defer c.Close()
	defer cancel()
	client := aga.NewGPUWatchSvcClient(c)
	respMsg, err := client.GPUWatchDelete(ctxt, req)
	if err != nil {
		return fmt.Errorf("Deleting GPU watch failed, err %v", err)
	}
	if respMsg.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Operation failed with %v error", respMsg.ApiStatus)
	}
	fmt.Printf("GPU watch %s deleted successfully\n", gpuWatchID)
	return nil
}

type GPUWatchSpec struct {
	Id        string
	GPU       []string
	Attribute []aga.GPUWatchAttrId
}

func printGPUWatchJson(resp *aga.GPUWatch) {
	var spec GPUWatchSpec
	spec.Id = utils.IdToStr(resp.GetSpec().GetId())
	for _, gpu := range resp.GetSpec().GetGPU() {
		spec.GPU = append(spec.GPU, utils.IdToStr(gpu))
	}
	spec.Attribute = resp.GetSpec().GetAttribute()
	b, _ := json.MarshalIndent(&spec, "  ", " ")
	bString := string(b)
	fmt.Println(" {")
	fmt.Printf("  \"Spec\": %s", bString[:len(bString)-4])
	fmt.Printf("  },\n")

	status := resp.GetStatus()
	b, _ = json.MarshalIndent(status, "  ", " ")
	fmt.Printf("  \"Status\": %s,\n", string(b))

	stats := resp.GetStats()
	b, _ = json.MarshalIndent(stats, "  ", " ")
	fmt.Printf("  \"Stats\": %s\n", string(b))
	fmt.Printf(" }")
}

func printGPUWatchSpec(resp *aga.GPUWatch, specOnly bool) {
	fmt.Printf("\n%-23s : %s\n", "GPU watch id",
		utils.IdToStr(resp.GetSpec().GetId()))
	for i, gpu := range resp.GetSpec().GetGPU() {
		if i == 0 {
			fmt.Printf("%-23s : %s\n", "GPUs", utils.IdToStr(gpu))
		} else {
			fmt.Printf("%-26s%s\n", "", utils.IdToStr(gpu))
		}
	}
	for i, attr := range resp.GetSpec().GetAttribute() {
		attrStr := strings.ToLower(strings.Replace(attr.String(),
			"GPU_WATCH_ATTR_ID_", "", -1))
		attrStr = strings.Replace(attrStr, "_", "-", -1)
		if i == 0 {
			fmt.Printf("%-23s : %s", "Attributes", attrStr)
		} else if i%3 == 0 {
			fmt.Printf("\n%-26s%s", "", attrStr)
		} else {
			fmt.Printf(", %s", attrStr)
		}
	}
	fmt.Println()
	if specOnly {
		fmt.Printf("\n%s\n", strings.Repeat("-", 60))
	}
}

func printGPUWatchStatus(resp *aga.GPUWatch, statusOnly bool) {
	var indent string
	if statusOnly {
		fmt.Printf("\n%-21s : %s\n", "GPU watch id",
			utils.IdToStr(resp.GetSpec().GetId()))
		indent = ""
	} else {
		fmt.Printf("\nStatus :\n\n")
		indent = "  "
	}
	fmt.Printf(indent+"%-21s : %d\n", "Number of subscribers",
		resp.GetStatus().GetNumSubscribers())
	if statusOnly {
		fmt.Printf("\n%s\n", strings.Repeat("-", 60))
	}
}

func printGPUWatchStats(resp *aga.GPUWatch, statsOnly bool) {
	if statsOnly {
		fmt.Printf("\nGPU watch id : %s\n",
			utils.IdToStr(resp.GetSpec().GetId()))
	} else {
		fmt.Printf("\nStatistics :\n")
	}
	for _, gpuAttr := range resp.GetStats().GetGPUWatchAttr() {
		fmt.Printf("\n  GPU : %s\n", utils.IdToStr(gpuAttr.GetGPU()))
		for i, attr := range gpuAttr.GetAttr() {
			if i == 0 {
				line := strings.Repeat("-", 42)
				fmt.Printf("  %s\n", line)
				fmt.Printf("  %-30s%-12s\n", "Attribute", "Value")
				fmt.Printf("  %s\n", line)
			}
			attrStr := strings.ToLower(strings.Replace(attr.GetId().String(),
				"GPU_WATCH_ATTR_ID_", "", -1))
			attrStr = strings.Replace(attrStr, "_", "-", -1)
			switch attr.GetValue().GetWatchAttrVal().(type) {
			case *aga.GPUWatchAttrVal_LongVal:
				valStr := fmt.Sprintf("%v %s", attr.GetValue().GetLongVal(),
					attr.GetValue().GetUnits())
				fmt.Printf("  %-30s%-12v\n", attrStr, valStr)
			case *aga.GPUWatchAttrVal_FloatVal:
				valStr := fmt.Sprintf("%v %s", attr.GetValue().GetFloatVal(),
					attr.GetValue().GetUnits())
				fmt.Printf("  %-30s%-12v\n", attrStr, valStr)
			case *aga.GPUWatchAttrVal_StringVal:
				fmt.Printf("  %-30s%-12s\n", attrStr,
					attr.GetValue().GetStringVal())
			}
		}
	}
	fmt.Printf("\n%s\n", strings.Repeat("-", 60))
}

func printGPUWatchSummary(count int) {
	fmt.Printf("\nNo. of GPU watch objects : %d\n\n", count)
}

func gpuWatchShowCmdHandler(cmd *cobra.Command, args []string) error {
	if cmd == nil || len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	cmd.SilenceUsage = true
	var req *aga.GPUWatchGetRequest
	if cmd.Flags().Changed("id") {
		if err := utils.IsUUIDValid(gpuWatchID); err != nil {
			return err
		}
		req = &aga.GPUWatchGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(gpuWatchID).Bytes()},
		}
	} else {
		req = &aga.GPUWatchGetRequest{}
	}
	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent running?")
	}
	defer c.Close()
	defer cancel()
	client := aga.NewGPUWatchSvcClient(c)
	respMsg, err := client.GPUWatchGet(ctxt, req)
	if err != nil {
		return fmt.Errorf("Getting GPU watch failed, err %v", err)
	}
	if respMsg.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Operation failed with %v error",
			respMsg.ApiStatus)
	}
	if cmd.Flags().Changed("yaml") {
		// convert all the response objects in a stream as single yaml
		// document
		yamlArr, _ := yaml.Marshal(respMsg.Response)
		fmt.Println(string(yamlArr))
		fmt.Println("---")
	} else if cmd.Flags().Changed("json") {
		if len(respMsg.Response) == 0 {
			fmt.Printf("[]\n")
		} else {
			fmt.Printf("[\n")
			for i, resp := range respMsg.Response {
				if i > 0 {
					fmt.Printf(",\n")
				}
				fmt.Printf("\n")
				printGPUWatchJson(resp)
			}
			fmt.Printf("\n]\n")
		}
	} else if cmd.Flags().Changed("status") {
		for _, resp := range respMsg.Response {
			printGPUWatchStatus(resp, true)
		}
		printGPUWatchSummary(len(respMsg.Response))
	} else {
		for _, resp := range respMsg.Response {
			printGPUWatchSpec(resp, true)
		}
		printGPUWatchSummary(len(respMsg.Response))
	}
	return nil
}

func gpuWatchStatsShowCmdHandler(cmd *cobra.Command, args []string) error {
	if cmd == nil || len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	cmd.SilenceUsage = true
	var req *aga.GPUWatchGetRequest
	if cmd.Flags().Changed("id") {
		if err := utils.IsUUIDValid(gpuWatchID); err != nil {
			return err
		}
		req = &aga.GPUWatchGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(gpuWatchID).Bytes()},
		}
	} else {
		req = &aga.GPUWatchGetRequest{}
	}
	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent running?")
	}
	defer c.Close()
	defer cancel()
	client := aga.NewGPUWatchSvcClient(c)
	respMsg, err := client.GPUWatchGet(ctxt, req)
	if err != nil {
		return fmt.Errorf("Getting GPU watch failed, err %v", err)
	}
	if respMsg.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Operation failed with %v error",
			respMsg.ApiStatus)
	}
	for _, resp := range respMsg.Response {
		printGPUWatchStats(resp, true)
	}
	printGPUWatchSummary(len(respMsg.Response))
	return nil
}

func gpuWatchAllShowCmdHandler(cmd *cobra.Command, args []string) error {
	if cmd == nil || len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	cmd.SilenceUsage = true
	var req *aga.GPUWatchGetRequest
	if cmd.Flags().Changed("id") {
		if err := utils.IsUUIDValid(gpuWatchID); err != nil {
			return err
		}
		req = &aga.GPUWatchGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(gpuWatchID).Bytes()},
		}
	} else {
		req = &aga.GPUWatchGetRequest{}
	}
	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent running?")
	}
	defer c.Close()
	defer cancel()
	client := aga.NewGPUWatchSvcClient(c)
	respMsg, err := client.GPUWatchGet(ctxt, req)
	if err != nil {
		return fmt.Errorf("Getting GPU watch failed, err %v", err)
	}
	if respMsg.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Operation failed with %v error",
			respMsg.ApiStatus)
	}
	for _, resp := range respMsg.Response {
		printGPUWatchSpec(resp, false)
		printGPUWatchStatus(resp, false)
		printGPUWatchStats(resp, false)
	}
	printGPUWatchSummary(len(respMsg.Response))
	return nil
}

func gpuWatchSubscribeCmdPreRun(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	gpuWatchIds := strings.Split(gpuWatchID, ",")
	for _, id := range gpuWatchIds {
		if err := utils.IsUUIDValid(id); err != nil {
			return err
		}
		gpuWatchUUIDs = append(gpuWatchUUIDs, uuid.FromStringOrNil(id).Bytes())
	}
	return nil
}

func gpuWatchSubscribeCmdHandler(cmd *cobra.Command, args []string) error {
	if cmd == nil || len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	cmd.SilenceUsage = true
	var rsp *aga.GPUWatch
	req := &aga.GPUWatchSubscribeRequest{
		Id: gpuWatchUUIDs,
	}
	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent running?")
	}
	defer c.Close()
	defer cancel()
	client := aga.NewGPUWatchSvcClient(c)
	stream, err := client.GPUWatchSubscribe(ctxt, req)
	if err != nil {
		return fmt.Errorf("GPU watch subscribe failed, err %v", err)
	}
	for {
		rsp, err = stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			return fmt.Errorf("GPU watch subscribe failed, err. %v\n", err)
		}
		printGPUWatchStats(rsp, true)
	}
	return nil
}
