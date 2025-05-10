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
/// gpuctl command line interface for events protobuf specifications
///
//------------------------------------------------------------------------------

package cmd

import (
	"fmt"
	"io"
	"strings"

	uuid "github.com/satori/go.uuid"
	"github.com/spf13/cobra"

	"github.com/ROCm/gpu-agent/sw/nic/gpuagent/cli/utils"
	aga "github.com/ROCm/gpu-agent/sw/nic/gpuagent/gen/go"
)

var (
	eventList        string
	eventSeverityStr string
	eventSeverity    aga.EventSeverity
	eventCategoryNum uint32
	eventCategory    aga.EventCategory
	eventIdList      []aga.EventId
	eventGpuId       string
	eventGpuUuids    [][]byte
)

var eventShowCmd = &cobra.Command{
	Use:     "events",
	Short:   "show system events",
	Long:    "show system events",
	PreRunE: eventShowCmdPreRunE,
	RunE:    eventShowCmdHandler,
}

var eventDebugCmd = &cobra.Command{
	Use:   "events",
	Short: "System events debug commands",
	Long:  "System events debug commands",
}

var eventSubscribeCmd = &cobra.Command{
	Use:     "subscribe",
	Short:   "subscribe to system events",
	Long:    "subscribe to system objects",
	PreRunE: eventSubscribeCmdPreRun,
	RunE:    eventSubscribeCmdHandler,
}

var eventGenCmd = &cobra.Command{
	Use:     "event",
	Short:   "generate system events",
	Long:    "generate system events",
	PreRunE: eventGenCmdPreRunE,
	RunE:    eventGenCmdHandler,
}

func init() {
	ShowCmd.AddCommand(eventShowCmd)
	eventShowCmd.Flags().StringVar(&eventList, "event-id", "", "Specify comma separated list of events of interest (1-5)")
	eventShowCmd.Flags().StringVarP(&eventGpuId, "gpu", "g", "", "Specify comma separated list of GPU ids")
	eventShowCmd.Flags().StringVar(&eventSeverityStr, "severity", "debug", "Specify severity of events of interest (debug, info, warn, critical)")
	eventShowCmd.Flags().Uint32Var(&eventCategoryNum, "category", 0, "Specify category of events of interest")
	//eventShowCmd.Flags().MarkHidden("category")
	//eventShowCmd.Flags().MarkHidden("severity")

	debugCmd.AddCommand(eventDebugCmd)
	eventDebugCmd.AddCommand(eventSubscribeCmd)
	eventSubscribeCmd.Flags().StringVar(&eventList, "event-id", "", "Specify comma separated list of events of interest (1-5)")
	eventSubscribeCmd.Flags().StringVarP(&eventGpuId, "gpu", "g", "", "Specify comma separated list of GPU ids")
	eventSubscribeCmd.MarkFlagRequired("gpu")
	eventSubscribeCmd.MarkFlagRequired("event-id")

	DebugCreateCmd.AddCommand(eventGenCmd)
	eventGenCmd.Flags().StringVarP(&eventList, "event-id", "i", "", "Specify comma separated list of events (1-5)")
	eventGenCmd.Flags().StringVarP(&eventGpuId, "gpu", "g", "", "Specify comma separated list of GPU ids")
	eventGenCmd.MarkFlagRequired("event-id")
	eventGenCmd.MarkFlagRequired("gpu")
}

func stringToEventId(eventIdStr string) (aga.EventId, error) {
	switch eventIdStr {
	case "0":
		return aga.EventId_EVENT_ID_NONE, nil
	case "1":
		return aga.EventId_EVENT_ID_VM_PAGE_FAULT, nil
	case "2":
		return aga.EventId_EVENT_ID_THERMAL_THROTTLE, nil
	case "3":
		return aga.EventId_EVENT_ID_GPU_PRE_RESET, nil
	case "4":
		return aga.EventId_EVENT_ID_GPU_POST_RESET, nil
	case "5":
		return aga.EventId_EVENT_ID_RING_HANG, nil
	default:
		break
	}
	return aga.EventId_EVENT_ID_NONE, fmt.Errorf("invalid_event_id")
}

