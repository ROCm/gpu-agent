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
/// gpuctl command line interface for version
///
//------------------------------------------------------------------------------

package cmd

import (
	"fmt"

	"github.com/spf13/cobra"

	"github.com/ROCm/gpu-agent/sw/nic/gpuagent/cli/utils"
	aga "github.com/ROCm/gpu-agent/sw/nic/gpuagent/gen/go"
)

var (
	// GpuctlVersion is set at build time via ldflags
	GpuctlVersion = "unknown"
)

var versionShowCmd = &cobra.Command{
	Use:   "version",
	Short: "Show version information",
	Long:  "Display gpuctl and gpuagent version",
	RunE:  versionShowCmdHandler,
}

func versionShowCmdHandler(cmd *cobra.Command, args []string) error {
	if len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	if cmd != nil {
		cmd.SilenceUsage = true
	}
	fmt.Printf("%-10s: %s\n", "gpuctl", GpuctlVersion)
	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent " +
			"running?")
	}
	defer c.Close()
	defer cancel()

	client := aga.NewDebugSvcClient(c)

	req := &aga.Empty{}
	resp, err := client.VersionGet(ctxt, req)
	if err != nil {
		return fmt.Errorf("Getting gpuagent version failed, err %v", err)
	}
	fmt.Printf("%-10s: %s\n", "gpuagent", resp.GetVersion())
	return nil
}

func init() {
	ShowCmd.AddCommand(versionShowCmd)
}
