module github.com/ROCm/gpu-agent/sw/nic/gpuagent

go 1.25.12

require (
	github.com/gogo/protobuf v1.3.2
	github.com/satori/go.uuid v1.2.0
	github.com/spf13/cobra v1.9.1
	google.golang.org/grpc v1.69.0-dev
	google.golang.org/protobuf v1.36.11
	gopkg.in/yaml.v2 v2.4.0
)

require (
	github.com/inconshreveable/mousetrap v1.1.0 // indirect
	github.com/spf13/pflag v1.0.6 // indirect
	golang.org/x/net v0.53.0 // indirect
	golang.org/x/sys v0.46.0 // indirect
	golang.org/x/text v0.38.0 // indirect
	google.golang.org/genproto/googleapis/rpc v0.0.0-20260414002931-afd174a4e478 // indirect
)

replace (
	golang.org/x/net v0.30.0 => golang.org/x/net v0.38.0
	golang.org/x/net v0.53.0 => golang.org/x/net v0.56.0
)

replace (
	google.golang.org/grpc v1.69.0-dev => google.golang.org/grpc v1.82.1
	google.golang.org/grpc v1.72.1 => google.golang.org/grpc v1.82.1
)

replace golang.org/x/sys v0.39.0 => golang.org/x/sys v0.44.0

replace golang.org/x/text v0.38.0 => golang.org/x/text v0.39.0