func stringToEventSeverity(eventSeverityStr string) (aga.EventSeverity, error) {
	switch eventSeverityStr {
	case "debug":
		return aga.EventSeverity_EVENT_SEVERITY_DEBUG, nil
	case "info":
		return aga.EventSeverity_EVENT_SEVERITY_INFO, nil
	case "warn":
		return aga.EventSeverity_EVENT_SEVERITY_WARN, nil
	case "critical":
		return aga.EventSeverity_EVENT_SEVERITY_CRITICAL, nil
	}
	return aga.EventSeverity_EVENT_SEVERITY_NONE,
		fmt.Errorf("invalid event severity")
}

func eventShowCmdPreRunE(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	if cmd.Flags().Changed("event-id") {
		if cmd.Flags().Changed("severity") || cmd.Flags().Changed("category") {
			return fmt.Errorf("Invalid arguments, only \"severity\" or \"category\" cannot be specified with \"event-id\"")
		}
	}
	// filter option validations
	// event list
	if cmd.Flags().Changed("event-id") {
		eventIds := strings.Split(eventList, ",")
		eventIdList = make([]aga.EventId, len(eventIds))
		for i := 0; i < len(eventIds); i++ {
			eventId, err := stringToEventId(eventIds[i])
			if err != nil {
				return fmt.Errorf("Invalid event id %v in the event list", eventIds[i])
			}
			eventIdList[i] = eventId
		}
	}
	// event severity
	if cmd.Flags().Changed("severity") {
		svrty, err := stringToEventSeverity(eventSeverityStr)
		if err != nil {
			return fmt.Errorf("Invalid event \"severity\" level, refer to help string")
		}
		eventSeverity = svrty
	}
	// event category
	if cmd.Flags().Changed("category") {
		switch eventCategoryNum {
		case 0:
			eventCategory = aga.EventCategory_EVENT_CATEGORY_NONE
		case 255:
			eventCategory = aga.EventCategory_EVENT_CATEGORY_OTHER
		default:
			return fmt.Errorf("Invalid event \"category\"")
		}
	}
	// gpu list
	if cmd.Flags().Changed("gpu") {
		eventGpuIds := strings.Split(eventGpuId, ",")
		for _, id := range eventGpuIds {
			if err := utils.IsUUIDValid(id); err != nil {
				return err
			}
			eventGpuUuids = append(eventGpuUuids, uuid.FromStringOrNil(id).Bytes())
		}
	}
	return nil
}

func printEvent(event *aga.Event) {
	fmt.Printf("%-20s : %s\n", "Event Id", event.GetId().String())
	fmt.Printf("%-20s : %s\n", "GPU Id", utils.IdToStr(event.GetGPU()))
	fmt.Printf("%-20s : %s\n", "Severity", event.GetSeverity().String())
	fmt.Printf("%-20s : %s\n", "Timestamp", event.GetTime().String())
	fmt.Printf("%-20s : %s\n", "Description", event.GetDescription())
	fmt.Println(strings.Repeat("-", 62))
}

func eventSubscribeCmdPreRun(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	// filter option validations
	// event list
	if cmd.Flags().Changed("event-id") {
		eventIds := strings.Split(eventList, ",")
		eventIdList = make([]aga.EventId, len(eventIds))
		for i := 0; i < len(eventIds); i++ {
			eventId, err := stringToEventId(eventIds[i])
			if err != nil {
				return fmt.Errorf("Invalid event id %v in the event list", eventIds[i])
			}
			eventIdList[i] = eventId
		}
	}
	// gpu list
	if cmd.Flags().Changed("gpu") {
		eventGpuIds := strings.Split(eventGpuId, ",")
		for _, id := range eventGpuIds {
			if err := utils.IsUUIDValid(id); err != nil {
				return err
			}
			eventGpuUuids = append(eventGpuUuids, uuid.FromStringOrNil(id).Bytes())
		}
	}
	return nil
}

