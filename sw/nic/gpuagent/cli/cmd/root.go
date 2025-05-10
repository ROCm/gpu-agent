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
/// root node for gpuctl command line interface
///
//------------------------------------------------------------------------------

package cmd

import (
	"os"

	"github.com/ROCm/gpu-agent/sw/nic/gpuagent/cli/utils"
	"github.com/spf13/cobra"
)

var (
	svcURL  string
	svcPort string
)

// RootCmd represents the base command when called without any subcommands
var RootCmd = &cobra.Command{
	Use:   "gpuctl",
	Short: "GPU CLI",
	Long:  "\n----------------------\n AMD GPU CLI \n----------------------\n",
}

// Execute adds all child commands to the root command and sets flags appropriately.
// This is called by main.main(). It only needs to happen once to the RootCmd.
func Execute() {
	if err := RootCmd.Execute(); err != nil {
		os.Exit(1)
	}
}

func init() {
	cobra.OnInitialize(initConfig)
	RootCmd.PersistentFlags().StringVar(&svcURL, "node-svc-ip",
		utils.GRPCDefaultBaseURL,
		"Remote node's service URL")
	RootCmd.PersistentFlags().StringVar(&svcPort, "node-svc-port",
		utils.GRPCDefaultPort,
		"Remote node's service port")
}

// NewGpuctlCommand exports the RootCmd for bash-completion
func NewGpuctlCommand() *cobra.Command {
	return RootCmd
}

func initConfig() {
	// Note: initialize any config variables if required
	utils.GRPCDefaultBaseURL = svcURL
	utils.GRPCDefaultPort = svcPort
}
