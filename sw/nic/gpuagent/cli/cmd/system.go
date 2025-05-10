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
	"strings"

	"github.com/spf13/cobra"

	"github.com/ROCm/gpu-agent/sw/nic/gpuagent/cli/utils"
	aga "github.com/ROCm/gpu-agent/sw/nic/gpuagent/gen/go"
)

var (
	traceLevel string
	apiTrace   string
	apiTraceEn bool
)

var traceLevelShowCmd = &cobra.Command{
	Use:   "trace-level",
	Short: "show trace level information",
	Long:  "show trace level information",
	RunE:  TraceLevelShowCmdHandler,
}

var traceDebugCmd = &cobra.Command{
	Use:     "trace",
	Short:   "update debug trace level",
	Long:    "update debug trace level",
	PreRunE: traceDebugCmdPreRun,
	RunE:    traceDebugCmdHandler,
}

func init() {
	ShowCmd.AddCommand(traceLevelShowCmd)
	debugCmd.AddCommand(traceDebugCmd)
	traceDebugCmd.Flags().StringVar(&traceLevel, "level", "",
		"Specify trace level (allowed: none, error, warn, info, debug, verbose)")
	traceDebugCmd.Flags().StringVar(&apiTrace, "api-trace", "",
		"Specify whether to enable|disable api-trace log")
	traceDebugCmd.Flags().Bool("flush", false, "Flush logs")
}

func isTraceLevelValid(level string) bool {
	switch level {
	case "none":
		return true
	case "error":
		return true
	case "debug":
		return true
	case "warn":
		return true
	case "info":
		return true
	case "verbose":
		return true
	default:
		return false
	}
}

func traceDebugCmdPreRun(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}

	if cmd.Flags().NFlag() == 0 {
		return fmt.Errorf("Required argument(s) missing, refer to help string")
	}
	if cmd.Flags().Changed("level") && !isTraceLevelValid(traceLevel) {
		return fmt.Errorf("Invalid argument for \"level\", refer to help string")
	}
	if cmd.Flags().Changed("api-trace") || cmd.Flags().Changed("level") {
		var err error
		// if both api-trace and level are not specified,
		// do a get to get the one not spcified
		if cmd.Flags().Changed("api-trace") != cmd.Flags().Changed("level") {
			var empty aga.Empty

			// connect to GPU agent
			c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
			if err != nil {
				return fmt.Errorf("Could not connect to the GPU agent, is agent running?")
			}
			defer c.Close()
			defer cancel()

			client := aga.NewDebugSvcClient(c)
			// GPU agent call
			resp, err := client.TraceGet(ctxt, &empty)
			if err != nil {
				fmt.Printf("Trace level get failed, err %v\n", err)
				return err
			}
			if !cmd.Flags().Changed("level") {
				traceLevel = strings.ToLower(strings.Replace(
					resp.GetTraceLevel().String(), "TRACE_LEVEL_", "", -1))
			}
			if !cmd.Flags().Changed("api-trace") {
				apiTraceEn = resp.GetApiTraceEn()
			} else {
				apiTraceEn, err = utils.EnableDisableToBool(apiTrace)
				if err != nil {
					return fmt.Errorf("Invalid argument specified for api-trace")
				}
			}
		} else {
			apiTraceEn, err = utils.EnableDisableToBool(apiTrace)
			if err != nil {
				return fmt.Errorf("Invalid argument specified for api-trace")
			}
		}
		if apiTraceEn && traceLevel != "verbose" {
			fmt.Printf("Warning: \"level\" should be verbose for api-trace to take effect\n")
		}
	}
	return nil
}

func inputToTraceLevel(level string) aga.TraceLevel {
	switch level {
	case "none":
		return aga.TraceLevel_TRACE_LEVEL_NONE
	case "error":
		return aga.TraceLevel_TRACE_LEVEL_ERROR
	case "debug":
		return aga.TraceLevel_TRACE_LEVEL_DEBUG
	case "warn":
		return aga.TraceLevel_TRACE_LEVEL_WARN
	case "info":
		return aga.TraceLevel_TRACE_LEVEL_INFO
	case "verbose":
		return aga.TraceLevel_TRACE_LEVEL_VERBOSE
	default:
		return aga.TraceLevel_TRACE_LEVEL_NONE
	}
}

func traceDebugCmdHandler(cmd *cobra.Command, args []string) error {
	if len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	if cmd != nil {
		cmd.SilenceUsage = true
	}

	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent running?")
	}
	defer c.Close()
	defer cancel()

	client := aga.NewDebugSvcClient(c)
	if cmd != nil && (cmd.Flags().Changed("level") ||
		cmd.Flags().Changed("api-trace")) {
		var traceReq *aga.TraceRequest
		traceReq = &aga.TraceRequest{
			Spec: &aga.TraceSpec{
				TraceLevel: inputToTraceLevel(traceLevel),
				ApiTraceEn: apiTraceEn,
			},
		}
		// GPU agent call
		resp, err := client.TraceUpdate(ctxt, traceReq)
		if err != nil {
			return fmt.Errorf("Updating trace level failed, err %v", err)
		}

		// print trace level
		if resp.ApiStatus != aga.ApiStatus_API_STATUS_OK {
			return fmt.Errorf("Operation failed with %v error", resp.ApiStatus)
		}
		if cmd.Flags().Changed("level") {
			fmt.Printf("Trace level set to %-12s\n", traceLevel)
		}
		if cmd.Flags().Changed("api-trace") {
			if apiTraceEn {
				fmt.Println("API trace is enabled")
			} else {
				fmt.Println("API trace is disabled")
			}
		}
	}
	if cmd == nil || cmd.Flags().Changed("flush") {
		var empty aga.Empty

		// GPU agent call
		_, err := client.TraceFlush(ctxt, &empty)
		if err != nil {
			return fmt.Errorf("Flush logs failed, err %v", err)
		}

		fmt.Printf("Flush logs succeeded\n")
	}
	return nil
}

func TraceLevelShowCmdHandler(cmd *cobra.Command, args []string) error {
	var empty aga.Empty

	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent running?")
	}
	defer c.Close()
	defer cancel()

	client := aga.NewDebugSvcClient(c)
	// GPU agent call
	resp, err := client.TraceGet(ctxt, &empty)
	if err != nil {
		fmt.Printf("Trace level get failed, err %v\n", err)
		return err
	}
	fmt.Printf("%-15s : %s\n", "Trace level",
		strings.ToLower(strings.Replace(
			resp.GetTraceLevel().String(), "TRACE_LEVEL_", "", -1)))
	fmt.Printf("%-15s : %s\n", "Trace file", resp.GetTraceFile())
	return nil
}