func eventSubscribeCmdHandler(cmd *cobra.Command, args []string) error {
	req := &aga.EventSubscribeRequest{}
	req.Filter = &aga.EventFilter{
		Filter: &aga.EventFilter_Events{
			Events: &aga.EventList{
				Id: eventIdList,
			},
		},
		Gpu: eventGpuUuids,
	}
	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent running?")
	}
	defer c.Close()
	defer cancel()

	client := aga.NewEventSvcClient(c)
	stream, err := client.EventSubscribe(ctxt, req)
	if err != nil {
		return fmt.Errorf("Event subscribe failed, err %v", err)
	}
	for {
		event, err := stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			return fmt.Errorf("Events receive failed, err %v", err)
		}
		printEvent(event)
	}
	return nil
}

func eventShowCmdHandler(cmd *cobra.Command, args []string) error {
	if len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	if cmd != nil {
		cmd.SilenceUsage = true
	}
	respMsg := &aga.EventResponse{}
	req := &aga.EventRequest{}
	req.Filter = &aga.EventFilter{}
	if cmd.Flags().Changed("event-id") {
		req.Filter.Filter = &aga.EventFilter_Events{
			Events: &aga.EventList{
				Id: eventIdList,
			},
		}
	} else if cmd.Flags().Changed("severity") || cmd.Flags().Changed("category") {
		req.Filter.Filter = &aga.EventFilter_MatchAttrs{
			MatchAttrs: &aga.EventMatchAttrs{
				Severity: eventSeverity,
				Category: eventCategory,
			},
		}
	}
	if cmd.Flags().Changed("gpu") {
		req.Filter.Gpu = eventGpuUuids
	}
	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent running?")
	}
	defer c.Close()
	defer cancel()

	client := aga.NewEventSvcClient(c)
	respMsg, err = client.EventGet(ctxt, req)
	if err != nil {
		return fmt.Errorf("Getting events failed, err %v", err)
	}
	if respMsg.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Operation failed with %v error", respMsg.ApiStatus)
	}
	for _, event := range respMsg.GetEvent() {
		printEvent(event)
	}
	fmt.Printf("Number of events: %d\n", len(respMsg.GetEvent()))
	return nil
}

func eventGenCmdPreRunE(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	// event list
	if cmd.Flags().Changed("event-id") {
		eventIds := strings.Split(eventList, ",")
		eventIdList = make([]aga.EventId, len(eventIds))
		for i := 0; i < len(eventIds); i++ {
			eventId, err := stringToEventId(eventIds[i])
			if err != nil {
				return fmt.Errorf("Invalid event id %v in the event list", eventIds[i])
			}
			eventIdList[i] = eventId
		}
	}
	// gpu list
	eventGpuIds := strings.Split(eventGpuId, ",")
	for _, id := range eventGpuIds {
		if err := utils.IsUUIDValid(id); err != nil {
			return err
		}
		eventGpuUuids = append(eventGpuUuids, uuid.FromStringOrNil(id).Bytes())
	}
	return nil
}

func eventGenCmdHandler(cmd *cobra.Command, args []string) error {
	if len(args) > 0 {
		return fmt.Errorf("Invalid argument")
	}
	if cmd != nil {
		cmd.SilenceUsage = true
	}
	respMsg := &aga.EventGenResponse{}
	req := &aga.EventGenRequest{
		Id:  eventIdList,
		GPU: eventGpuUuids,
	}
	// connect to GPU agent
	c, ctxt, cancel, err := utils.CreateNewAGAGRPClient()
	if err != nil {
		return fmt.Errorf("Could not connect to the GPU agent, is agent running?")
	}
	defer c.Close()
	defer cancel()

	client := aga.NewDebugEventSvcClient(c)
	respMsg, err = client.EventGen(ctxt, req)
	if err != nil {
		return fmt.Errorf("Generating events failed, err %v", err)
	}
	if respMsg.ApiStatus != aga.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("Operation failed with %v error", respMsg.ApiStatus)
	}
	fmt.Printf("Generated events\n")
	return nil
}
