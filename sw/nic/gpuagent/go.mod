module github.com/ROCm/gpu-agent/sw/nic/gpuagent

go 1.24.6

require (
	github.com/gogo/protobuf v1.3.2
	github.com/satori/go.uuid v1.2.0
	github.com/spf13/cobra v1.9.1
	google.golang.org/grpc v1.69.0-dev
	google.golang.org/protobuf v1.34.2
	gopkg.in/yaml.v2 v2.4.0
)

require (
	github.com/inconshreveable/mousetrap v1.1.0 // indirect
	github.com/spf13/pflag v1.0.6 // indirect
	golang.org/x/net v0.30.0 // indirect
	golang.org/x/sys v0.33.0 // indirect
	golang.org/x/text v0.25.0 // indirect
	google.golang.org/genproto/googleapis/rpc v0.0.0-20240903143218-8af14fe29dc1 // indirect
)

replace golang.org/x/net v0.30.0 => golang.org/x/net v0.38.0
