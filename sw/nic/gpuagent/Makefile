#
# Copyright(C) Advanced Micro Devices, Inc. All rights reserved.
#
# You may not use this software and documentation (if any) (collectively,
# the "Materials") except in compliance with the terms and conditions of
# the Software License Agreement included with the Materials or otherwise as
# set forth in writing and signed by you and an authorized signatory of AMD.
# If you do not have a copy of the Software License Agreement, contact your
# AMD representative for a copy.
#
# You agree that you will not reverse engineer or decompile the Materials,
# in whole or in part, except as allowed by applicable law.
#
# THE MATERIALS ARE DISTRIBUTED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OR
# REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

export TOPDIR                := ${realpath ../..}
export ABS_DIR               := /usr/src/github.com/ROCm/gpu-agent/sw
export ARCH                  ?= x86_64
export PLATFORM              ?= sim
export BLD_DIR               := $(ABS_DIR)/nic/build


# Third-party library directories
export PROTOBUF_DIR          := $(TOPDIR)/nic/third-party/protobuf
export GRPC_DIR              := $(TOPDIR)/nic/third-party/grpc
export ZEROMQ_DIR            := $(TOPDIR)/nic/third-party/libzmq
export LIBEV_DIR             := $(TOPDIR)/nic/third-party/libev-4.33
export BOOST_DIR             := $(TOPDIR)/nic/third-party/boost_1_88_0
export ABSEIL_DIR            := $(TOPDIR)/nic/third-party/abseil-cpp

export BLD_ARCH_DIR          := ${BLD_DIR}/$(ARCH)/$(PLATFORM)
export BLD_OUT_DIR           := ${BLD_ARCH_DIR}/out
export BLD_LIB_DIR           := ${BLD_ARCH_DIR}/lib
export BLD_BIN_DIR           := ${BLD_ARCH_DIR}/bin
export BLD_GEN_DIR           := ${BLD_ARCH_DIR}/gen
export BLD_PROTOGEN_DIR      := ${BLD_GEN_DIR}/proto
export SDKDIR                := $(TOPDIR)/nic/sdk
export CC                    := g++
export PROTOC                := ${BLD_DIR}/bin/protoc

export SPDLOG_INC_DIR        ?= ${TOPDIR}/nic/third-party/spdlog/include
export PROTOBUF_INCLUDE      := ${BLD_DIR}/include
export GRPC_INCLUDE          := ${PROTOBUF_INCLUDE} /usr/local/include/grpc/include
export GRPC_CPP_PLUGIN       := ${BLD_BIN_DIR}/grpc_cpp_plugin
export GRPC_PY_PLUGIN        := ${BLD_BIN_DIR}/grpc_python_plugin
export PROTOC_LIB_PATH       := ${BLD_DIR}/lib
export OTHER_THIRD_PARTY_INC ?= ${SPDLOG_INC_DIR} ${GFLAGS_INC_DIR} ${GRPC_INCLUDE}

