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
/// gpuctl command line interface for debug protobufs
///
//------------------------------------------------------------------------------

package cmd

import (
	"fmt"
	"sort"
	"strings"

	"github.com/spf13/cobra"

	"github.com/ROCm/gpu-agent/sw/nic/gpuagent/cli/utils"
	aga "github.com/ROCm/gpu-agent/sw/nic/gpuagent/gen/go"
)

var deviceShowCmd = &cobra.Command{
	Use:   "device",
	Short: "show device information",
	Long:  "show device information",
}

var deviceTopologyShowCmd = &cobra.Command{
	Use:     "topology",
	Short:   "show device topology information",
	Long:    "show device topology information",
	PreRunE: deviceTopologyShowCmdPreRunE,
	RunE:    deviceTopologyShowCmdHandler,
}

func init() {
	ShowCmd.AddCommand(deviceShowCmd)
	deviceShowCmd.AddCommand(deviceTopologyShowCmd)
	deviceTopologyShowCmd.Flags().BoolP("link-weight", "w", false,
		"Show link weight of connections between devices")
	deviceTopologyShowCmd.Flags().BoolP("num-hops", "n", false,
		"Show number of hops between devices")
	deviceTopologyShowCmd.Flags().BoolP("detail", "d", false,
		"Show device topology in detail")
}

func deviceTopologyShowCmdPreRunE(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	return nil
}

const (
	TOPO_PRINT_LINK_TYPE   uint32 = 0
	TOPO_PRINT_LINK_WEIGHT uint32 = 1
	TOPO_PRINT_NUM_HOPS    uint32 = 2
)

func printTopologyMatrix(resp *aga.DeviceTopologyGetResponse, printOpt uint32) {
	fmt.Println()
	if printOpt == TOPO_PRINT_LINK_TYPE {
		fmt.Printf("%s", strings.Repeat("-", 52))
		fmt.Printf(" Link type between two devices ")
		fmt.Printf("%s\n", strings.Repeat("-", 53))
	} else if printOpt == TOPO_PRINT_LINK_WEIGHT {
		fmt.Printf("%s", strings.Repeat("-", 51))
		fmt.Printf(" Link weight between two devices ")
		fmt.Printf("%s\n", strings.Repeat("-", 52))
	} else if printOpt == TOPO_PRINT_NUM_HOPS {
		fmt.Printf("%s", strings.Repeat("-", 50))
		fmt.Printf(" Number of hops between two devices ")
		fmt.Printf("%s\n", strings.Repeat("-", 50))
	}
	fmt.Println()
	// print header line with all the device names
	deviceTopo := resp.GetDeviceTopology()
	for i, device := range deviceTopo {
		if i == 0 {
			fmt.Printf("%-8s", "")
		}
		fmt.Printf("%-8s", device.GetDevice().GetName())
	}
	fmt.Println()
	// print details of each device
	for deviceIdx, device := range deviceTopo {
		fmt.Println()
		fmt.Printf("%-8s", device.GetDevice().GetName())
		peerDevices := device.GetPeerDevice()
		for peerDeviceIdx, peerDevice := range peerDevices {
			if deviceIdx == peerDeviceIdx {
				fmt.Printf("%-8s", "X")
			}
			if printOpt == TOPO_PRINT_LINK_TYPE {
				fmt.Printf("%-8s", strings.Replace(
					peerDevice.GetConnection().GetType().String(),
					"IO_LINK_TYPE_", "", -1))
			} else if printOpt == TOPO_PRINT_LINK_WEIGHT {
				fmt.Printf("%-8v", peerDevice.GetWeight())
			} else if printOpt == TOPO_PRINT_NUM_HOPS {
				fmt.Printf("%-8v", peerDevice.GetNumHops())
			}
		}
	}
	fmt.Printf("%-8s\n", "X")
}

func deviceTopologyShowCmdHandler(cmd *cobra.Command, args []string) error {
	if len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	if cmd != nil {
		cmd.SilenceUsage = true
	}

	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, " +
			"is agent running?")
	}
	defer c.Close()
	defer cancel()

	client := aga.NewTopoSvcClient(c)

	req := &aga.DeviceTopologyGetRequest{}
	// GPU agent call
	resp, err := client.DeviceTopologyGet(ctxt, req)
	if err != nil {
		return fmt.Errorf("Device topology get failed, err %v", err)
	}
	// sort device topology as per gpu id
	deviceTopo := resp.GetDeviceTopology()
	sort.Slice(deviceTopo, func(i, j int) bool {
		nameI := deviceTopo[i].GetDevice().GetName()
		nameJ := deviceTopo[j].GetDevice().GetName()
		var idI int
		var idJ int
		fmt.Sscanf(nameI, "GPU%d", &idI)
		fmt.Sscanf(nameJ, "GPU%d", &idJ)
		return idI < idJ
	})
	for _, device := range deviceTopo {
		peerDevices := device.GetPeerDevice()
		// sort peer devices as per gpu id
		sort.Slice(peerDevices, func(i, j int) bool {
			nameI := peerDevices[i].GetPeerDevice().GetName()
			nameJ := peerDevices[j].GetPeerDevice().GetName()
			var idI int
			var idJ int
			fmt.Sscanf(nameI, "GPU%d", &idI)
			fmt.Sscanf(nameJ, "GPU%d", &idJ)
			return idI < idJ
		})
	}
	// print trace level
	if resp.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Operation failed with %v error", resp.ApiStatus)
	}
	if cmd.Flags().Changed("detail") {
		printTopologyMatrix(resp, TOPO_PRINT_LINK_TYPE)
		printTopologyMatrix(resp, TOPO_PRINT_LINK_WEIGHT)
		printTopologyMatrix(resp, TOPO_PRINT_NUM_HOPS)
	} else if cmd.Flags().Changed("link-weight") {
		printTopologyMatrix(resp, TOPO_PRINT_LINK_WEIGHT)
	} else if cmd.Flags().Changed("num-hops") {
		printTopologyMatrix(resp, TOPO_PRINT_NUM_HOPS)
	} else {
		printTopologyMatrix(resp, TOPO_PRINT_LINK_TYPE)
	}
	fmt.Println()
	return nil
}