TARGET                       := gpuagent
EXCLUDE_DIRS                 := $(TOPDIR)/nic/third-party
EXCLUDE_VENDOR               := $(ABS_DIR)/nic/gpuagent/vendor
SMI_SRC_DIR                  := $(TOPDIR)/nic/gpuagent/api/smi
GPUAGENT_PROTO_DIR           := protos
GPUAGENT_PROTO_DIR_ABS       := ${TOPDIR}/nic/gpuagent/protos
GOGO_PROTO_DIR               := ${TOPDIR}/vendor/github.com/gogo/protobuf/gogoproto
GPUAGENT_PROTO_GEN_DIR       := ${BLD_PROTOGEN_DIR}/gpuagent/
GPUAGENT_PROTO_GO_GEN_DIR    := ${ABS_DIR}/nic/gpuagent/gen/go
GPUAGENT_PROTO_SRCS          := $(wildcard ${GPUAGENT_PROTO_DIR}/*.proto)
GPU_PROTO_GO_FILES           := $(filter-out protos/types.proto, $(wildcard ${GPUAGENT_PROTO_DIR_ABS}/*.proto))
GOGO_PROTO_SRCS              := ${GOGO_PROTO_DIR}/gogo.proto

SRC   := $(shell find $(TOPDIR) -type d \( -path $(EXCLUDE_DIRS) \) -prune -o \
                                -type d \( -path $(EXCLUDE_VENDOR) \) -prune -o \
                                -type d \( -path $(SMI_SRC_DIR) \) -prune -o \
                                -type d \( -path $(BLD_PROTOGEN_DIR) \) -prune -o \
                                -type f -name "*.cc" -print)
SRC   += $(patsubst $(GPUAGENT_PROTO_DIR)/%.proto, $(GPUAGENT_PROTO_GEN_DIR)/%.pb.cc, $(GPUAGENT_PROTO_SRCS))
SRC   += $(patsubst $(GPUAGENT_PROTO_DIR)/%.proto, $(GPUAGENT_PROTO_GEN_DIR)/%.grpc.pb.cc, $(GPUAGENT_PROTO_SRCS))
SRC   += $(patsubst $(GOGO_PROTO_DIR)/%.proto, $(BLD_PROTOGEN_DIR)/%.pb.cc, $(GOGO_PROTO_SRCS))
SRC   += $(patsubst $(GOGO_PROTO_DIR)/%.proto, $(BLD_PROTOGEN_DIR)/%.grpc.pb.cc, $(GOGO_PROTO_SRCS))
SRC_C := $(shell find $(TOPDIR) -type d \( -path $(EXCLUDE_DIRS) \) -prune -o \
                                -type d \( -path $(EXCLUDE_VENDOR) \) -prune -o \
                                -type d \( -path $(BLD_DIR) \) -prune -o \
                                -type f -name "*.c" -print)
INC_COMMON   := $(TOPDIR) $(BLD_ARCH_DIR) $(BLD_PROTOGEN_DIR) \
                $(TOPDIR)/nic/sdk/ $(OTHER_THIRD_PARTY_INC) \
                ${BLD_DIR}/include
INC_AMD_SMI  := ${TOPDIR}/nic/third-party/rocm/amd_smi_lib/include/
INC_GIM_SMI  := ${TOPDIR}/nic/third-party/rocm/gim_amd_smi_lib/include/

# SMI sources
SMI_CC_SRCS     	:= $(wildcard ${SMI_SRC_DIR}/*.cc)
SMI_MOCK_SRCS   	:= $(wildcard ${SMI_SRC_DIR}/*mock*.cc)

# amdsmi sources
AMD_SMI_CC_SRCS    	:= $(wildcard ${SMI_SRC_DIR}/amdsmi/*.cc)
AMD_SMI_MOCK_SRCS   := $(wildcard ${SMI_SRC_DIR}/amdsmi/*mock*.cc)
AMD_SMI_SRC         := $(SRC) $(filter-out $(SMI_MOCK_SRCS), $(SMI_CC_SRCS)) \
                       $(filter-out $(AMD_SMI_MOCK_SRCS), $(AMD_SMI_CC_SRCS))

# gim amdsmi sources
GIM_SMI_CC_SRCS    	:= $(wildcard ${SMI_SRC_DIR}/gimamdsmi/*.cc)
GIM_SMI_MOCK_SRCS   := $(wildcard ${SMI_SRC_DIR}/gimamdsmi/*mock*.cc)
GIM_SMI_SRC         := $(SRC) $(filter-out $(SMI_MOCK_SRCS), $(SMI_CC_SRCS)) \
                       $(filter-out $(GIM_SMI_MOCK_SRCS), $(GIM_SMI_CC_SRCS))

# amdsmi mock sources
AMD_SMI_MOCK_SRC    := $(SRC) $(SMI_MOCK_SRCS) $(AMD_SMI_MOCK_SRCS)

PROTO_INCS      := ${GPUAGENT_PROTO_DIR} \
                   ${TOPDIR}/vendor/github.com/gogo/protobuf/protobuf \
                   ${TOPDIR}/vendor/github.com/gogo/protobuf/gogoproto \
                   ${TOPDIR}/vendor/github.com/pensando/grpc-gateway/third_party \
                   ${TOPDIR}/vendor/github.com/pensando/grpc-gateway/third_party/googleapis \
                   ${PROTOBUF_INCLUDE}
PROTO_INCS      := $(addprefix -I,${PROTO_INCS})
PROTOC_CPP_OPTS := --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
PROTOC_PY_OPTS  := --plugin=protoc-gen-grpc=${GRPC_PY_PLUGIN}
PROTO_PROTOC_GOFAST_OPTS = --gogofast_out=Mgogo.proto=github.com/gogo/protobuf/gogoproto,plugins=grpc:${GPUAGENT_PROTO_DIR}

OBJ_DIR         := $(BLD_OUT_DIR)/$(TARGET)_bin
OBJ             := $(patsubst $(TOPDIR)/%, $(OBJ_DIR)/%, $(AMD_SMI_SRC:.cc=.o))
OBJ             += $(patsubst $(TOPDIR)/%, $(OBJ_DIR)/%, $(SRC_C:.c=.o))
OBJ_DIR_GIM     := $(BLD_OUT_DIR)/$(TARGET)_gim_bin
OBJ_GIM         := $(patsubst $(TOPDIR)/%, $(OBJ_DIR_GIM)/%, $(GIM_SMI_SRC:.cc=.o))
OBJ_GIM         += $(patsubst $(TOPDIR)/%, $(OBJ_DIR_GIM)/%, $(SRC_C:.c=.o))
OBJ_DIR_MOCK    := $(BLD_OUT_DIR)/$(TARGET)_mock_bin
OBJ_MOCK        := $(patsubst $(TOPDIR)/%, $(OBJ_DIR_MOCK)/%, $(AMD_SMI_MOCK_SRC:.cc=.o))
OBJ_MOCK        += $(patsubst $(TOPDIR)/%, $(OBJ_DIR_MOCK)/%, $(SRC_C:.c=.o))

INCS_AMD_SMI    := $(addprefix -I,${INC_COMMON} ${INC_AMD_SMI})
INCS_GIM_SMI    := $(addprefix -I,${INC_COMMON} ${INC_GIM_SMI})
INCS_MOCK       := $(addprefix -I,${INC_COMMON})

LDFLAGS_COMMON  := -pthread -rdynamic -no-canonical-prefixes -Wl,--gc-sections \
                   -Wl,-z,relro,-z,now -Wl,--build-id=md5 -Wl,--hash-style=gnu \
                   -L$(BLD_LIB_DIR) -Wl,-rpath,$(BLD_LIB_DIR) \
				   -Wl,-rpath-link,$(BLD_LIB_DIR) -L${BLD_DIR}/lib -L${BLD_DIR}/lib64 \
                   -l:libprotobuf.a -lgrpc++_reflection -lgrpc++ -lgrpc -lgpr -lupb -lcares \
                   -laddress_sorting -l:libgrpc.a -l:libre2.a -labsl_synchronization -labsl_periodic_sampler -labsl_time \
                   -labsl_time_zone -labsl_int128 -labsl_bad_optional_access -labsl_bad_any_cast_impl -labsl_bad_variant_access\
                   -labsl_spinlock_wait -labsl_log_severity -labsl_strerror -labsl_raw_logging_internal -labsl_throw_delegate \
                   -labsl_scoped_set_env -labsl_base -labsl_malloc_internal -labsl_hash -labsl_low_level_hash -labsl_city \
                   -labsl_civil_time -labsl_leak_check_disable -labsl_leak_check -labsl_stacktrace -labsl_symbolize \
                   -labsl_debugging_internal -labsl_demangle_internal -labsl_examine_stack -labsl_failure_signal_handler \
                   -labsl_graphcycles_internal -labsl_random_internal_randen -labsl_random_internal_randen_slow \
                   -labsl_random_internal_randen_hwaes_impl -labsl_random_internal_randen_hwaes -labsl_random_distributions \
                   -labsl_random_internal_platform -labsl_random_internal_pool_urbg -labsl_random_seed_gen_exception \
                   -labsl_random_internal_seed_material -labsl_random_internal_distribution_test_util -labsl_random_seed_sequences \
                   -labsl_hashtablez_sampler -labsl_raw_hash_set -labsl_strings -labsl_strings_internal -labsl_cord \
                   -labsl_cord_internal -labsl_str_format_internal -labsl_cordz_info -labsl_cordz_handle \
                   -labsl_cordz_sample_token -labsl_cordz_functions -labsl_exponential_biased -labsl_status \
                   -labsl_statusor -labsl_flags_commandlineflag_internal -labsl_flags_commandlineflag \
                   -labsl_flags_private_handle_accessor -labsl_flags_marshalling -labsl_flags_program_name \
                   -labsl_flags_config -labsl_flags_internal -labsl_flags_reflection -labsl_flags -labsl_flags_usage_internal \
                   -labsl_flags_usage -labsl_flags_parse -lpthread -lz -lm -lrt -ldl -l:libev.a -l:libzmq.a \
                   -l:libssl.a -l:libcrypto.a
LDFLAGS_AMD_SMI := ${LDFLAGS_COMMON} \
                   -L${TOPDIR}/nic/third-party/rocm/amd_smi_lib/x86_64/lib/ \
                   -l:libamd_smi.so
LDFLAGS_GIM_SMI := ${LDFLAGS_COMMON} \
                   -L${TOPDIR}/nic/third-party/rocm/gim_amd_smi_lib/x86_64/lib/ \
                   -l:libgim_amd_smi.so
LDFLAGS_MOCK    := ${LDFLAGS_COMMON}

$(TARGET): build-libs gogo-protos gen-protos $(OBJ)
	mkdir -p $(BLD_BIN_DIR)
	$(CC) $(OBJ) $(LDFLAGS_AMD_SMI) -o $(BLD_BIN_DIR)/$(TARGET)
	${MAKE} gpuctl

$(TARGET)_mock: build-libs gogo-protos gen-protos $(OBJ_MOCK)
	mkdir -p $(BLD_BIN_DIR)
	$(CC) $(OBJ_MOCK) $(LDFLAGS_MOCK) -o $(BLD_BIN_DIR)/$(TARGET)_mock
	${MAKE} gpuctl

$(TARGET)_gim: build-libs gogo-protos gen-protos $(OBJ_GIM)
	mkdir -p $(BLD_BIN_DIR)
	$(CC) $(OBJ_GIM) $(LDFLAGS_GIM_SMI) -o $(BLD_BIN_DIR)/$(TARGET)_gim
	${MAKE} gpuctl

.PHONY: mod
mod:
	@go mod tidy
	@go mod edit -go=1.24.6
	#CVE-2024-24790 - gpuctl
	@go mod edit -replace golang.org/x/net@v0.30.0=golang.org/x/net@v0.38.0
	@go mod vendor


.PHONY: all
all: build-libs gogo-protos gen-protos $(OBJ) $(OBJ_GIM) $(OBJ_MOCK)
	mkdir -p $(BLD_BIN_DIR)
	$(CC) $(OBJ) $(LDFLAGS_AMD_SMI) -o $(BLD_BIN_DIR)/$(TARGET)
	$(CC) $(OBJ_GIM) $(LDFLAGS_GIM_SMI) -o $(BLD_BIN_DIR)/$(TARGET)_gim
	$(CC) $(OBJ_MOCK) $(LDFLAGS_MOCK) -o $(BLD_BIN_DIR)/$(TARGET)_mock
	${MAKE} gpuctl

BUILD_LIBS_STAMP := $(BLD_OUT_DIR)/.build_libs_done

# these are maintained in seperate file to keep the variables and settings
# contained (TBD, clean up later)
$(BUILD_LIBS_STAMP):
	@mkdir -p $(dir $@)
	$(MAKE) -f Makefile.lib third-party-libs
	@touch $@

build-libs: $(BUILD_LIBS_STAMP)


gen-protos:
	@mkdir -p ${GPUAGENT_PROTO_GEN_DIR}
	LC_ALL=C LD_LIBRARY_PATH=${PROTOC_LIB_PATH} ${PROTOC} --cpp_out=${GPUAGENT_PROTO_GEN_DIR} --grpc_out=${GPUAGENT_PROTO_GEN_DIR} ${PROTOC_CPP_OPTS} ${PROTO_INCS} ${PROTOC_DEFS} ${GPUAGENT_PROTO_SRCS}
	@mkdir -p ${GPUAGENT_PROTO_GO_GEN_DIR}
	${PROTOC} --proto_path=${GPUAGENT_PROTO_DIR_ABS} -I${GPUAGENT_PROTO_DIR_ABS} \
    -I${ABS_DIR}/vendor/github.com/gogo/protobuf/gogoproto \
    --gogofast_out=Mgogo.proto=github.com/gogo/protobuf/gogoproto,plugins=grpc:${GPUAGENT_PROTO_GO_GEN_DIR} \
    types.proto ${GPU_PROTO_GO_FILES}

gpuctl:
	@echo "building gpuctl"
	CGO_ENABLED=0 go build -C cli -o ${BLD_BIN_DIR}/gpuctl

gogo-protos:
	@mkdir -p ${BLD_PROTOGEN_DIR}
	LC_ALL=C LD_LIBRARY_PATH=${PROTOC_LIB_PATH} ${PROTOC} --cpp_out=${BLD_PROTOGEN_DIR} --grpc_out=${BLD_PROTOGEN_DIR} ${PROTOC_CPP_OPTS} ${PROTO_INCS} ${PROTOC_DEFS} ${GOGO_PROTO_SRCS}

$(OBJ_DIR)/%.o: $(TOPDIR)/%.cc
	@mkdir -p $(dir $@)  # Create the necessary subdirectories in temp
	$(CC) $(CFLAGS) $(DEFS) $(INCS_AMD_SMI) -DAMD_SMI -D__FNAME__=__FILE__ -c $< -o $@

$(OBJ_DIR)/%.o: $(TOPDIR)/%.c
	@mkdir -p $(dir $@)  # Create the necessary subdirectories in temp
	$(CC) $(CFLAGS)  $(INCS_AMD_SMI) -DAMD_SMI -D__FNAME__=__FILE__ -c $< -o $@

$(OBJ_DIR_GIM)/%.o: $(TOPDIR)/%.cc
	@mkdir -p $(dir $@)  # Create the necessary subdirectories in temp
	$(CC) $(CFLAGS) $(DEFS) $(INCS_GIM_SMI) -DGIM_AMD_SMI -D__FNAME__=__FILE__ -c $< -o $@

$(OBJ_DIR_GIM)/%.o: $(TOPDIR)/%.c
	@mkdir -p $(dir $@)  # Create the necessary subdirectories in temp
	$(CC) $(CFLAGS)  $(INCS_GIM_SMI) -DGIM_AMD_SMI -D__FNAME__=__FILE__ -c $< -o $@

$(OBJ_DIR_MOCK)/%.o: $(TOPDIR)/%.cc
	@mkdir -p $(dir $@)  # Create the necessary subdirectories in temp
	$(CC) $(CFLAGS) $(DEFS) $(INCS_MOCK) -DAMD_SMI -D__FNAME__=__FILE__ -c $< -o $@

$(OBJ_DIR_MOCK)/%.o: $(TOPDIR)/%.c
	@mkdir -p $(dir $@)  # Create the necessary subdirectories in temp
	$(CC) $(CFLAGS)  $(INCS_MOCK) -DAMD_SMI -D__FNAME__=__FILE__ -c $< -o $@

clean:
	rm -rf $(BLD_BIN_DIR) $(OBJ_DIR) $(OBJ_DIR_GIM) $(OBJ_DIR_MOCK) $(BLD_PROTOGEN_DIR); \
	rm -rf $(GRPC_DIR)/cmake/build; \
	rm -rf $(BLD_OUT_DIR)/.build_libs_done \
	rm -rf $(ABSEIL_DIR)/cmake/build; sync
